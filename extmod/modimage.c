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
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "images/bmp.h"
#include "images/gif.h"
#include "images/picojpeg.h"
#include "modframebuf.h"

// defined for fs operation
#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#if MICROPY_HW_ENABLE_IMAGE

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
#ifdef MICROPY_HW_IMAGE_JPEG

struct str_callback_data {
    int line_size;
    uint16_t * fb;
};

STATIC int datasrc(void *buffer, int size, void *pdata) {
    FIL *f = (FIL *)pdata;
    UINT n;

    f_read(f, buffer, size, &n);
    return n;
}

STATIC int sinkfn(void *buffer, int size, void *pdata) {
    struct str_callback_data *pcb = (struct str_callback_data *)pdata;
    int i = 0;
    uint16_t * fb = (uint16_t *) pcb->fb;
    uint8_t * buf = (uint8_t *) buffer;

    // that was tricky to find :) need to reverse nibbles to make the 16bits RGB565 pixel data
    for(i=0;i<size/2;i++) {
        uint8_t byte1 = *buf++;
        uint8_t byte2 = *buf++;
        *fb++ = (byte1 << 8 | byte2);
    }
    // update bytearray pointer
    pcb->fb = fb;
    
    return(0);
}

STATIC mp_obj_t image_loadjpeg(size_t n_args, const mp_obj_t *args) {
    (void)n_args;

    mp_obj_image_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *filename = mp_obj_str_get_str(args[1]);

    FIL fp;
    FRESULT res;
    const char *p_out;
    fs_user_mount_t *vfs_fat;    

    // for decoding
    uint8_t status;
    void *storage;
    pjpeg_image_info_t image_info;
    unsigned int linebuf_size;
    struct str_callback_data cb;   

    // retrieve VFS FAT mount
    mp_vfs_mount_t *vfs = mp_vfs_lookup_path(filename, &p_out);
    if (vfs != MP_VFS_NONE && vfs != MP_VFS_ROOT) {
        vfs_fat = MP_OBJ_TO_PTR(vfs->obj);
    }
    else {
        printf("Cannot find user mount for %s\n", filename);
        return mp_const_none;
    }   

    res = f_open(&vfs_fat->fatfs, &fp, filename, FA_READ);
    if (res == FR_OK){
 
        // call decoder
        storage = m_malloc(pjpeg_get_storage_size());
        status = pjpeg_decode_init(&image_info, datasrc, &fp, storage, PJPG_RGB565);
        if(status) {
            printf("pjpeg_decode_init() failed with status %u\n", status);
            if (status == PJPG_UNSUPPORTED_MODE) {
                printf("Progressive JPEG files are not supported.\n");
            }
            else if(status == PJPG_NOT_JPEG) {
                printf("Not a JPEG file\n");
            }
        }
        else {
            if(image_info.m_width != 160 || image_info.m_height != 128) {
                printf("Only JPEG file of dimensions 160x128 supported\n");
            }
            else {
                pjpeg_set_window(&image_info, image_info.m_width, image_info.m_height, -1, -1);
                linebuf_size = pjpeg_get_line_buffer_size(&image_info);
                cb.line_size = (int)linebuf_size/(int)image_info.m_MCUHeight;
                image_info.m_linebuf = m_malloc(linebuf_size);

                if (image_info.m_linebuf == NULL) {
                    printf("Not enough memory\n");
                }
                else {
                    //printf("Memory allocation: %u for internal decoder, %u for linebuf\n", pjpeg_get_storage_size(), linebuf_size);      
                    cb.fb = self->fb->buf;
                    status = pjpeg_decode_scanlines(&image_info, sinkfn, &cb);
                    m_free(image_info.m_linebuf);

                    if (status)
                    {
                        printf("pjpeg_decode_scanlines() failed with status %u\n", status);
                    }
                    // else {
                    //     char *p = m_malloc(4);
                    //     switch (image_info.m_scanType)
                    //     {
                    //         case PJPG_GRAYSCALE:    strcpy(p, "GRAY"); break;
                    //         case PJPG_YH1V1:        strcpy(p, "H1V1"); break;
                    //         case PJPG_YH2V1:        strcpy(p, "H2V1"); break;
                    //         case PJPG_YH1V2:        strcpy(p, "H1V2"); break;
                    //         case PJPG_YH2V2:        strcpy(p, "H2V2"); break;
                    //         default:                strcpy(p, "UNKN");
                    //     }
                    //     printf("Width: %d, Height: %d, Comps: %d, Scan type: %d ->%s\n", image_info.m_width, image_info.m_height, image_info.m_comps, image_info.m_scanType, p);
                    //     m_free(p);
                    // }
                }

            }
        }

        //printf("Cleaning resources\n");
        f_close(&fp);
        m_free(storage);
    }
    else {
        printf("Cannot decode JPEG image\n");
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(image_loadjpeg_obj, 2, 2, image_loadjpeg);
#endif // MICROPY_HW_IMAGE_JPEG

// --------------------------------------------------------------------------------------------------------------------
#ifdef MICROPY_HW_IMAGE_P16
STATIC mp_obj_t image_loadp16(size_t n_args, const mp_obj_t *args) {
    (void)n_args;

    mp_obj_image_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *filename = mp_obj_str_get_str(args[1]);

    FIL fp;
    FRESULT res;
    const char *p_out;
    fs_user_mount_t *vfs_fat;      

    // retrieve VFS FAT mount
    mp_vfs_mount_t *vfs = mp_vfs_lookup_path(filename, &p_out);
    if (vfs != MP_VFS_NONE && vfs != MP_VFS_ROOT) {
        vfs_fat = MP_OBJ_TO_PTR(vfs->obj);
    }
    else {
        printf("Cannot find user mount for %s\n", filename);
        return mp_const_none;
    }   

    // TODO: add reading palette information
    res = f_open(&vfs_fat->fatfs, &fp, filename, FA_READ);
    if (res == FR_OK) {    
        UINT n;
        FRESULT res;
        res = f_read(&fp, self->fb->buf, 80*128, &n);
        if (res != FR_OK) {   
            printf("Cannot read P16 image\n");
        }
    }
    else {
        printf("Cannot load P16 image\n");
    }

    f_close(&fp);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(image_loadp16_obj, 2, 4, image_loadp16);
#endif //MICROPY_HW_IMAGE_P16

// --------------------------------------------------------------------------------------------------------------------
#ifdef MICROPY_HW_IMAGE_P256
STATIC mp_obj_t image_loadp256(size_t n_args, const mp_obj_t *args) {
    (void)n_args;

    mp_obj_image_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *filename = mp_obj_str_get_str(args[1]);

    FIL fp;
    FRESULT res;
    const char *p_out;
    fs_user_mount_t *vfs_fat;      

    // retrieve VFS FAT mount
    mp_vfs_mount_t *vfs = mp_vfs_lookup_path(filename, &p_out);
    if (vfs != MP_VFS_NONE && vfs != MP_VFS_ROOT) {
        vfs_fat = MP_OBJ_TO_PTR(vfs->obj);
    }
    else {
        printf("Cannot find user mount for %s\n", filename);
        return mp_const_none;
    }   

    // TODO: add reading palette information
    res = f_open(&vfs_fat->fatfs, &fp, filename, FA_READ);
    if (res == FR_OK) {    
        UINT n;
        FRESULT res;
        res = f_read(&fp, self->fb->buf, 160*128, &n);
        if (res != FR_OK) {   
            printf("Cannot read P256 image\n");
        }
    }
    else {
        printf("Cannot load P256 image\n");
    }

    f_close(&fp);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(image_loadp256_obj, 2, 4, image_loadp256);
#endif //MICROPY_HW_IMAGE_P256


// --------------------------------------------------------------------------------------------------------------------
#ifdef MICROPY_HW_IMAGE_BMP
STATIC mp_obj_t image_loadbmp(size_t n_args, const mp_obj_t *args) {
    (void)n_args;

    mp_obj_image_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *filename = mp_obj_str_get_str(args[1]);

    mp_int_t x = 0;
    mp_int_t y = 0;

    if (n_args > 2) {
        x = mp_obj_get_int(args[2]);
        y = mp_obj_get_int(args[3]);
    }

    return load_bmp(self->fb, filename, x, y);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(image_loadbmp_obj, 2, 4, image_loadbmp);
#endif //MICROPY_HW_IMAGE_BMP

// --------------------------------------------------------------------------------------------------------------------
#ifdef MICROPY_HW_IMAGE_GIF
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
#endif //MICROPY_HW_IMAGE_GIF

// --------------------------------------------------------------------------------------------------------------------

// Methods for Class "Image"
STATIC const mp_rom_map_elem_t image_locals_dict_table[] = {
#ifdef MICROPY_HW_IMAGE_BMP
    { MP_ROM_QSTR(MP_QSTR_loadbmp), MP_ROM_PTR(&image_loadbmp_obj) },  
#endif
#ifdef MICROPY_HW_IMAGE_GIF    
    { MP_ROM_QSTR(MP_QSTR_loadgif), MP_ROM_PTR(&image_loadgif_obj) },
#endif
#ifdef MICROPY_HW_IMAGE_JPEG        
    { MP_ROM_QSTR(MP_QSTR_loadjpeg), MP_ROM_PTR(&image_loadjpeg_obj) },
#endif
#ifdef MICROPY_HW_IMAGE_P16  
    { MP_ROM_QSTR(MP_QSTR_loadp16), MP_ROM_PTR(&image_loadp16_obj) },
#endif
#ifdef MICROPY_HW_IMAGE_P256
    { MP_ROM_QSTR(MP_QSTR_loadp256), MP_ROM_PTR(&image_loadp256_obj) },
#endif
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
#ifdef MICROPY_HW_IMAGE_BMP    
    { MP_ROM_QSTR(MP_QSTR_loadbmp), MP_ROM_PTR(&image_loadbmp_obj) },
#endif
#ifdef MICROPY_HW_IMAGE_GIF
    { MP_ROM_QSTR(MP_QSTR_loadgif), MP_ROM_PTR(&image_loadgif_obj) },
#endif
#ifdef MICROPY_HW_IMAGE_JPEG
    { MP_ROM_QSTR(MP_QSTR_loadjpeg), MP_ROM_PTR(&image_loadjpeg_obj) },
#endif
#ifdef MICROPY_HW_IMAGE_P16
    { MP_ROM_QSTR(MP_QSTR_loadp16), MP_ROM_PTR(&image_loadp16_obj) },
#endif   
#ifdef MICROPY_HW_IMAGE_P256
    { MP_ROM_QSTR(MP_QSTR_loadp256), MP_ROM_PTR(&image_loadp256_obj) },
#endif   
};
STATIC MP_DEFINE_CONST_DICT(image_module_globals, image_module_globals_table);

// define module
const mp_obj_module_t mp_module_image = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&image_module_globals,
};

#endif