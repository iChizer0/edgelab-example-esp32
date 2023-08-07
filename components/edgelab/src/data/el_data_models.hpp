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

#ifndef _EL_DATA_MODELS_HPP_
#define _EL_DATA_MODELS_HPP_

#include <cstdint>
#include <unordered_map>

#include "el_compiler.h"
#include "el_config_internal.h"
#include "el_debug.h"
#include "el_flash.h"
#include "el_types.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace edgelab::data {

using el_model_id_t = decltype(el_model_info_t::id);

class Models {
   public:
    Models();
    ~Models();

    Models(const Models&)            = delete;
    Models& operator=(const Models&) = delete;

    el_err_code_t init(const char* partition_name = CONFIG_EL_MODEL_PARTITION_NAME);
    void          deinit();

    size_t                                                    seek_models_from_flash();
    bool                                                      has_model(el_model_id_t model_id);
    el_err_code_t                                             get(el_model_id_t model_id, el_model_info_t* model_info);
    const el_model_info_t&                                    get_model_info(el_model_id_t model_id);
    const std::unordered_map<el_model_id_t, el_model_info_t>& get_all_model_info();

   protected:
    bool     verify_header_magic(const el_model_header_t* header);
    uint8_t  parse_model_id(const el_model_header_t* header);
    uint8_t  parse_model_type(const el_model_header_t* header);
    uint32_t parse_model_size(const el_model_header_t* header);

   private:
    uint32_t                                           __partition_start_addr;
    uint32_t                                           __partition_size;
    const uint8_t*                                     __flash_2_memory_map;
    el_model_mmap_handler_t                            __mmap_handler;
    std::unordered_map<el_model_id_t, el_model_info_t> __model_info;
};

Models::Models()
    : __partition_start_addr(0), __partition_size(0), __flash_2_memory_map(nullptr), __mmap_handler(), __model_info() {}

Models::~Models() { deinit(); }

el_err_code_t Models::init(const char* partition_name) {
    el_err_code_t ret{el_model_partition_mmap_init(
      partition_name, &__partition_start_addr, &__partition_size, &__flash_2_memory_map, &__mmap_handler)};
    if (ret != EL_OK) return ret;
    seek_models_from_flash();
    return ret;
}

void Models::deinit() {
    el_model_partition_mmap_deinit(&__mmap_handler);
    __flash_2_memory_map = nullptr;
    __mmap_handler       = el_model_mmap_handler_t{};
    __model_info.clear();
}

size_t Models::seek_models_from_flash() {
    if (!__flash_2_memory_map) [[unlikely]]
        return 0u;

    size_t                   header_size = sizeof(el_model_header_t);
    const uint8_t*           mem_addr    = nullptr;
    const el_model_header_t* header      = nullptr;
    for (size_t it = 0u; it < __partition_size; it += header_size) {
        mem_addr = __flash_2_memory_map + it;
        header   = reinterpret_cast<const el_model_header_t*>(mem_addr);
        if (!verify_header_magic(header)) continue;

        uint8_t  model_id   = parse_model_id(header);
        uint8_t  model_type = parse_model_type(header);
        uint32_t model_size = parse_model_size(header);
        if (!model_id || !model_type || !model_size || model_size > (__partition_size - it)) [[unlikely]]
            continue;

        __model_info.emplace(model_id,
                             el_model_info_t{.id          = model_id,
                                             .type        = model_type,
                                             .addr_flash  = __partition_start_addr + it,
                                             .size        = model_size,
                                             .addr_memory = mem_addr + header_size});
        it += model_size;
    }

    return __model_info.size();
}

bool Models::has_model(el_model_id_t model_id) {
    auto it{__model_info.find(model_id)};
    return it != __model_info.end();
}

el_err_code_t Models::get(el_model_id_t model_id, el_model_info_t* model_info) {
    if (!model_info) return EL_EINVAL;
    auto it{__model_info.find(model_id)};
    if (it != __model_info.end()) [[likely]] {
        *model_info = it->second;
        return EL_OK;
    }
    return EL_EINVAL;
}

const el_model_info_t& Models::get_model_info(el_model_id_t model_id) {
    auto it{__model_info.find(model_id)};
    if (it != __model_info.end()) [[likely]] {
        return it->second;
    }
    static el_model_info_t undefined_model_info{};
    return undefined_model_info;
}

const std::unordered_map<el_model_id_t, el_model_info_t>& Models::get_all_model_info() { return __model_info; }

inline bool Models::verify_header_magic(const el_model_header_t* header) {
    return (el_ntohl(header->b4[0]) & 0xFFFFFF00) == (CONFIG_EL_MODEL_HEADER_MAGIC << 8);
}

inline uint8_t Models::parse_model_id(const el_model_header_t* header) { return header->b1[3] >> 4; }

inline uint8_t Models::parse_model_type(const el_model_header_t* header) { return header->b1[3] & 0x0F; }

inline uint32_t Models::parse_model_size(const el_model_header_t* header) {
    return ((el_ntohl(header->b4[1]) & 0xFFFFFF00) >> 8);
}

}  // namespace edgelab::data

#ifdef __cplusplus
}
#endif

#endif
