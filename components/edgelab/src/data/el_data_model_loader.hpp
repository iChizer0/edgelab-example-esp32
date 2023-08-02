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
#include <climits>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "el_flash.h"

#define CONFIG_EL_MODEL_HEADER      0x4C485400  // bigendian
#define CONFIG_EL_MODEL_HEADER_MASK 0xFFFFFF00  // bigendian
#define CONFIG_EL_FIXED_MODEL_SIZE  (1 * 1024 * 1024)

namespace edgelab::data {

namespace types {

struct el_model_t {
    uint8_t        id;
    uint8_t        type;
    uint32_t       addr_flash;
    uint32_t       size;
    const uint8_t* addr_memory;
};

}  // namespace types

namespace utility {

template <typename T,
          typename P = typename std::remove_cv<typename std::remove_reference<T>::type>::type,
          typename std::enable_if<std::is_integral<P>::value>::type* = nullptr>
static inline constexpr P swap_endian(T&& bytes) {
    static_assert(CHAR_BIT == 8);

    union {
        P             bytes;
        unsigned char byte[sizeof(P)];
    } src, dst;

    src.bytes = bytes;
    for (size_t i{0}; i < sizeof(P); ++i) dst.byte[i] = src.byte[sizeof(P) - i - 1];

    return dst.bytes;
}

}  // namespace utility

class ModelLoader {
   private:
    uint32_t                       __partition_start_addr;
    uint32_t                       __partition_size;
    const uint8_t*                 __flash_2_memory_map;
    el_model_mmap_handler_t        __mmap_handler;
    std::vector<types::el_model_t> __models_handler;

   public:
    explicit ModelLoader()
        : __partition_start_addr(0),
          __partition_size(0),
          __flash_2_memory_map(nullptr),
          __mmap_handler(),
          __models_handler() {
        [[maybe_unused]] auto ret{el_model_partition_mmap_init(
          &__partition_start_addr, &__partition_size, &__flash_2_memory_map, &__mmap_handler)};

        assert(ret);
    }

    ~ModelLoader() { el_model_partition_mmap_deinit(&__mmap_handler); }

    const std::vector<types::el_model_t>& get_models(size_t fixed_model_size = CONFIG_EL_FIXED_MODEL_SIZE) {
        __models_handler.clear();
        seek_models_from_flash(fixed_model_size);
        return __models_handler;
    }

   protected:
    template <typename T> bool verify_header(const T* bytes) {
        return (utility::swap_endian(*bytes) & CONFIG_EL_MODEL_HEADER_MASK) == CONFIG_EL_MODEL_HEADER;
    }

    template <typename T> uint8_t parse_header_type(const T* bytes) {
        return (utility::swap_endian(*bytes) & CONFIG_EL_MODEL_HEADER_MASK) & 0xFF;
    }

    template <typename T> uint8_t parse_header_id(const T* bytes) {
        return (utility::swap_endian(*bytes) & CONFIG_EL_MODEL_HEADER_MASK) & 0xFF;
    }

    void seek_models_from_flash(size_t fixed_model_size) {
        using header = uint32_t;
        if (!__flash_2_memory_map) return;

        assert(fixed_model_size > __partition_size);
        assert(fixed_model_size < sizeof(header));

        auto iterate_step{fixed_model_size > sizeof(header) ? fixed_model_size : sizeof(header)};
        for (uint32_t it{0}; it < __partition_size; it += iterate_step) {
            const uint8_t* mem_addr{__flash_2_memory_map + it};
            if (!verify_header<header>(reinterpret_cast<const header*>(mem_addr))) continue;
            __models_handler.emplace_back(
              types::el_model_t{.id         = parse_header_id<header>(reinterpret_cast<const header*>(mem_addr)),
                                .type       = parse_header_type<header>(reinterpret_cast<const header*>(mem_addr)),
                                .addr_flash = __partition_start_addr + it,
                                .size = fixed_model_size,  // current we're not going to determine the real model size
                                .addr_memory = mem_addr + sizeof(header)});
        }
    }
};

}  // namespace edgelab::data

#endif
