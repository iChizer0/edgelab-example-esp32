/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu (Seeed Technology Inc.)
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

#ifndef _EL_FLASH_ESP_H
#define _EL_FLASH_ESP_H

#define CONFIG_EL_LIB_FLASHDB

#ifdef CONFIG_EL_LIB_FLASHDB
    #define NOR_FLASH_DEV_NAME "norflash0"

    #define FAL_FLASH_DEV_TABLE \
        { &nor_flash0, }

    #ifdef FAL_PART_HAS_TABLE_CFG
        #define FAL_PART_TABLE                                                                     \
            {                                                                                      \
                {FAL_PART_MAGIC_WORD, "fdb_kvdb1", NOR_FLASH_DEV_NAME, 0, 16 * 1024, 0},           \
                  {FAL_PART_MAGIC_WORD, "fdb_tsdb1", NOR_FLASH_DEV_NAME, 16 * 1024, 16 * 1024, 0}, \
            }
    #endif

    #define FDB_USING_KVDB

    #define FDB_USING_TSDB

    #define FDB_USING_FAL_MODE

    #define FDB_WRITE_GRAN 1

    #define FDB_DEBUG_ENABLE

extern const struct fal_flash_dev nor_flash0;

#endif

#endif
