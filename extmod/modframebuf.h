/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef MICROPY_INCLUDED_EXTMOD_MODFRAMEBUF_H
#define MICROPY_INCLUDED_EXTMOD_MODFRAMEBUF_H

#include "py/obj.h"

// constants for formats
#define FRAMEBUF_MVLSB    (0)
#define FRAMEBUF_RGB565   (1)
#define FRAMEBUF_GS4_HMSB (2)
#define FRAMEBUF_MHLSB    (3)
#define FRAMEBUF_MHMSB    (4)
#define FRAMEBUF_GS2_HMSB (5)
#define FRAMEBUF_PAL16    (6)
#define FRAMEBUF_PAL256   (7)

#define FRAMEBUF_PAL16_DEFAULT  (0)
#define FRAMEBUF_PAL16_C64      (1)
#define FRAMEBUF_PAL16_APPLE2   (2)
#define FRAMEBUF_PAL16_CGA      (3)
#define FRAMEBUF_PAL16_MSX      (4)
#define FRAMEBUF_PAL16_SPECTRUM (5)

#define FRAMEBUF_PAL256_676     (0)
#define FRAMEBUF_PAL256_685     (1)
#define FRAMEBUF_PAL256_884     (2)

typedef struct _mp_obj_framebuf_t {
    mp_obj_base_t base;
    mp_obj_t buf_obj; // need to store this to prevent GC from reclaiming buf
    void *buf;
    uint16_t width, height, stride;
    uint8_t format;
    uint8_t palette;
} mp_obj_framebuf_t;

typedef void (*setpixel_t)(const mp_obj_framebuf_t*, int, int, uint32_t);
typedef uint32_t (*getpixel_t)(const mp_obj_framebuf_t*, int, int);
typedef void (*fill_rect_t)(const mp_obj_framebuf_t *, int, int, int, int, uint32_t);

typedef struct _mp_framebuf_p_t {
    setpixel_t setpixel;
    getpixel_t getpixel;
    fill_rect_t fill_rect;
} mp_framebuf_p_t;

// Functions for MHLSB and MHMSB
void mono_horiz_setpixel(const mp_obj_framebuf_t *fb, int x, int y, uint32_t col);
uint32_t mono_horiz_getpixel(const mp_obj_framebuf_t *fb, int x, int y);
void mono_horiz_fill_rect(const mp_obj_framebuf_t *fb, int x, int y, int w, int h, uint32_t col);

// Functions for MVLSB format
void mvlsb_setpixel(const mp_obj_framebuf_t *fb, int x, int y, uint32_t col);
uint32_t mvlsb_getpixel(const mp_obj_framebuf_t *fb, int x, int y);
void mvlsb_fill_rect(const mp_obj_framebuf_t *fb, int x, int y, int w, int h, uint32_t col);

// Functions for RGB565 format
#define COL0(r, g, b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))
#define COL(c) COL0((c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff)

void rgb565_setpixel(const mp_obj_framebuf_t *fb, int x, int y, uint32_t col);
uint32_t rgb565_getpixel(const mp_obj_framebuf_t *fb, int x, int y);
void rgb565_fill_rect(const mp_obj_framebuf_t *fb, int x, int y, int w, int h, uint32_t col);
// Functions for GS2_HMSB format
void gs2_hmsb_setpixel(const mp_obj_framebuf_t *fb, int x, int y, uint32_t col);
uint32_t gs2_hmsb_getpixel(const mp_obj_framebuf_t *fb, int x, int y);
void gs2_hmsb_fill_rect(const mp_obj_framebuf_t *fb, int x, int y, int w, int h, uint32_t col);

// Functions for GS4_HMSB format
void gs4_hmsb_setpixel(const mp_obj_framebuf_t *fb, int x, int y, uint32_t col);
uint32_t gs4_hmsb_getpixel(const mp_obj_framebuf_t *fb, int x, int y);

void gs4_hmsb_fill_rect(const mp_obj_framebuf_t *fb, int x, int y, int w, int h, uint32_t col);

// Functions for GS8 format
void gs8_setpixel(const mp_obj_framebuf_t *fb, int x, int y, uint32_t col);
uint32_t gs8_getpixel(const mp_obj_framebuf_t *fb, int x, int y);
void gs8_fill_rect(const mp_obj_framebuf_t *fb, int x, int y, int w, int h, uint32_t col);

// Function for PAL16
void pal16_setpixel(const mp_obj_framebuf_t *fb, int x, int y, uint32_t col_index);
uint32_t pal16_getpixel(const mp_obj_framebuf_t *fb, int x, int y);
void pal16_fill_rect(const mp_obj_framebuf_t *fb, int x, int y, int w, int h, uint32_t col);

// Function for PAL16
void pal256_setpixel(const mp_obj_framebuf_t *fb, int x, int y, uint32_t col_index);
uint32_t pal256_getpixel(const mp_obj_framebuf_t *fb, int x, int y);
void pal256_fill_rect(const mp_obj_framebuf_t *fb, int x, int y, int w, int h, uint32_t col);

STATIC mp_framebuf_p_t formats[] = {
    [FRAMEBUF_MVLSB] = {mvlsb_setpixel, mvlsb_getpixel, mvlsb_fill_rect},
    [FRAMEBUF_RGB565] = {rgb565_setpixel, rgb565_getpixel, rgb565_fill_rect},
    [FRAMEBUF_GS2_HMSB] = {gs2_hmsb_setpixel, gs2_hmsb_getpixel, gs2_hmsb_fill_rect},
    [FRAMEBUF_GS4_HMSB] = {gs4_hmsb_setpixel, gs4_hmsb_getpixel, gs4_hmsb_fill_rect},
    [FRAMEBUF_PAL16] = {pal16_setpixel, pal16_getpixel, pal16_fill_rect},
    [FRAMEBUF_PAL256] = {pal256_setpixel, pal256_getpixel, pal256_fill_rect},
    [FRAMEBUF_MHLSB] = {mono_horiz_setpixel, mono_horiz_getpixel, mono_horiz_fill_rect},
    [FRAMEBUF_MHMSB] = {mono_horiz_setpixel, mono_horiz_getpixel, mono_horiz_fill_rect},
};

static inline void setpixel(const mp_obj_framebuf_t *fb, int x, int y, uint32_t col) {
    formats[fb->format].setpixel(fb, x, y, col);
}

static inline uint32_t getpixel(const mp_obj_framebuf_t *fb, int x, int y) {
    return formats[fb->format].getpixel(fb, x, y);
}
void fill_rect(const mp_obj_framebuf_t *fb, int x, int y, int w, int h, uint32_t col);

#endif // MICROPY_INCLUDED_EXTMOD_MODFRAMEBUF_H