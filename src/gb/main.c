/**
 * mrbz - Minimal Ruby for Game Boy
 * Main entry point
 */

#include <gb/gb.h>
#include <stdio.h>
#include "../mrbz/vm.h"
#include "platform.h"

// External declarations
extern void load_game_tiles(void);

// Include appropriate bytecode based on build target
#include "../game/snake.ruby.c"
#define GAME_BYTECODE snake_bytecode
#define GAME_NAME "Snake"

void main(void) {
    // Initialize display
    DISPLAY_ON;
    SHOW_BKG;

    // Load tile graphics
    load_game_tiles();

    // Initialize and run VM
    mrbz_vm vm;
    mrbz_value result;

    mrbz_vm_init(&vm);

    // Clear screen before starting game
    for (uint8_t y = 0; y < 18; y++) {
        for (uint8_t x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, 128);  // TILE_EMPTY
        }
    }

    mrbz_vm_run(&vm, &result, GAME_BYTECODE);

    // VM should not return for snake (game_over halts)
    // Wait forever as fallback
    while (1) {
        wait_vbl_done();
    }
}
