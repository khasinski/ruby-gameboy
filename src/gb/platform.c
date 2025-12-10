/**
 * mrbz - Minimal Ruby for Game Boy
 * Platform-specific implementations
 */

#include <gb/gb.h>
#include <stdio.h>
#include "platform.h"
#include "../mrbz/vm.h"

// Random seed
static uint16_t rand_seed = 12345;

// Initialize platform
void gb_platform_init(void) {
    // Additional init can go here
}

// Read joypad and return direction as integer
// Returns: 0=none, 1=up, 2=down, 3=left, 4=right
void gb_read_joypad(mrbz_vm* vm, mrbz_value* ret) {
    uint8_t j;

    (void)vm; // unused

    j = joypad();

    // Priority: up > down > left > right
    if (j & J_UP) {
        MRBZ_SET_INT(*ret, 1);
    } else if (j & J_DOWN) {
        MRBZ_SET_INT(*ret, 2);
    } else if (j & J_LEFT) {
        MRBZ_SET_INT(*ret, 3);
    } else if (j & J_RIGHT) {
        MRBZ_SET_INT(*ret, 4);
    } else {
        MRBZ_SET_INT(*ret, 0);
    }
}

// Draw a tile at x,y position
void gb_draw_tile(mrbz_vm* vm, int16_t x, int16_t y, int16_t tile, mrbz_value* ret) {
    (void)vm;

    if (x >= 0 && x < 20 && y >= 0 && y < 18) {
        set_bkg_tile_xy(x, y, tile);
    }

    MRBZ_SET_NIL(*ret);
}

// Clear a tile (set to empty)
void gb_clear_tile(mrbz_vm* vm, int16_t x, int16_t y, mrbz_value* ret) {
    gb_draw_tile(vm, x, y, TILE_EMPTY, ret);
}

// Wait for vertical blank
void gb_wait_vbl(mrbz_vm* vm, mrbz_value* ret) {
    (void)vm;
    wait_vbl_done();
    MRBZ_SET_NIL(*ret);
}

// Simple LCG random number generator
void gb_rand(mrbz_vm* vm, int16_t max, mrbz_value* ret) {
    (void)vm;

    if (max <= 0) {
        MRBZ_SET_INT(*ret, 0);
        return;
    }

    // LCG: seed = seed * 25173 + 13849
    rand_seed = rand_seed * 25173 + 13849;

    MRBZ_SET_INT(*ret, rand_seed % max);
}

// Game over - display score and halt
void gb_game_over(mrbz_vm* vm, int16_t score, mrbz_value* ret) {
    uint8_t x, y;

    (void)vm;

    // Clear screen
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, TILE_EMPTY);
        }
    }

    // Display "GAME OVER" using printf
    printf("\n\n\n   GAME OVER\n\n   Score: %d\n", score);

    MRBZ_SET_NIL(*ret);

    // Halt forever
    while (1) {
        wait_vbl_done();
    }
}
