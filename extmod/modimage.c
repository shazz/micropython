/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Shazz - TRSi
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

#include <stdio.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "images/bmp.h"
#include "images/gif.h"
#include "modframebuf.h"

typedef struct _mp_obj_image_t {
    mp_obj_base_t base;
    mp_obj_framebuf_t *fb;
} mp_obj_image_t;

// --------------------------------------------------------------------------------------------------------------------
STATIC mp_obj_t image_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    mp_obj_image_t *o = m_new_obj(mp_obj_image_t);
    o->base.type = type;
    o->fb = (mp_obj_framebuf_t*)args[0];

    return MP_OBJ_FROM_PTR(o);
}

// --------------------------------------------------------------------------------------------------------------------
STATIC mp_obj_t image_loadbmp(size_t n_args, const mp_obj_t *args) {
    (void)n_args;

    printf("Loading BMP\n");

    mp_obj_image_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *filename = mp_obj_str_get_str(args[1]);

    mp_int_t x = 0;
    mp_int_t y = 0;

    if (n_args > 2){
        x = mp_obj_get_int(args[2]);
        y = mp_obj_get_int(args[3]);
    }

    printf("filename: %s\n", filename);

    return load_bmp(self->fb, filename, x, y);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(image_loadbmp_obj, 2, 4, image_loadbmp);

// --------------------------------------------------------------------------------------------------------------------
STATIC mp_obj_t image_loadgif(size_t n_args, const mp_obj_t *args) {
    
    (void)n_args;

    mp_obj_image_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *filename = mp_obj_str_get_str(args[1]);
    mp_obj_t callback    = MP_OBJ_TO_PTR(args[2]);

    mp_int_t x = 0;
    mp_int_t y = 0;

    if (n_args > 3){
        x = mp_obj_get_int(args[3]);
        y = mp_obj_get_int(args[4]);
    }
   
    return gif_load(self->fb, filename, x, y, callback);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(image_loadgif_obj, 3, 5, image_loadgif);

// --------------------------------------------------------------------------------------------------------------------

// Methods for Class "Image"
STATIC const mp_rom_map_elem_t image_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_loadbmp), MP_ROM_PTR(&image_loadbmp_obj) },  
    { MP_ROM_QSTR(MP_QSTR_loadgif), MP_ROM_PTR(&image_loadgif_obj) },
};
STATIC MP_DEFINE_CONST_DICT(image_locals_dict, image_locals_dict_table);

STATIC const mp_obj_type_t mp_type_image = {
    { &mp_type_type },
    .name = MP_QSTR_Image,
    .make_new = image_make_new,
    .locals_dict = (mp_obj_dict_t*)&image_locals_dict,
};

// Functions for the Module "image" and the Class "Image"
STATIC const mp_rom_map_elem_t image_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_image) },
    { MP_ROM_QSTR(MP_QSTR_Image), MP_ROM_PTR(&mp_type_image) },
    { MP_ROM_QSTR(MP_QSTR_loadbmp), MP_ROM_PTR(&image_loadbmp_obj) },
    { MP_ROM_QSTR(MP_QSTR_loadgif), MP_ROM_PTR(&image_loadgif_obj) },
};
STATIC MP_DEFINE_CONST_DICT(image_module_globals, image_module_globals_table);

// define module
const mp_obj_module_t mp_module_image = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&image_module_globals,
};