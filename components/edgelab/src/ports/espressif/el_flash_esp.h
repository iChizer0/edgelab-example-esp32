/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 nullptr (Seeed Technology Inc.)
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

#ifndef _EL_FLASH_ESP_H_
#define _EL_FLASH_ESP_H_

#include "el_common.h"
#include "esp_partition.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef CONFIG_EL_LIB_FLASHDB
    #include <fal_def.h>

    #define NOR_FLASH_DEV_NAME "nor_flash0"
    #define FAL_FLASH_DEV_TABLE \
        { &el_flash_nor_flash0, }

    #define FAL_PART_HAS_TABLE_CFG
    #ifdef FAL_PART_HAS_TABLE_CFG
        #define FAL_PART_TABLE \
            { {FAL_PART_MAGIC_WORD, "kvdb0", NOR_FLASH_DEV_NAME, 0, 64 * 1024, 0}, }
    #endif

    #define FDB_USING_KVDB
    #ifdef FDB_USING_KVDB
        #define FDB_KV_AUTO_UPDATE
    #endif
    #define FDB_USING_FAL_MODE
    #define FDB_WRITE_GRAN 1
    #define FDB_DEBUG_ENABLE

    #define FLASH_ERASE_MIN_SIZE (4 * 1024)

static SemaphoreHandle_t el_flash_lock = NULL;

const static esp_partition_t* el_flash_partition;
extern const struct fal_flash_dev el_flash_nor_flash0;

#endif

#endif
