/**
 * mrbz - Native test harness
 * Runs VM on desktop for easier debugging
 */

#include <stdio.h>
#include <stdint.h>
#include "../mrbz/vm.h"

#ifdef USE_SNAKE
#include "../game/snake.ruby.c"
#define GAME_BYTECODE snake_bytecode
#else
#include "../game/test.ruby.c"
#define GAME_BYTECODE test_bytecode
#endif

// Stub implementations for platform functions
void gb_read_joypad(mrbz_vm* vm, mrbz_value* ret) {
    (void)vm;
    MRBZ_SET_INT(*ret, 0);
}

void gb_draw_tile(mrbz_vm* vm, int16_t x, int16_t y, int16_t tile, mrbz_value* ret) {
    (void)vm;
    printf("[draw_tile %d,%d tile=%d]\n", x, y, tile);
    MRBZ_SET_NIL(*ret);
}

void gb_clear_tile(mrbz_vm* vm, int16_t x, int16_t y, mrbz_value* ret) {
    (void)vm;
    printf("[clear_tile %d,%d]\n", x, y);
    MRBZ_SET_NIL(*ret);
}

void gb_wait_vbl(mrbz_vm* vm, mrbz_value* ret) {
    (void)vm;
    MRBZ_SET_NIL(*ret);
}

void gb_rand(mrbz_vm* vm, int16_t max, mrbz_value* ret) {
    (void)vm;
    MRBZ_SET_INT(*ret, max > 0 ? 0 : 0);
}

void gb_game_over(mrbz_vm* vm, int16_t score, mrbz_value* ret) {
    (void)vm;
    printf("[game_over score=%d]\n", score);
    MRBZ_SET_NIL(*ret);
}

int main(void) {
    mrbz_vm vm;
    mrbz_value result;

    printf("mrbz Native Test\n");
    printf("================\n\n");

    printf("Bytecode size: %zu bytes\n", sizeof(GAME_BYTECODE));
    printf("First 8 bytes: ");
    for (int i = 0; i < 8; i++) {
        printf("%02X ", GAME_BYTECODE[i]);
    }
    printf("(%.4s)\n\n", GAME_BYTECODE);

    mrbz_vm_init(&vm);

    printf("Running VM...\n");
    printf("-------------\n");
    mrbz_vm_run(&vm, &result, GAME_BYTECODE);

    printf("\n-------------\n");
    printf("Result: ");
    mrbz_print_value(result);
    printf("\n");

#ifdef USE_SNAKE
    printf("\n*** Snake game ended ***\n");
    return 0;
#else
    if (result.type == MRBZ_T_INT && result.v.i == 31) {
        printf("\n*** SUCCESS: Got expected value 31! ***\n");
        return 0;
    } else {
        printf("\n*** FAIL: Expected 31, got ");
        mrbz_print_value(result);
        printf(" ***\n");
        return 1;
    }
#endif
}
