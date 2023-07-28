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

#include "el_flash_esp.h"

static int el_flash_init(void) {
    if (el_flash_lock == NULL) {
        el_flash_lock = xSemaphoreCreateCounting(1, 1);
        assert(el_flash_lock != NULL);
    }

    el_flash_partition =
      esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, EL_FLASH_PARTITION_NAME);

    assert(el_flash_partition != NULL);

    return 1;
}

static int el_flash_read(long offset, uint8_t* buf, size_t size) {
    esp_err_t ret;

    xSemaphoreTake(el_flash_lock, portMAX_DELAY);
    ret = esp_partition_read(el_flash_partition, offset, buf, size);
    xSemaphoreGive(el_flash_lock);

    return ret;
}

static int el_flash_write(long offset, const uint8_t* buf, size_t size) {
    esp_err_t ret;

    xSemaphoreTake(el_flash_lock, portMAX_DELAY);
    ret = esp_partition_write(el_flash_partition, offset, buf, size);
    xSemaphoreGive(el_flash_lock);

    return ret;
}

static int el_flash_erase(long offset, size_t size) {
    esp_err_t ret;
    int32_t   erase_size = ((size - 1) / FLASH_ERASE_MIN_SIZE) + 1;

    xSemaphoreTake(el_flash_lock, portMAX_DELAY);
    ret = esp_partition_erase_range(el_flash_partition, offset, erase_size * FLASH_ERASE_MIN_SIZE);
    xSemaphoreGive(el_flash_lock);

    return ret;
}

#ifdef CONFIG_EL_LIB_FLASHDB

const struct fal_flash_dev el_flash_nor_flash0 = {
  .name       = NOR_FLASH_DEV_NAME,
  .addr       = 0x0,
  .len        = 64 * 1024,
  .blk_size   = FLASH_ERASE_MIN_SIZE,
  .ops        = {el_flash_init, el_flash_read, el_flash_write, el_flash_erase},
  .write_gran = 1,
};

#endif
