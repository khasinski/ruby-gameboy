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
// 0=empty, 1=head, 2=body, 3=food, 4-13=digits, 14-16=gem shades
static const uint8_t tile_palette_map[] = {
    0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5
};
static void gb_set_tile_palette(uint8_t x, uint8_t y, uint8_t palette);
#endif

// Read joypad and return direction as symbol
// Returns: :up, :down, :left, :right, or nil for no input
void gb_read_joypad(mrbz_vm* vm, mrbz_value* ret) {
    uint8_t j;
    uint8_t sym_idx;

    j = joypad();

    // Priority: up > down > left > right
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
        // Auto-assign palette based on tile ID
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

    // LCG: seed = seed * 25173 + 13849
    rand_seed = rand_seed * 25173 + 13849;

    MRBZ_SET_INT(*ret, rand_seed % max);
}

// Game over - display score and halt
void gb_game_over(mrbz_vm* vm, int16_t score, mrbz_value* ret) {
    uint8_t x, y;

    (void)vm;

    // Clear screen with space character (0x00) for printf to work
#ifdef CGB
    // Reset all tile attributes to palette 0
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

    // Display "GAME OVER" using printf
    printf("\n\n\n   GAME OVER\n\n   Score: %d\n", score);

    MRBZ_SET_NIL(*ret);

    // Halt forever
    while (1) {
        wait_vbl_done();
    }
}

// Quarter-wave sine table (0-90 degrees in 64 steps), scaled by 64
static const int8_t sin_quarter[65] = {
     0,  2,  3,  5,  6,  8,  9, 11,
    12, 14, 16, 17, 19, 20, 22, 23,
    24, 26, 27, 29, 30, 32, 33, 34,
    36, 37, 38, 39, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52,
    53, 54, 55, 56, 56, 57, 58, 59,
    59, 60, 60, 61, 61, 62, 62, 62,
    63, 63, 63, 64, 64, 64, 64, 64,
    64
};

static int16_t sin_lookup(int16_t angle) {
    uint8_t a;
    uint8_t idx;

    a = (uint8_t)(angle & 0xFF);

    if (a < 64) {
        return sin_quarter[a];
    } else if (a < 128) {
        idx = 128 - a;
        return sin_quarter[idx];
    } else if (a < 192) {
        idx = a - 128;
        return -sin_quarter[idx];
    } else {
        idx = 0 - a;  /* 256 - a, using uint8 wrap */
        return -sin_quarter[idx];
    }
}

void gb_sin_lut(mrbz_vm* vm, int16_t angle, mrbz_value* ret) {
    (void)vm;
    MRBZ_SET_INT(*ret, sin_lookup(angle));
}

void gb_cos_lut(mrbz_vm* vm, int16_t angle, mrbz_value* ret) {
    (void)vm;
    MRBZ_SET_INT(*ret, sin_lookup(angle + 64));
}

// Bresenham line drawing
void gb_draw_line(mrbz_vm* vm, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t tile, mrbz_value* ret) {
    int16_t dx, dy, sx, sy, err, e2;
#ifdef CGB
    uint8_t pal = 0;
    if (tile >= TILE_OFFSET && tile < TILE_OFFSET + 15) {
        pal = tile_palette_map[tile - TILE_OFFSET];
    }
#endif

    (void)vm;

    dx = x1 - x0;
    if (dx < 0) dx = -dx;
    dy = y1 - y0;
    if (dy < 0) dy = -dy;
    dy = -dy;
    sx = x0 < x1 ? 1 : -1;
    sy = y0 < y1 ? 1 : -1;
    err = dx + dy;

    while (1) {
        if (x0 >= 0 && x0 < 20 && y0 >= 0 && y0 < 18) {
            set_bkg_tile_xy(x0, y0, tile);
#ifdef CGB
            gb_set_tile_palette(x0, y0, pal);
#endif
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }

    MRBZ_SET_NIL(*ret);
}

// Clear entire screen
void gb_clear_screen(mrbz_vm* vm, mrbz_value* ret) {
    uint8_t x, y;
    (void)vm;

    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, TILE_EMPTY);
        }
    }
#ifdef CGB
    VBK_REG = 1;
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, 0);
        }
    }
    VBK_REG = 0;
#endif

    MRBZ_SET_NIL(*ret);
}

#ifdef CGB

// CGB palette colors (RGB555 format)
// Each palette has 4 colors: background, light, dark, darkest
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
    // Palette 5: Ruby gem (3 shades of red)
    RGB(27, 31, 22), RGB(31, 14, 14), RGB(24, 4, 4), RGB(16, 2, 2),
};

void gb_init_cgb_palettes(void) {
    set_bkg_palette(0, 6, cgb_bg_palettes);
}

static void gb_set_tile_palette(uint8_t x, uint8_t y, uint8_t palette) {
    VBK_REG = 1;
    set_bkg_tile_xy(x, y, palette & 0x07);
    VBK_REG = 0;
}

#endif

// --- Shadow buffer rendering for 3D ---

static uint8_t shadow_tiles[360];  // 20x18 tile map
#ifdef CGB
static uint8_t shadow_attrs[360];  // 20x18 CGB palette attributes
#endif

// Shade tiles: color 3 (dark), color 2 (medium), color 1 (bright)
static const uint8_t shade_tiles[] = {
    TILE_OFFSET + 14,
    TILE_OFFSET + 15,
    TILE_OFFSET + 16,
};

void gb_shadow_clear(mrbz_vm* vm, mrbz_value* ret) {
    uint16_t i;
    (void)vm;
    for (i = 0; i < 360; i++) shadow_tiles[i] = TILE_EMPTY;
#ifdef CGB
    for (i = 0; i < 360; i++) shadow_attrs[i] = 0;
#endif
    MRBZ_SET_NIL(*ret);
}

void gb_shadow_flush(mrbz_vm* vm, mrbz_value* ret) {
    (void)vm;
    wait_vbl_done();
    set_bkg_tiles(0, 0, 20, 18, shadow_tiles);
#ifdef CGB
    VBK_REG = 1;
    set_bkg_tiles(0, 0, 20, 18, shadow_attrs);
    VBK_REG = 0;
#endif
    MRBZ_SET_NIL(*ret);
}

void gb_fill_triangle(mrbz_vm* vm, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       int16_t x2, int16_t y2, int16_t shade, mrbz_value* ret) {
    (void)vm; (void)x0; (void)y0; (void)x1; (void)y1; (void)x2; (void)y2; (void)shade;
    MRBZ_SET_NIL(*ret);
}

// Interpolate X along an edge at a given Y (pixel coordinates)
static int16_t edge_interp(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t y) {
    int16_t dy;
    dy = y1 - y0;
    if (dy == 0) return x0;
    return x0 + (x1 - x0) * (y - y0) / dy;
}

// Internal tile-level triangle fill into shadow buffer
static void fill_tri_internal(int16_t x0, int16_t y0,
                               int16_t x1, int16_t y1,
                               int16_t x2, int16_t y2,
                               uint8_t tile) {
    int16_t tmp;
    int16_t ty, ty_min, ty_max;
    int16_t scan_y, long_x, short_x, left_x, right_x, tx;
    uint16_t idx;

    if (y0 > y1) { tmp=x0; x0=x1; x1=tmp; tmp=y0; y0=y1; y1=tmp; }
    if (y0 > y2) { tmp=x0; x0=x2; x2=tmp; tmp=y0; y0=y2; y2=tmp; }
    if (y1 > y2) { tmp=x1; x1=x2; x2=tmp; tmp=y1; y1=y2; y2=tmp; }

    ty_min = y0 >> 3;
    ty_max = y2 >> 3;
    if (ty_min < 0) ty_min = 0;
    if (ty_max > 17) ty_max = 17;

    for (ty = ty_min; ty <= ty_max; ty++) {
        scan_y = ty * 8 + 4;
        if (scan_y < y0 || scan_y > y2) continue;

        long_x = edge_interp(x0, y0, x2, y2, scan_y);
        if (scan_y < y1) {
            short_x = edge_interp(x0, y0, x1, y1, scan_y);
        } else {
            short_x = edge_interp(x1, y1, x2, y2, scan_y);
        }

        if (long_x < short_x) {
            left_x = long_x >> 3;
            right_x = short_x >> 3;
        } else {
            left_x = short_x >> 3;
            right_x = long_x >> 3;
        }
        if (left_x < 0) left_x = 0;
        if (right_x > 19) right_x = 19;

        for (tx = left_x; tx <= right_x; tx++) {
            idx = ty * 20 + tx;
            shadow_tiles[idx] = tile;
#ifdef CGB
            shadow_attrs[idx] = 5;
#endif
        }
    }
}

// All-in-one: clear, cull+shade+fill all faces, flush to VRAM
void gb_render_faces(mrbz_vm* vm, uint8_t px_arr, uint8_t py_arr,
                      uint8_t fa_arr, uint8_t fb_arr, uint8_t fc_arr,
                      int16_t num_faces, mrbz_value* ret) {
    int16_t i, nz;
    uint8_t a, b, c;
    int16_t ax, ay, bx, by, cx, cy;
    uint16_t j;
    uint8_t tile;

    // Clear shadow buffer
    for (j = 0; j < 360; j++) shadow_tiles[j] = TILE_EMPTY;
#ifdef CGB
    for (j = 0; j < 360; j++) shadow_attrs[j] = 0;
#endif

    // Process each face: cull, shade, fill
    for (i = 0; i < num_faces; i++) {
        a = (uint8_t)vm->arrays[fa_arr][i].v.i;
        b = (uint8_t)vm->arrays[fb_arr][i].v.i;
        c = (uint8_t)vm->arrays[fc_arr][i].v.i;

        ax = vm->arrays[px_arr][a].v.i;
        ay = vm->arrays[py_arr][a].v.i;
        bx = vm->arrays[px_arr][b].v.i;
        by = vm->arrays[py_arr][b].v.i;
        cx = vm->arrays[px_arr][c].v.i;
        cy = vm->arrays[py_arr][c].v.i;

        // Backface culling (nz < 0 = front-facing)
        nz = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
        if (nz >= 0) continue;

        // Directional light from the right side
        {
            int16_t light = (ax + bx + cx) / 3 - 80;
            if (light > 10) {
                tile = shade_tiles[2];
            } else if (light > -10) {
                tile = shade_tiles[1];
            } else {
                tile = shade_tiles[0];
            }
        }

        fill_tri_internal(ax, ay, bx, by, cx, cy, tile);
    }

    // Flush to VRAM during VBlank (no LCD off - no flash)
    wait_vbl_done();
    set_bkg_tiles(0, 0, 20, 18, shadow_tiles);
#ifdef CGB
    VBK_REG = 1;
    set_bkg_tiles(0, 0, 20, 18, shadow_attrs);
    VBK_REG = 0;
#endif

    MRBZ_SET_NIL(*ret);
}

// --- Pixel-resolution 3D rendering (all in C) ---

#define GEM_VERTS 11
#define GEM_FACES 18

static const int16_t gem_vx[] = {  0, 16,  8, -8,-16,   0, 40, 24,-24,-40,  0 };
static const int16_t gem_vy[] = {-24,-24,-24,-24,-24,   0,  0,  0,  0,  0, 40 };
static const int16_t gem_vz[] = {-16, -8, 16, 16, -8, -40,-16, 32, 32,-16,  0 };

static const uint8_t gem_fa[] = { 0,0,0, 0,0,1,1,2,2,3,3,4,4, 5,6,7,8,9 };
static const uint8_t gem_fb[] = { 1,2,3, 6,5,7,6,8,7,9,8,5,9, 10,10,10,10,10 };
static const uint8_t gem_fc[] = { 2,3,4, 1,6,2,7,3,8,4,9,0,5, 6,7,8,9,5 };

// 1bpp pixel buffer (expanded to 2bpp on-the-fly during VRAM copy)
static uint8_t pb[1920];

// Fill triangle at 4x4 block resolution into both bitplanes (solid color 3)
static void fill_tri_px(int16_t x0, int16_t y0,
                         int16_t x1, int16_t y1,
                         int16_t x2, int16_t y2) {
    int16_t tmp, by, long_x, short_x, lx, rx, bx;
    uint16_t row_base, byte_off;
    uint8_t nibble;
    int16_t py, px_start;

    if (y0 > y1) { tmp=x0; x0=x1; x1=tmp; tmp=y0; y0=y1; y1=tmp; }
    if (y0 > y2) { tmp=x0; x0=x2; x2=tmp; tmp=y0; y0=y2; y2=tmp; }
    if (y1 > y2) { tmp=x1; x1=x2; x2=tmp; tmp=y1; y1=y2; y2=tmp; }
    if (y0 == y2) return;

    for (by = (y0 >> 2); by <= (y2 >> 2); by++) {
        int16_t center_y = by * 4 + 2;
        if (center_y < y0 || center_y > y2 || center_y < 0 || center_y >= FB_HEIGHT)
            continue;

        long_x = edge_interp(x0, y0, x2, y2, center_y);
        if (center_y < y1) {
            short_x = edge_interp(x0, y0, x1, y1, center_y);
        } else {
            short_x = edge_interp(x1, y1, x2, y2, center_y);
        }
        if (long_x > short_x) { tmp = long_x; long_x = short_x; short_x = tmp; }

        lx = long_x < 0 ? 0 : long_x;
        rx = short_x >= FB_WIDTH ? FB_WIDTH - 1 : short_x;
        if (lx > rx) continue;

        lx = lx >> 2;
        rx = rx >> 2;

        for (py = by * 4; py < by * 4 + 4; py++) {
            if (py < 0 || py >= FB_HEIGHT) continue;
            row_base = ((uint16_t)(py >> 3) << 7) | (py & 7);

            for (bx = lx; bx <= rx; bx++) {
                px_start = bx * 4;
                if (px_start < 0 || px_start >= FB_WIDTH) continue;

                byte_off = row_base + ((uint16_t)(px_start >> 3) << 3);
                nibble = (px_start & 4) ? 0x0F : 0xF0;

                pb[byte_off] |= nibble;
                pb[1920 + byte_off] |= nibble;
            }
        }
    }
}

void fb_init(void) {
    uint8_t x, y;
    uint16_t i;
    volatile uint8_t* vram;

    for (y = 0; y < 18; y++)
        for (x = 0; x < 20; x++)
            set_bkg_tile_xy(x, y, FB_BORDER_TILE);

    for (y = 0; y < FB_TILES_Y; y++)
        for (x = 0; x < FB_TILES_X; x++)
            set_bkg_tile_xy(x + FB_OFFSET_X, y + FB_OFFSET_Y,
                            y * FB_TILES_X + x);

    vram = (volatile uint8_t*)0x8000;
    for (i = 0; i < 16; i++)
        vram[FB_BORDER_TILE * 16 + i] = 0;

#ifdef CGB
    VBK_REG = 1;
    for (y = 0; y < FB_TILES_Y; y++)
        for (x = 0; x < FB_TILES_X; x++)
            set_bkg_tile_xy(x + FB_OFFSET_X, y + FB_OFFSET_Y, 5);
    VBK_REG = 0;
#endif
}

void gb_update_and_render(mrbz_vm* vm, int16_t angle_x, int16_t angle_y,
                           mrbz_value* ret) {
    int16_t sn_x, cs_x, sn_y, cs_y;
    int16_t proj_x[GEM_VERTS], proj_y[GEM_VERTS];
    int16_t i, x, y, z, x2, z2, y2;
    int16_t nz, ax, ay, bx, by, cvx, cvy;
    uint8_t a, b, c;
    uint16_t j, dst;

    (void)vm;

    sn_x = sin_lookup(angle_x);
    cs_x = sin_lookup(angle_x + 64);
    sn_y = sin_lookup(angle_y);
    cs_y = sin_lookup(angle_y + 64);

    for (i = 0; i < GEM_VERTS; i++) {
        x = gem_vx[i]; y = gem_vy[i]; z = gem_vz[i];
        x2 = (x * cs_y - z * sn_y) / 64;
        z2 = (x * sn_y + z * cs_y) / 64;
        y2 = (y * cs_x - z2 * sn_x) / 64;
        proj_x[i] = 80 + x2;
        proj_y[i] = 72 + y2;
    }

    // Clear shadow buffer
    for (j = 0; j < 360; j++) shadow_tiles[j] = TILE_EMPTY;
#ifdef CGB
    for (j = 0; j < 360; j++) shadow_attrs[j] = 0;
#endif

    // Fill faces into tile shadow buffer
    for (i = 0; i < GEM_FACES; i++) {
        a = gem_fa[i]; b = gem_fb[i]; c = gem_fc[i];
        ax = proj_x[a]; ay = proj_y[a];
        bx = proj_x[b]; by = proj_y[b];
        cvx = proj_x[c]; cvy = proj_y[c];

        nz = (bx - ax) * (cvy - ay) - (by - ay) * (cvx - ax);
        if (nz >= -20) continue;

        fill_tri_internal(ax, ay, bx, by, cvx, cvy, TILE_FILL);
    }

    // Flush to VRAM (360 bytes, fast, no flash)
    wait_vbl_done();
    set_bkg_tiles(0, 0, 20, 18, shadow_tiles);
#ifdef CGB
    VBK_REG = 1;
    set_bkg_tiles(0, 0, 20, 18, shadow_attrs);
    VBK_REG = 0;
#endif

    MRBZ_SET_NIL(*ret);
}
