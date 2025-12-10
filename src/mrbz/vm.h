/**
 * mrbz - Minimal Ruby for Game Boy
 * Virtual Machine header
 */

#ifndef MRBZ_VM_H
#define MRBZ_VM_H

#include <stdint.h>

// Configuration
#define MRBZ_MAX_REGS      32    // Number of registers
#define MRBZ_MAX_ARRAYS    8     // Number of pre-allocated arrays
#define MRBZ_MAX_ARRAY_LEN 100   // Maximum array length
#define MRBZ_MAX_SYMBOLS   32    // Maximum symbols in pool
#define MRBZ_MAX_CONSTS    16    // Maximum constants
#define MRBZ_MAX_IVARS     16    // Maximum instance variables

// Value types
typedef enum {
    MRBZ_T_NIL = 0,
    MRBZ_T_FALSE,
    MRBZ_T_TRUE,
    MRBZ_T_INT,
    MRBZ_T_SYMBOL,
    MRBZ_T_ARRAY
} mrbz_type;

// A value in the VM
typedef struct {
    uint8_t type;
    union {
        int16_t i;          // Integer value
        uint8_t sym;        // Symbol index
        uint8_t arr;        // Array index
    } v;
} mrbz_value;

// Helper macros using statement expressions (for SDCC compatibility)
// These set a destination variable directly

#define MRBZ_SET_NIL(dest) do { \
    (dest).type = MRBZ_T_NIL; \
    (dest).v.i = 0; \
} while(0)

#define MRBZ_SET_TRUE(dest) do { \
    (dest).type = MRBZ_T_TRUE; \
    (dest).v.i = 1; \
} while(0)

#define MRBZ_SET_FALSE(dest) do { \
    (dest).type = MRBZ_T_FALSE; \
    (dest).v.i = 0; \
} while(0)

#define MRBZ_SET_INT(dest, n) do { \
    (dest).type = MRBZ_T_INT; \
    (dest).v.i = (n); \
} while(0)

#define MRBZ_SET_SYM(dest, n) do { \
    (dest).type = MRBZ_T_SYMBOL; \
    (dest).v.sym = (n); \
} while(0)

#define MRBZ_SET_ARR(dest, n) do { \
    (dest).type = MRBZ_T_ARRAY; \
    (dest).v.arr = (n); \
} while(0)

// Check if value is truthy (not nil or false)
#define MRBZ_TRUTHY(v)  ((v).type != MRBZ_T_NIL && (v).type != MRBZ_T_FALSE)

// Virtual machine state
typedef struct {
    // Registers
    mrbz_value regs[MRBZ_MAX_REGS];

    // Array pool (static allocation)
    mrbz_value arrays[MRBZ_MAX_ARRAYS][MRBZ_MAX_ARRAY_LEN];
    uint8_t array_lens[MRBZ_MAX_ARRAYS];
    uint8_t next_array;

    // Symbol table - pointers into bytecode
    const char* sym_names[MRBZ_MAX_SYMBOLS];
    uint8_t sym_count;

    // Instance variables (for @variables)
    uint8_t ivar_syms[MRBZ_MAX_IVARS];   // Symbol index for each ivar
    mrbz_value ivars[MRBZ_MAX_IVARS];    // Values
    uint8_t ivar_count;

    // Constants (for CONST_NAME)
    uint8_t const_syms[MRBZ_MAX_CONSTS]; // Symbol index for each constant
    mrbz_value consts[MRBZ_MAX_CONSTS];  // Values
    uint8_t const_count;

    // Bytecode pointer (for symbol table access)
    const uint8_t* bytecode;

    // Running state
    uint8_t running;
} mrbz_vm;

// Get symbol name by index
const char* mrbz_get_symbol(mrbz_vm* vm, uint8_t idx);

// Initialize a VM
void mrbz_vm_init(mrbz_vm* vm);

// Run bytecode and get result
void mrbz_vm_run(mrbz_vm* vm, mrbz_value* result, const uint8_t* bytecode);

// Debug: print a value
void mrbz_print_value(mrbz_value v);

#endif // MRBZ_VM_H
