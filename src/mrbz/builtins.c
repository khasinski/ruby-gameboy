/**
 * mrbz - Minimal Ruby for Game Boy
 * Built-in function implementations
 */

#include "vm.h"
#include <stdio.h>

// Forward declarations from platform layer (using output params for SDCC)
extern void gb_read_joypad(mrbz_vm* vm, mrbz_value* ret);
extern void gb_draw_tile(mrbz_vm* vm, int16_t x, int16_t y, int16_t tile, mrbz_value* ret);
extern void gb_clear_tile(mrbz_vm* vm, int16_t x, int16_t y, mrbz_value* ret);
extern void gb_wait_vbl(mrbz_vm* vm, mrbz_value* ret);
extern void gb_rand(mrbz_vm* vm, int16_t max, mrbz_value* ret);
extern void gb_game_over(mrbz_vm* vm, int16_t score, mrbz_value* ret);

// Simple string comparison (SDCC-compatible)
static uint8_t str_eq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

// Dispatch built-in calls based on symbol name
void mrbz_builtin_call(mrbz_vm* vm, uint8_t sym_idx, uint8_t argc, uint8_t base_reg, mrbz_value* ret) {
    int16_t x, y, tile, score, max, size, i;
    mrbz_value default_val;
    uint8_t arr_idx;
    const char* name;

    // Initialize return value to nil
    MRBZ_SET_NIL(*ret);

    // Get symbol name
    name = mrbz_get_symbol(vm, sym_idx);
    if (name == 0) {
        printf("No sym %d\n", sym_idx);
        return;
    }

    // Match built-in function by name
    if (str_eq(name, "read_joypad")) {
        gb_read_joypad(vm, ret);
    }
    else if (str_eq(name, "draw_tile")) {
        if (argc >= 3) {
            x = vm->regs[base_reg].v.i;
            y = vm->regs[base_reg + 1].v.i;
            tile = vm->regs[base_reg + 2].v.i;
            gb_draw_tile(vm, x, y, tile, ret);
        }
    }
    else if (str_eq(name, "clear_tile")) {
        if (argc >= 2) {
            x = vm->regs[base_reg].v.i;
            y = vm->regs[base_reg + 1].v.i;
            gb_clear_tile(vm, x, y, ret);
        }
    }
    else if (str_eq(name, "wait_vbl")) {
        gb_wait_vbl(vm, ret);
    }
    else if (str_eq(name, "rand")) {
        if (argc >= 1) {
            max = vm->regs[base_reg].v.i;
            gb_rand(vm, max, ret);
        }
    }
    else if (str_eq(name, "game_over")) {
        if (argc >= 1) {
            score = vm->regs[base_reg].v.i;
            gb_game_over(vm, score, ret);
        } else {
            gb_game_over(vm, 0, ret);
        }
    }
    else if (str_eq(name, "puts") || str_eq(name, "p")) {
        if (argc >= 1) {
            printf("puts: ");
            mrbz_print_value(vm->regs[base_reg]);
            printf("\n");
        }
    }
    else if (str_eq(name, "new")) {
        // Array.new(size, default) - called on Array class
        if (argc >= 2) {
            size = vm->regs[base_reg].v.i;
            default_val = vm->regs[base_reg + 1];

            if (size > MRBZ_MAX_ARRAY_LEN) size = MRBZ_MAX_ARRAY_LEN;
            if (size < 0) size = 0;

            if (vm->next_array < MRBZ_MAX_ARRAYS) {
                arr_idx = vm->next_array;
                vm->next_array++;
                vm->array_lens[arr_idx] = (uint8_t)size;
                for (i = 0; i < size; i++) {
                    vm->arrays[arr_idx][i] = default_val;
                }
                MRBZ_SET_ARR(*ret, arr_idx);
            }
        }
    }
    else if (str_eq(name, "!=")) {
        // Not-equal comparison
        if (argc >= 1) {
            if (vm->regs[base_reg - 1].type == vm->regs[base_reg].type) {
                if (vm->regs[base_reg - 1].v.i != vm->regs[base_reg].v.i) {
                    MRBZ_SET_TRUE(*ret);
                } else {
                    MRBZ_SET_FALSE(*ret);
                }
            } else {
                MRBZ_SET_TRUE(*ret);
            }
        }
    }
    else {
        // Unknown - not an error, might be a user-defined method
        printf("UNK: %s\n", name);
    }
}
