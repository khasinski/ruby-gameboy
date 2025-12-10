/**
 * mrbz - Minimal Ruby for Game Boy
 * Virtual Machine implementation
 */

#include "vm.h"
#include "opcodes.h"
#include <stdio.h>
#include <string.h>

// Forward declaration for built-in dispatch (using output param for SDCC)
extern void mrbz_builtin_call(mrbz_vm* vm, uint8_t sym_idx, uint8_t argc, uint8_t base_reg, mrbz_value* ret);

// Debug flag (disable for release, or enable with -DMRBZ_DEBUG=1)
#ifndef MRBZ_DEBUG
#define MRBZ_DEBUG 0
#endif

#if MRBZ_DEBUG
#define DBG_PRINT(...) printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

// Initialize the VM
void mrbz_vm_init(mrbz_vm* vm) {
    uint8_t i;
    vm->running = 0;
    vm->next_array = 0;
    vm->sym_count = 0;
    vm->ivar_count = 0;
    vm->const_count = 0;
    vm->method_count = 0;
    vm->call_depth = 0;
    vm->bytecode = 0;

    // Clear registers
    for (i = 0; i < MRBZ_MAX_REGS; i++) {
        MRBZ_SET_NIL(vm->regs[i]);
    }

    // Clear array lengths
    for (i = 0; i < MRBZ_MAX_ARRAYS; i++) {
        vm->array_lens[i] = 0;
    }

    // Clear symbol table
    for (i = 0; i < MRBZ_MAX_SYMBOLS; i++) {
        vm->sym_names[i] = 0;
    }
}

// Get symbol name by index
const char* mrbz_get_symbol(mrbz_vm* vm, uint8_t idx) {
    if (idx < vm->sym_count) {
        return vm->sym_names[idx];
    }
    return 0;
}

// Read 16-bit value from bytecode (big-endian)
static uint16_t read_u16(const uint8_t* p) {
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

// Read 32-bit value from bytecode (big-endian)
static uint32_t read_u32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

// Parse symbol table from bytecode
// Symbols are after the instruction bytes, in format:
// 2 bytes: symbol count
// For each symbol: 2 bytes length + characters + null terminator
static void parse_symbols(mrbz_vm* vm, uint16_t offset) {
    const uint8_t* p;
    uint16_t count, len;
    uint8_t i;

    p = vm->bytecode + offset;
    count = read_u16(p);
    p += 2;

    DBG_PRINT("Parsing %d symbols at offset %d\n", count, offset);

    for (i = 0; i < count && i < MRBZ_MAX_SYMBOLS; i++) {
        len = read_u16(p);
        p += 2;
        vm->sym_names[i] = (const char*)p;
        DBG_PRINT("  sym[%d] = \"%s\"\n", i, vm->sym_names[i]);
        p += len + 1;  // +1 for null terminator
    }
    vm->sym_count = (i < count) ? i : count;
}

// Print a value (for debugging)
void mrbz_print_value(mrbz_value v) {
    switch (v.type) {
        case MRBZ_T_NIL:   printf("nil"); break;
        case MRBZ_T_TRUE:  printf("true"); break;
        case MRBZ_T_FALSE: printf("false"); break;
        case MRBZ_T_INT:   printf("%d", v.v.i); break;
        case MRBZ_T_SYMBOL: printf(":sym%d", v.v.sym); break;
        case MRBZ_T_ARRAY: printf("[arr#%d]", v.v.arr); break;
        default: printf("?"); break;
    }
}

// Compare two values for equality
static uint8_t values_equal(mrbz_value a, mrbz_value b) {
    if (a.type != b.type) return 0;
    switch (a.type) {
        case MRBZ_T_NIL:
        case MRBZ_T_TRUE:
        case MRBZ_T_FALSE:
            return 1;
        case MRBZ_T_INT:
            return a.v.i == b.v.i;
        case MRBZ_T_SYMBOL:
            return a.v.sym == b.v.sym;
        case MRBZ_T_ARRAY:
            return a.v.arr == b.v.arr;
        default:
            return 0;
    }
}

// Allocate a new array, return its index
static uint8_t alloc_array(mrbz_vm* vm) {
    uint8_t idx;
    if (vm->next_array >= MRBZ_MAX_ARRAYS) {
        printf("ERR: arrays full\n");
        return 0;
    }
    idx = vm->next_array;
    vm->next_array++;
    vm->array_lens[idx] = 0;
    return idx;
}

// Run the VM on bytecode
void mrbz_vm_run(mrbz_vm* vm, mrbz_value* result, const uint8_t* bytecode) {
    uint16_t pc;
    uint16_t ilen;
    uint16_t inst_end;
    uint8_t op;
    uint8_t a, b, c;
    int16_t val, offset;
    uint8_t arr_idx;

    vm->bytecode = bytecode;
    vm->running = 1;

    // Parse bytecode header (mruby RITE format)
    // Bytes 0-7:   "RITE0300" magic
    // Bytes 8-11:  total size (big-endian)
    // Bytes 12-15: "MATZ" compiler ident
    // Bytes 16-19: compiler version
    pc = 20;

    // Bytes 20-23: "IREP" section marker
    pc += 4;

    // Bytes 24-27: IREP section size
    pc += 4;

    // Bytes 28-31: "0300" IREP version (2) + record size (2, but actually 4 bytes total)
    pc += 4;

    // Bytes 32-35: record size (4 bytes)
    pc += 4;

    // Bytes 36-37: nlocals (2 bytes)
    // Bytes 38-39: nregs (2 bytes)
    // Bytes 40-41: rlen (2 bytes) - number of child IREPs
    // Bytes 42-43: clen (2 bytes) - catch handler table size
    pc += 8;

    // Bytes 44-47: ilen (4 bytes) - instruction length in bytes
    ilen = read_u16(bytecode + pc + 2);
    pc += 4;

    // Now pc = 48, which is where instructions start
    inst_end = pc + ilen;

    DBG_PRINT("Header: %.4s\n", bytecode);
    DBG_PRINT("ilen=%d pc=%d end=%d\n", ilen, pc, inst_end);

    // Parse pool and symbols after instructions
    // After instructions: 2-byte pool count, then pool entries
    //                     2-byte symbol count, then symbol entries
    {
        uint16_t pool_offset, pool_count, sym_offset;
        pool_offset = inst_end;
        pool_count = read_u16(bytecode + pool_offset);
        // Skip pool entries (each is 1 byte type + data)
        // For simplicity, assume no pool entries (plen=0) for now
        sym_offset = pool_offset + 2;  // Skip pool count
        DBG_PRINT("pool_count=%d sym_offset=%d\n", pool_count, sym_offset);
        parse_symbols(vm, sym_offset);
    }

    // Main execution loop
    while (vm->running && pc < inst_end) {
        op = bytecode[pc];
        pc++;
        DBG_PRINT("PC=%d OP=0x%02X\n", pc-1, op);

        // Bounds check
        if (op > 0x69) {
            printf("Bad OP: 0x%02X at %d\n", op, pc-1);
            vm->running = 0;
            MRBZ_SET_NIL(*result);
            break;
        }

        switch (op) {
            case OP_NOP:
                break;

            case OP_MOVE:
                a = bytecode[pc++];
                b = bytecode[pc++];
                vm->regs[a] = vm->regs[b];
                DBG_PRINT("  MOVE R%d <- R%d\n", a, b);
                break;

            case OP_LOADI_0: case OP_LOADI_1: case OP_LOADI_2: case OP_LOADI_3:
            case OP_LOADI_4: case OP_LOADI_5: case OP_LOADI_6: case OP_LOADI_7:
                a = bytecode[pc++];
                val = op - OP_LOADI_0;
                MRBZ_SET_INT(vm->regs[a], val);
                DBG_PRINT("  LOADI_%d R%d\n", val, a);
                break;

            case OP_LOADI__1:
                a = bytecode[pc++];
                MRBZ_SET_INT(vm->regs[a], -1);
                DBG_PRINT("  LOADI_-1 R%d\n", a);
                break;

            case OP_LOADI:
                a = bytecode[pc++];
                b = bytecode[pc++];
                MRBZ_SET_INT(vm->regs[a], (int16_t)b);
                DBG_PRINT("  LOADI R%d <- %d\n", a, b);
                break;

            case OP_LOADINEG:
                a = bytecode[pc++];
                b = bytecode[pc++];
                MRBZ_SET_INT(vm->regs[a], -(int16_t)b);
                DBG_PRINT("  LOADINEG R%d <- -%d\n", a, b);
                break;

            case OP_LOADI16:
                a = bytecode[pc++];
                val = (int16_t)read_u16(bytecode + pc);
                pc += 2;
                MRBZ_SET_INT(vm->regs[a], val);
                DBG_PRINT("  LOADI16 R%d <- %d\n", a, val);
                break;

            case OP_LOADNIL:
                a = bytecode[pc++];
                MRBZ_SET_NIL(vm->regs[a]);
                DBG_PRINT("  LOADNIL R%d\n", a);
                break;

            case OP_LOADT:
                a = bytecode[pc++];
                MRBZ_SET_TRUE(vm->regs[a]);
                DBG_PRINT("  LOADT R%d\n", a);
                break;

            case OP_LOADF:
                a = bytecode[pc++];
                MRBZ_SET_FALSE(vm->regs[a]);
                DBG_PRINT("  LOADF R%d\n", a);
                break;

            case OP_LOADSYM:
                a = bytecode[pc++];
                b = bytecode[pc++];
                MRBZ_SET_SYM(vm->regs[a], b);
                DBG_PRINT("  LOADSYM R%d <- sym%d\n", a, b);
                break;

            // Arithmetic operations
            case OP_ADD:
                a = bytecode[pc++];
                val = vm->regs[a].v.i + vm->regs[a+1].v.i;
                MRBZ_SET_INT(vm->regs[a], val);
                DBG_PRINT("  ADD R%d = %d\n", a, val);
                break;

            case OP_ADDI:
                a = bytecode[pc++];
                b = bytecode[pc++];
                val = vm->regs[a].v.i + (int16_t)b;
                MRBZ_SET_INT(vm->regs[a], val);
                DBG_PRINT("  ADDI R%d += %d = %d\n", a, b, val);
                break;

            case OP_SUB:
                a = bytecode[pc++];
                val = vm->regs[a].v.i - vm->regs[a+1].v.i;
                MRBZ_SET_INT(vm->regs[a], val);
                DBG_PRINT("  SUB R%d = %d\n", a, val);
                break;

            case OP_SUBI:
                a = bytecode[pc++];
                b = bytecode[pc++];
                val = vm->regs[a].v.i - (int16_t)b;
                MRBZ_SET_INT(vm->regs[a], val);
                DBG_PRINT("  SUBI R%d -= %d = %d\n", a, b, val);
                break;

            case OP_MUL:
                a = bytecode[pc++];
                val = vm->regs[a].v.i * vm->regs[a+1].v.i;
                MRBZ_SET_INT(vm->regs[a], val);
                DBG_PRINT("  MUL R%d = %d\n", a, val);
                break;

            case OP_DIV:
                a = bytecode[pc++];
                if (vm->regs[a+1].v.i == 0) {
                    printf("ERR: div/0\n");
                    MRBZ_SET_INT(vm->regs[a], 0);
                } else {
                    val = vm->regs[a].v.i / vm->regs[a+1].v.i;
                    MRBZ_SET_INT(vm->regs[a], val);
                }
                DBG_PRINT("  DIV R%d\n", a);
                break;

            // Comparison operations
            case OP_EQ:
                a = bytecode[pc++];
                if (values_equal(vm->regs[a], vm->regs[a+1])) {
                    MRBZ_SET_TRUE(vm->regs[a]);
                } else {
                    MRBZ_SET_FALSE(vm->regs[a]);
                }
                DBG_PRINT("  EQ R%d\n", a);
                break;

            case OP_LT:
                a = bytecode[pc++];
                if (vm->regs[a].v.i < vm->regs[a+1].v.i) {
                    MRBZ_SET_TRUE(vm->regs[a]);
                } else {
                    MRBZ_SET_FALSE(vm->regs[a]);
                }
                DBG_PRINT("  LT R%d\n", a);
                break;

            case OP_LE:
                a = bytecode[pc++];
                if (vm->regs[a].v.i <= vm->regs[a+1].v.i) {
                    MRBZ_SET_TRUE(vm->regs[a]);
                } else {
                    MRBZ_SET_FALSE(vm->regs[a]);
                }
                DBG_PRINT("  LE R%d\n", a);
                break;

            case OP_GT:
                a = bytecode[pc++];
                if (vm->regs[a].v.i > vm->regs[a+1].v.i) {
                    MRBZ_SET_TRUE(vm->regs[a]);
                } else {
                    MRBZ_SET_FALSE(vm->regs[a]);
                }
                DBG_PRINT("  GT R%d\n", a);
                break;

            case OP_GE:
                a = bytecode[pc++];
                if (vm->regs[a].v.i >= vm->regs[a+1].v.i) {
                    MRBZ_SET_TRUE(vm->regs[a]);
                } else {
                    MRBZ_SET_FALSE(vm->regs[a]);
                }
                DBG_PRINT("  GE R%d\n", a);
                break;

            // Jump operations
            case OP_JMP:
                offset = (int16_t)read_u16(bytecode + pc);
                pc += 2;
                pc = pc + offset;
                DBG_PRINT("  JMP to %d\n", pc);
                break;

            case OP_JMPIF:
                a = bytecode[pc++];
                offset = (int16_t)read_u16(bytecode + pc);
                pc += 2;
                if (MRBZ_TRUTHY(vm->regs[a])) {
                    pc = pc + offset;
                    DBG_PRINT("  JMPIF taken -> %d\n", pc);
                } else {
                    DBG_PRINT("  JMPIF not taken\n");
                }
                break;

            case OP_JMPNOT:
                a = bytecode[pc++];
                offset = (int16_t)read_u16(bytecode + pc);
                pc += 2;
                if (!MRBZ_TRUTHY(vm->regs[a])) {
                    pc = pc + offset;
                    DBG_PRINT("  JMPNOT taken -> %d\n", pc);
                } else {
                    DBG_PRINT("  JMPNOT not taken\n");
                }
                break;

            case OP_JMPNIL:
                a = bytecode[pc++];
                offset = (int16_t)read_u16(bytecode + pc);
                pc += 2;
                if (vm->regs[a].type == MRBZ_T_NIL) {
                    pc = pc + offset;
                }
                DBG_PRINT("  JMPNIL R%d\n", a);
                break;

            // Array operations
            case OP_ARRAY:
                a = bytecode[pc++];
                b = bytecode[pc++];
                arr_idx = alloc_array(vm);
                for (c = 0; c <= b; c++) {
                    vm->arrays[arr_idx][c] = vm->regs[a + c];
                }
                vm->array_lens[arr_idx] = b + 1;
                MRBZ_SET_ARR(vm->regs[a], arr_idx);
                DBG_PRINT("  ARRAY R%d = [%d elems]\n", a, b+1);
                break;

            case OP_AREF:
                a = bytecode[pc++];
                b = bytecode[pc++];
                c = bytecode[pc++];
                if (vm->regs[b].type == MRBZ_T_ARRAY) {
                    arr_idx = vm->regs[b].v.arr;
                    if (c < vm->array_lens[arr_idx]) {
                        vm->regs[a] = vm->arrays[arr_idx][c];
                    } else {
                        MRBZ_SET_NIL(vm->regs[a]);
                    }
                } else {
                    MRBZ_SET_NIL(vm->regs[a]);
                }
                DBG_PRINT("  AREF R%d = R%d[%d]\n", a, b, c);
                break;

            case OP_ASET:
                a = bytecode[pc++];
                b = bytecode[pc++];
                c = bytecode[pc++];
                if (vm->regs[b].type == MRBZ_T_ARRAY) {
                    arr_idx = vm->regs[b].v.arr;
                    if (c < MRBZ_MAX_ARRAY_LEN) {
                        vm->arrays[arr_idx][c] = vm->regs[a];
                        if (c >= vm->array_lens[arr_idx]) {
                            vm->array_lens[arr_idx] = c + 1;
                        }
                    }
                }
                DBG_PRINT("  ASET R%d[%d] = R%d\n", b, c, a);
                break;

            case OP_GETIDX:
                a = bytecode[pc++];
                if (vm->regs[a].type == MRBZ_T_ARRAY && vm->regs[a+1].v.i >= 0) {
                    arr_idx = vm->regs[a].v.arr;
                    c = (uint8_t)vm->regs[a+1].v.i;
                    if (c < vm->array_lens[arr_idx]) {
                        vm->regs[a] = vm->arrays[arr_idx][c];
                    } else {
                        MRBZ_SET_NIL(vm->regs[a]);
                    }
                } else {
                    MRBZ_SET_NIL(vm->regs[a]);
                }
                DBG_PRINT("  GETIDX R%d\n", a);
                break;

            case OP_SETIDX:
                a = bytecode[pc++];
                if (vm->regs[a].type == MRBZ_T_ARRAY) {
                    arr_idx = vm->regs[a].v.arr;
                    c = (uint8_t)vm->regs[a+1].v.i;
                    if (c < MRBZ_MAX_ARRAY_LEN) {
                        vm->arrays[arr_idx][c] = vm->regs[a+2];
                        if (c >= vm->array_lens[arr_idx]) {
                            vm->array_lens[arr_idx] = c + 1;
                        }
                    }
                }
                DBG_PRINT("  SETIDX R%d\n", a);
                break;

            // Method call - dispatch to built-ins
            case OP_SSEND:
                a = bytecode[pc++];
                b = bytecode[pc++];
                c = bytecode[pc++];
                mrbz_builtin_call(vm, b, c & 0x0F, a + 1, &vm->regs[a]);
                DBG_PRINT("  SSEND R%d = builtin[%d]\n", a, b);
                break;

            case OP_SEND:
                a = bytecode[pc++];
                b = bytecode[pc++];
                c = bytecode[pc++];
                mrbz_builtin_call(vm, b, c & 0x0F, a + 1, &vm->regs[a]);
                DBG_PRINT("  SEND R%d = builtin[%d]\n", a, b);
                break;

            // Instance variables
            case OP_GETIV:
                a = bytecode[pc++];
                b = bytecode[pc++];
                MRBZ_SET_NIL(vm->regs[a]);
                for (c = 0; c < vm->ivar_count; c++) {
                    if (vm->ivar_syms[c] == b) {
                        vm->regs[a] = vm->ivars[c];
                        break;
                    }
                }
                DBG_PRINT("  GETIV R%d = @%d\n", a, b);
                break;

            case OP_SETIV:
                a = bytecode[pc++];
                b = bytecode[pc++];
                c = 0;
                for (c = 0; c < vm->ivar_count; c++) {
                    if (vm->ivar_syms[c] == b) {
                        vm->ivars[c] = vm->regs[a];
                        break;
                    }
                }
                if (c == vm->ivar_count && vm->ivar_count < MRBZ_MAX_IVARS) {
                    vm->ivar_syms[vm->ivar_count] = b;
                    vm->ivars[vm->ivar_count] = vm->regs[a];
                    vm->ivar_count++;
                }
                DBG_PRINT("  SETIV @%d = R%d\n", b, a);
                break;

            // Constants
            case OP_GETCONST:
                a = bytecode[pc++];
                b = bytecode[pc++];
                MRBZ_SET_NIL(vm->regs[a]);
                for (c = 0; c < vm->const_count; c++) {
                    if (vm->const_syms[c] == b) {
                        vm->regs[a] = vm->consts[c];
                        break;
                    }
                }
                DBG_PRINT("  GETCONST R%d = C%d\n", a, b);
                break;

            case OP_SETCONST:
                a = bytecode[pc++];
                b = bytecode[pc++];
                c = 0;
                for (c = 0; c < vm->const_count; c++) {
                    if (vm->const_syms[c] == b) {
                        vm->consts[c] = vm->regs[a];
                        break;
                    }
                }
                if (c == vm->const_count && vm->const_count < MRBZ_MAX_CONSTS) {
                    vm->const_syms[vm->const_count] = b;
                    vm->consts[vm->const_count] = vm->regs[a];
                    vm->const_count++;
                }
                DBG_PRINT("  SETCONST C%d = R%d\n", b, a);
                break;

            // Return
            case OP_RETURN:
                a = bytecode[pc++];
                *result = vm->regs[a];
                vm->running = 0;
                DBG_PRINT("  RETURN R%d\n", a);
                break;

            case OP_STOP:
                MRBZ_SET_NIL(*result);
                vm->running = 0;
                DBG_PRINT("  STOP\n");
                break;

            // Argument setup - skip for now
            case OP_ENTER:
                pc += 3;
                DBG_PRINT("  ENTER\n");
                break;

            // Load self - return nil
            case OP_LOADSELF:
                a = bytecode[pc++];
                MRBZ_SET_NIL(vm->regs[a]);
                DBG_PRINT("  LOADSELF R%d\n", a);
                break;

            default:
                printf("UNK OP: 0x%02X\n", op);
                vm->running = 0;
                MRBZ_SET_NIL(*result);
                break;
        }
    }
}
