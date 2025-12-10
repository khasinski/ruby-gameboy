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

// Initialize platform
void gb_platform_init(void);

// Built-in functions (called from builtins.c)
// Using output parameter instead of return value for SDCC compatibility
void gb_read_joypad(mrbz_vm* vm, mrbz_value* ret);
void gb_draw_tile(mrbz_vm* vm, int16_t x, int16_t y, int16_t tile, mrbz_value* ret);
void gb_clear_tile(mrbz_vm* vm, int16_t x, int16_t y, mrbz_value* ret);
void gb_wait_vbl(mrbz_vm* vm, mrbz_value* ret);
void gb_rand(mrbz_vm* vm, int16_t max, mrbz_value* ret);
void gb_game_over(mrbz_vm* vm, int16_t score, mrbz_value* ret);

#endif // MRBZ_PLATFORM_H
