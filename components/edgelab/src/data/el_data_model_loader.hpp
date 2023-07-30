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

#ifndef _EL_MODEL_LOADER_HPP_
#define _EL_MODEL_LOADER_HPP_

#include <cassert>
#include <cstdint>
#include <vector>

// TODO: should be moved to port later
#include <esp_partition.h>
#include <esp_spi_flash.h>

#define CONFIG_EL_MINIMAL_MODEL_SIZE (10240)

namespace edgelab::data {

namespace types {

    struct el_model_t {
        uint8_t type;
        uint8_t index;
        uint32_t size;
        const uint8_t* addr_flash;
        const uint8_t* addr_memory;
    };

}

class ModelLoader {
private:
    const uint8_t*                 __partition_start_addr;
    size_t                         __partition_size;
    const uint8_t*                 __flash_2_memory_map;
    spi_flash_mmap_handle_t        __mmap_handler; // TODO: use define for mmap handler type
    std::vector<types::el_model_t> __models;

public:
    // TODO: abstract API call later
    explicit ModelLoader(const char* partition_name) {
        const esp_partition_t* partition{esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, partition_name)};
        assert(partition != nullptr);
        __partition_start_addr = partition->address;
        __partition_size       = partition->size;

        assert(spi_flash_mmap(
            __partition_start_addr,
            __partition_size,
            SPI_FLASH_MMAP_DATA,
            reinterpret_cast<const void**>(&__flash_2_memory_map),
            &__mmap_handler) != ESP_OK);
    }

    ~ModelLoader() {
        spi_flash_mmap(__mmap_handler);
    }

    void clear() {
        // TODO: clear model partition
    }

    const std::vector<types::el_model_t>* get_models() {
        return &__models;
    }

protected:
    bool seek_models_from_flash() {
        



        return false;
    } 

};

}

#endif
