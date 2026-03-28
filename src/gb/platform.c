/**
 * mrbz - Minimal Ruby for Game Boy
 * Platform-specific implementations
 */

#include <gb/gb.h>
#include <stdio.h>
#ifdef CGB
#include <gb/cgb.h>
#endif
#include "platform.h"
#include "../mrbz/vm.h"

// Random seed
static uint16_t rand_seed = 12345;

#ifdef CGB
// Palette assignment per game tile ID (index = tile - TILE_OFFSET)
// 0=empty, 1=head, 2=body, 3=food, 4-13=digits
static const uint8_t tile_palette_map[] = {
    0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};
static void gb_set_tile_palette(uint8_t x, uint8_t y, uint8_t palette);
#endif

// Read joypad and return direction as symbol
void gb_read_joypad(mrbz_vm* vm, mrbz_value* ret) {
    uint8_t j;
    uint8_t sym_idx;

    j = joypad();

    if (j & J_UP) {
        sym_idx = mrbz_find_symbol(vm, "up");
        MRBZ_SET_SYM(*ret, sym_idx);
    } else if (j & J_DOWN) {
        sym_idx = mrbz_find_symbol(vm, "down");
        MRBZ_SET_SYM(*ret, sym_idx);
    } else if (j & J_LEFT) {
        sym_idx = mrbz_find_symbol(vm, "left");
        MRBZ_SET_SYM(*ret, sym_idx);
    } else if (j & J_RIGHT) {
        sym_idx = mrbz_find_symbol(vm, "right");
        MRBZ_SET_SYM(*ret, sym_idx);
    } else {
        MRBZ_SET_NIL(*ret);
    }
}

// Draw a tile at x,y position
void gb_draw_tile(mrbz_vm* vm, int16_t x, int16_t y, int16_t tile, mrbz_value* ret) {
    (void)vm;

    if (x >= 0 && x < 20 && y >= 0 && y < 18) {
        set_bkg_tile_xy(x, y, tile);
#ifdef CGB
        if (tile >= TILE_OFFSET && tile < TILE_OFFSET + 14) {
            gb_set_tile_palette(x, y, tile_palette_map[tile - TILE_OFFSET]);
        }
#endif
    }

    MRBZ_SET_NIL(*ret);
}

// Clear a tile (set to empty)
void gb_clear_tile(mrbz_vm* vm, int16_t x, int16_t y, mrbz_value* ret) {
    (void)vm;
    if (x >= 0 && x < 20 && y >= 0 && y < 18) {
        set_bkg_tile_xy(x, y, TILE_EMPTY);
#ifdef CGB
        gb_set_tile_palette(x, y, 0);
#endif
    }
    MRBZ_SET_NIL(*ret);
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

    rand_seed = rand_seed * 25173 + 13849;
    MRBZ_SET_INT(*ret, rand_seed % max);
}

// Game over - display score, wait for restart
void gb_game_over(mrbz_vm* vm, int16_t score, mrbz_value* ret) {
    uint8_t x, y;

    (void)vm;

#ifdef CGB
    VBK_REG = 1;
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, 0);
        }
    }
    VBK_REG = 0;
#endif
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, 0);
        }
    }

    printf("\n\n\n   GAME OVER\n\n   Score: %d\n\n  Press any button", score);

    // Wait for all buttons released, then wait for a press
    while (joypad()) { wait_vbl_done(); }
    while (!joypad()) { wait_vbl_done(); }

    // Clear screen for restart
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, TILE_EMPTY);
        }
    }

    MRBZ_SET_NIL(*ret);
}

// --- Sound ---

// Pentatonic scale frequencies (C D E G A across 4 octaves = 20 notes)
static const uint16_t note_freqs[] = {
    1548, 1602, 1651, 1714, 1750,  // C4 D4 E4 G4 A4
    1797, 1825, 1849, 1881, 1899,  // C5 D5 E5 G5 A5
    1923, 1936, 1949, 1964, 1974,  // C6 D6 E6 G6 A6
    1987, 1993, 1999, 2006, 2011   // C7 D7 E7 G7 A7
};

void gb_play_note(mrbz_vm* vm, int16_t note, mrbz_value* ret) {
    uint16_t freq;
    (void)vm;

    if (note < 0) note = 0;
    if (note > 19) note = 19;

    freq = note_freqs[note];

    // Enable sound if not already
    NR52_REG = 0x80;
    NR51_REG = 0xFF;
    NR50_REG = 0x77;

    // Channel 1: square wave
    NR10_REG = 0x00;        // no sweep
    NR11_REG = 0x80;        // 50% duty
    NR12_REG = 0xF1;        // volume 15, decay 1
    NR13_REG = freq & 0xFF;
    NR14_REG = 0x80 | ((freq >> 8) & 0x07);  // trigger

    MRBZ_SET_NIL(*ret);
}

#ifdef CGB

// CGB palette colors (RGB555 format)
static const uint16_t cgb_bg_palettes[] = {
    // Palette 0: Background (soft green field)
    RGB(27, 31, 22), RGB(20, 26, 16), RGB(14, 20, 10), RGB(6, 10, 4),
    // Palette 1: Snake head (bright green)
    RGB(27, 31, 22), RGB(4, 28, 4),  RGB(2, 20, 2),  RGB(0, 10, 0),
    // Palette 2: Snake body (darker green)
    RGB(27, 31, 22), RGB(6, 24, 6),  RGB(2, 16, 2),  RGB(0, 8, 0),
    // Palette 3: Food (red)
    RGB(27, 31, 22), RGB(31, 8, 8),  RGB(24, 2, 2),  RGB(16, 0, 0),
    // Palette 4: Score digits (white on dark)
    RGB(27, 31, 22), RGB(31, 31, 31), RGB(20, 20, 20), RGB(6, 6, 6),
};

void gb_init_cgb_palettes(void) {
    set_bkg_palette(0, 5, cgb_bg_palettes);
}

static void gb_set_tile_palette(uint8_t x, uint8_t y, uint8_t palette) {
    VBK_REG = 1;
    set_bkg_tile_xy(x, y, palette & 0x07);
    VBK_REG = 0;
}

#endif
