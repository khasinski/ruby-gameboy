/**
 * mrbz - Minimal Ruby for Game Boy
 * Platform abstraction layer header
 */

#ifndef MRBZ_PLATFORM_H
#define MRBZ_PLATFORM_H

#include "../mrbz/vm.h"

// Joypad direction symbols (must match symbol pool order)
#define SYM_NONE  0
#define SYM_UP    1
#define SYM_DOWN  2
#define SYM_LEFT  3
#define SYM_RIGHT 4

// Game tiles start at offset 128 to preserve font tiles for printf
#define TILE_OFFSET 128

// Tile IDs (add TILE_OFFSET when using)
#define TILE_EMPTY (TILE_OFFSET + 0)
#define TILE_HEAD  (TILE_OFFSET + 1)
#define TILE_BODY  (TILE_OFFSET + 2)
#define TILE_FOOD  (TILE_OFFSET + 3)

// Tile for solid fill (wireframe drawing)
#define TILE_FILL  (TILE_OFFSET + 14)

// Pixel framebuffer constants (128x120 px = 16x15 tiles)
#define FB_TILES_X   16
#define FB_TILES_Y   15
#define FB_NUM_TILES (FB_TILES_X * FB_TILES_Y)  // 240
#define FB_BORDER_TILE FB_NUM_TILES             // tile 240 = always blank
#define FB_WIDTH     (FB_TILES_X * 8)           // 128
#define FB_HEIGHT    (FB_TILES_Y * 8)           // 120
#define FB_OFFSET_X  2   // center 128px in 160px wide screen
#define FB_OFFSET_Y  1   // center 120px in 144px high screen

// Built-in functions (called from builtins.c)
// Using output parameter instead of return value for SDCC compatibility
void gb_read_joypad(mrbz_vm* vm, mrbz_value* ret);
void gb_draw_tile(mrbz_vm* vm, int16_t x, int16_t y, int16_t tile, mrbz_value* ret);
void gb_clear_tile(mrbz_vm* vm, int16_t x, int16_t y, mrbz_value* ret);
void gb_wait_vbl(mrbz_vm* vm, mrbz_value* ret);
void gb_rand(mrbz_vm* vm, int16_t max, mrbz_value* ret);
void gb_game_over(mrbz_vm* vm, int16_t score, mrbz_value* ret);
void gb_sin_lut(mrbz_vm* vm, int16_t angle, mrbz_value* ret);
void gb_cos_lut(mrbz_vm* vm, int16_t angle, mrbz_value* ret);
void gb_draw_line(mrbz_vm* vm, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t tile, mrbz_value* ret);
void gb_clear_screen(mrbz_vm* vm, mrbz_value* ret);

// Shadow buffer rendering (triangle-based 3D)
void gb_shadow_clear(mrbz_vm* vm, mrbz_value* ret);
void gb_shadow_flush(mrbz_vm* vm, mrbz_value* ret);
void gb_fill_triangle(mrbz_vm* vm, int16_t x0, int16_t y0,
                       int16_t x1, int16_t y1,
                       int16_t x2, int16_t y2,
                       int16_t shade, mrbz_value* ret);
// All-in-one tile-based render
void gb_render_faces(mrbz_vm* vm, uint8_t px_arr, uint8_t py_arr,
                      uint8_t fa_arr, uint8_t fb_arr, uint8_t fc_arr,
                      int16_t num_faces, mrbz_value* ret);

// Pixel-resolution: rotate, project, cull, shade, fill, copy to VRAM
void fb_init(void);
void gb_update_and_render(mrbz_vm* vm, int16_t angle_x, int16_t angle_y, mrbz_value* ret);

#ifdef CGB
// Game Boy Color support
void gb_init_cgb_palettes(void);
#endif

#endif // MRBZ_PLATFORM_H
