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

#ifndef _EL_MODELS_HPP_
#define _EL_MODELS_HPP_

#include <cassert>
#include <climits>
#include <cstdint>
#include <type_traits>
#include <unordered_map>

#include "el_compiler.h"
#include "el_flash.h"

#define CONFIG_EL_MODEL_HEADER             0x4C485400  // big-endian
#define CONFIG_EL_MODEL_HEADER_MASK        0xFFFFFF00  // big-endian
#define CONFIG_EL_MODEL_HEADER_ID_MASK     0xF0
#define CONFIG_EL_MODEL_HEADER_ID_RSHIFT   4
#define CONFIG_EL_MODEL_HEADER_TYPE_MASK   0x0F
#define CONFIG_EL_MODEL_HEADER_TYPE_RSHIFT 0
#define CONFIG_EL_FIXED_MODEL_SIZE         (1 * 1024 * 1024)

namespace edgelab::data {

namespace types {

// model header specification:
//      [ 24 bits magic code | 4 bits ID | 4 bits type ], big-endian in file
// valid ID range [1, 15]:
//      0 -> undefined
//      each model should have a unique ID
// valid type range [1, 15]:
//      0 -> undefined
//      1 -> FOMO
//      2 -> PFLD
//      3 -> YOLO
typedef uint32_t el_model_header_t;

typedef struct EL_ATTR_PACKED {
    uint8_t        id;
    uint8_t        type;
    uint32_t       addr_flash;
    uint32_t       size;
    const uint8_t* addr_memory = nullptr;
} el_model_info_t;

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

// TODO: thread safe model loading
class Models {
    using model_id_t = decltype(types::el_model_info_t::id);

   private:
    uint32_t                                               __partition_start_addr;
    uint32_t                                               __partition_size;
    const uint8_t*                                         __flash_2_memory_map;
    el_model_mmap_handler_t                                __mmap_handler;
    std::unordered_map<model_id_t, types::el_model_info_t> __model_info;

   public:
    explicit Models()
        : __partition_start_addr(0),
          __partition_size(0),
          __flash_2_memory_map(nullptr),
          __mmap_handler(),
          __model_info() {
        [[maybe_unused]] auto ret{el_model_partition_mmap_init(
          &__partition_start_addr, &__partition_size, &__flash_2_memory_map, &__mmap_handler)};
        assert(ret);
        seek_models_from_flash(CONFIG_EL_FIXED_MODEL_SIZE);
    }

    ~Models() { el_model_partition_mmap_deinit(&__mmap_handler); }

    std::unordered_map<model_id_t, types::el_model_info_t> get_all_model_info(
      bool seek_from_flash = false, size_t fixed_model_size = CONFIG_EL_FIXED_MODEL_SIZE) {
        if (seek_from_flash) {
            __model_info.clear();
            seek_models_from_flash(fixed_model_size);
        }
        return __model_info;
    }

    types::el_model_info_t get_model_info(model_id_t model_id,
                                          bool       seek_from_flash  = false,
                                          size_t     fixed_model_size = CONFIG_EL_FIXED_MODEL_SIZE) {
        if (seek_from_flash) {
            __model_info.clear();
            seek_models_from_flash(fixed_model_size);
        }
        auto it{__model_info.find(model_id)};
        if (it != __model_info.end()) [[likely]]
            return it->second;
        return {};
    }

   protected:
    template <typename T> inline bool verify_header(const T* bytes) {
        return (*bytes & CONFIG_EL_MODEL_HEADER_MASK) == CONFIG_EL_MODEL_HEADER;
    }

    template <typename T> inline uint8_t parse_header_id(const T* bytes) {
        return ((*bytes & ~CONFIG_EL_MODEL_HEADER_MASK) & CONFIG_EL_MODEL_HEADER_ID_MASK) >>
               CONFIG_EL_MODEL_HEADER_ID_RSHIFT;
    }

    template <typename T> inline uint8_t parse_header_type(const T* bytes) {
        return ((*bytes & ~CONFIG_EL_MODEL_HEADER_MASK) & CONFIG_EL_MODEL_HEADER_TYPE_MASK) >>
               CONFIG_EL_MODEL_HEADER_TYPE_RSHIFT;
    }

    void seek_models_from_flash(size_t fixed_model_size) {
        using header_t = typename types::el_model_header_t;

        if (!__flash_2_memory_map) [[unlikely]]
            return;
        auto header_size{sizeof(header_t)};

        assert(fixed_model_size < header_size);
        assert(fixed_model_size > __partition_size);

        auto header{header_t{}};
        auto iterate_step{fixed_model_size > header_size ? fixed_model_size : header_size};
        for (size_t it{0}; it < __partition_size; it += iterate_step) {
            const uint8_t* mem_addr{__flash_2_memory_map + it};

            header = utility::swap_endian(*reinterpret_cast<const header_t*>(mem_addr));
            if (!verify_header(&header)) [[likely]]
                continue;

            auto header_id{parse_header_id(&header)};
            auto header_type{parse_header_type(&header)};
            if (!header_id || !header_type) [[unlikely]]
                continue;

            __model_info.emplace(header_id,
                                 types::el_model_info_t{.id          = header_id,
                                                        .type        = header_type,
                                                        .addr_flash  = __partition_start_addr + it,
                                                        .size        = fixed_model_size,
                                                        .addr_memory = mem_addr + header_size});
        }
    }
};

}  // namespace edgelab::data

#endif
