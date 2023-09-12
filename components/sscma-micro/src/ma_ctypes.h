/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
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
 *
 */

#ifndef _MA_CTYPES_H_
#define _MA_CTYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ma_compiler.h"

typedef enum {
    MA_OK      = 0,  // success
    MA_AGAIN   = 1,  // try again
    MA_ELOG    = 2,  // logic error
    MA_ETIMOUT = 3,  // timeout
    MA_EIO     = 4,  // IO error
    MA_EINVAL  = 5,  // invalid argument
    MA_ENOMEM  = 6,  // out of memory
    MA_EBUSY   = 7,  // busy
    MA_ENOTSUP = 8,  // not supported
    MA_EPERM   = 9,  // operation not permitted
} ma_err_code_t;

typedef struct MA_ATTR_PACKED ma_shape_t {
    size_t size;
    int*   dims;
} ma_shape_t;

typedef struct MA_ATTR_PACKED ma_quant_param_t {
    float   scale;
    int32_t zero_point;
} ma_quant_param_t;

typedef struct MA_ATTR_PACKED ma_memory_pool_t {
    void*  pool;
    size_t size;
} ma_memory_pool_t;

typedef struct MA_ATTR_PACKED ma_box_t {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint8_t  score;
    uint8_t  target;
} ma_box_t;

typedef struct MA_ATTR_PACKED ma_point_t {
    uint16_t x;
    uint16_t y;
    uint8_t  score;
    uint8_t  target;
} ma_point_t;

typedef struct MA_ATTR_PACKED ma_class_t {
    uint16_t score;
    uint16_t target;
} ma_class_t;

typedef struct MA_ATTR_PACKED ma_tensor_t {
    size_t index;
    void*  data;
    size_t size;
} ma_tensor_t;

typedef struct MA_ATTR_PACKED ma_input_t {
    ma_tensor_t* tensor;
    size_t       length;
} ma_input_t;

typedef enum {
    MA_PIXEL_FORMAT_RGB888 = 0,
    MA_PIXEL_FORMAT_RGB565,
    MA_PIXEL_FORMAT_YUV422,
    MA_PIXEL_FORMAT_GRAYSCALE,
    MA_PIXEL_FORMAT_JPEG,
    MA_PIXEL_FORMAT_UNKNOWN,
} ma_pixel_format_t;

typedef enum ma_pixel_rotate_t {
    MA_PIXEL_ROTATE_0 = 0,
    MA_PIXEL_ROTATE_90,
    MA_PIXEL_ROTATE_180,
    MA_PIXEL_ROTATE_270,
    MA_PIXEL_ROTATE_UNKNOWN,
} ma_pixel_rotate_t;

typedef struct MA_ATTR_PACKED ma_img_t {
    uint8_t*          data;
    size_t            size;
    uint16_t          width;
    uint16_t          height;
    ma_pixel_format_t format;
    ma_pixel_rotate_t rotate;
} ma_img_t;

typedef struct MA_ATTR_PACKED ma_res_t {
    uint32_t width;
    uint32_t height;
} ma_res_t;

typedef enum { MA_SENSOR_TYPE_CAM = 1u } ma_sensor_type_t;

typedef enum { MA_SENSOR_STA_REG = 0u, MA_SENSOR_STA_AVAIL = 1u, MA_SENSOR_STA_LOCKED = 2u } ma_sensor_state_t;

typedef struct MA_ATTR_PACKED ma_sensor_info_t {
    uint8_t           id;
    ma_sensor_type_t  type;
    ma_sensor_state_t state;
} ma_sensor_info_t;

typedef enum {
    MA_MODEL_FMT_UNDEFINED     = 0u,
    MA_MODEL_FMT_PACKED_TFLITE = 1u,
    MA_MODEL_FMT_PLAIN_TFLITE  = 2u
} ma_model_format_t;

typedef int ma_model_format_v;

/**
 * @brief Algorithm Types
 */
typedef enum {
    MA_ALGO_TYPE_UNDEFINED = 0u,
    MA_ALGO_TYPE_FOMO      = 1u,
    MA_ALGO_TYPE_PFLD      = 2u,
    MA_ALGO_TYPE_YOLO      = 3u,
    MA_ALGO_TYPE_IMCLS     = 4u
} ma_algorithm_type_t;

/**
 * @brief Algorithm Catagories
 */
typedef enum {
    MA_ALGO_CAT_UNDEFINED = 0u,
    MA_ALGO_CAT_DET       = 1u,
    MA_ALGO_CAT_POSE      = 2u,
    MA_ALGO_CAT_CLS       = 3u,
} ma_algorithm_cat_t;

/**
 * @brief Model Header Specification
 * @details
 *      [ 24 bits magic code | 4 bits id | 4 bits type | 24 bits size (unsigned) | 8 bits unused padding ]
 *      big-endian in file
 */
typedef union MA_ATTR_PACKED ma_model_header_t {
    unsigned char b1[8];
    uint32_t      b4[2];
} ma_model_header_t;

/**
 * @brief Mdoel Info Specification
 * @details
 *      valid id range [1, 15]:
 *          0 -> undefined
 *          each model should have a unique id
 *      valid type range [1, 15]:
 *          0 -> undefined
 *          1 -> FOMO
 *          2 -> PFLD
 *          3 -> YOLO
 */
typedef struct MA_ATTR_PACKED ma_model_info_t {
    uint8_t             id;
    ma_algorithm_type_t type;
    uint32_t            addr_flash;
    uint32_t            size;
    const uint8_t*      addr_memory;
} ma_model_info_t;

typedef uint8_t ma_model_id_t;

#endif
