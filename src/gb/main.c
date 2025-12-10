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
#ifdef USE_SNAKE
#include "../game/snake.ruby.c"
#define GAME_BYTECODE snake_bytecode
#define GAME_NAME "Snake"
#else
#include "../game/test.ruby.c"
#define GAME_BYTECODE test_bytecode
#define GAME_NAME "Test"
#endif

void main(void) {
    // Initialize display
    DISPLAY_ON;
    SHOW_BKG;

    // Load tile graphics
    load_game_tiles();

    // Initialize platform
    gb_platform_init();

    // Show startup message
    printf("mrbz VM\n");
    printf("%s\n\n", GAME_NAME);

    // Initialize and run VM
    mrbz_vm vm;
    mrbz_value result;

    mrbz_vm_init(&vm);

#ifdef USE_SNAKE
    // Clear screen before starting game (remove startup text)
    for (uint8_t y = 0; y < 18; y++) {
        for (uint8_t x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, 128);  // TILE_EMPTY
        }
    }
#endif

    mrbz_vm_run(&vm, &result, GAME_BYTECODE);

    // Show result
    printf("\nResult: ");
    mrbz_print_value(result);
    printf("\n");

    // Wait forever
    while (1) {
        wait_vbl_done();
    }
}
