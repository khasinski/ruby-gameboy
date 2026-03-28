/**
 * mrbz - Minimal Ruby for Game Boy
 * Main entry point
 */

#include <gb/gb.h>
#ifdef CGB
#include <gb/cgb.h>
#endif
#include <stdio.h>
#include "../mrbz/vm.h"
#include "platform.h"

// External declarations
extern void load_game_tiles(void);
#ifdef CGB
extern void gb_init_cgb_palettes(void);
#endif

#ifdef GAME_DEMO
#include "../game/demo.ruby.c"
#define GAME_BYTECODE demo_bytecode
#else
#include "../game/snake.ruby.c"
#define GAME_BYTECODE snake_bytecode
#endif

void main(void) {
    // Load tile graphics
    load_game_tiles();

#ifdef CGB
    gb_init_cgb_palettes();
#endif

    // Clear screen
    {
        uint8_t x, y;
        for (y = 0; y < 18; y++) {
            for (x = 0; x < 20; x++) {
                set_bkg_tile_xy(x, y, 128);
            }
        }
    }

    // Turn on display
    DISPLAY_ON;
    SHOW_BKG;

    // Initialize and run VM
    {
        mrbz_vm vm;
        mrbz_value result;

        mrbz_vm_init(&vm);
        mrbz_vm_run(&vm, &result, GAME_BYTECODE);
    }

    // Wait forever as fallback
    while (1) {
        wait_vbl_done();
    }
}
