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

#ifndef _EL_DATA_PERSISTENT_MAP_HPP_
#define _EL_DATA_PERSISTENT_MAP_HPP_

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#define CONFIG_EL_LIB_FLASHDB

#ifdef CONFIG_EL_LIB_FLASHDB

    #include <flashdb.h>

namespace edgelab::data {

namespace types {

template <class T>
struct is_c_str : std::integral_constant<bool,
                                         std::is_same<char const*, typename std::decay<T>::type>::value ||
                                           std::is_same<char*, typename std::decay<T>::type>::value> {};

template <typename KeyType, typename ContainedType, typename UnderlyingType> struct el_map_kv_t {
    explicit el_map_kv_t(KeyType key_, ContainedType value_, size_t size_ = 0, UnderlyingType underlying_ = {})
        : key(key_), value(value_), size(size_), underlying(underlying_), __call_delete(false) {}

    ~el_map_kv_t() {
        if constexpr (is_c_str<ContainedType>::value == std::true_type::value)
            if (__call_delete) delete[] value;
    };

    operator ContainedType() { return value; }

    KeyType        key;
    ContainedType  value;
    size_t         size;
    UnderlyingType underlying;
    bool           __call_delete;
};

// Disabling perfect forwarding due to char* type contained
// template <typename KeyType = const char*, typename ContainedType, typename UnderlyingType = struct fdb_kv>
// constexpr el_map_kv_t<KeyType, ContainedType, UnderlyingType> el_make_map_kv(KeyType&& key, ContainedType&& value) {
//     size_t size{0};
//     if constexpr (is_c_str<ContainedType>::value != std::true_type::value) {
//         size = sizeof(value);
//     } else {
//         size = strlen(value) + 1;
//     }
//     return el_map_kv_t<KeyType, ContainedType, UnderlyingType>(
//       std::forward<KeyType>(key), std::forward<ContainedType>(value), size);
// }

template <typename KeyType = const char*, typename ContainedType, typename UnderlyingType = struct fdb_kv>
constexpr el_map_kv_t<KeyType, ContainedType, UnderlyingType> el_make_map_kv(KeyType key, ContainedType value) {
    size_t size{0};
    if constexpr (is_c_str<ContainedType>::value != std::true_type::value) {
        size = sizeof(value);
        return el_map_kv_t<KeyType, ContainedType, UnderlyingType>(key, value, size);
    } else {
        size = std::strlen(value) + 1;
        return el_map_kv_t<KeyType, typename std::remove_const<ContainedType>::type, UnderlyingType>(key, value, size);
    }
}

}  // namespace types

template <typename KeyType = const char*, typename ValType = struct fdb_kv> class PersistentMap {
   private:
    mutable SemaphoreHandle_t __lock;
    fdb_kvdb_t                __kvdb;

    inline void m_lock() const noexcept { xSemaphoreTake(__lock, portMAX_DELAY); }
    inline void m_unlock() const noexcept { xSemaphoreGive(__lock); }

    struct Guard {
        Guard(const PersistentMap<KeyType, ValType>* const persistent_map) noexcept
            : ___persistent_map(persistent_map) {
            ___persistent_map->m_lock();
        }

        ~Guard() noexcept { ___persistent_map->m_unlock(); }

        Guard(const Guard&)            = delete;
        Guard& operator=(const Guard&) = delete;

        const PersistentMap<KeyType, ValType>* const ___persistent_map;
    };

   protected:
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = ValType;
        using pointer           = value_type*;
        using reference         = value_type&;

        explicit Iterator(fdb_kvdb_t kvdb, SemaphoreHandle_t lock)
            : ___kvdb(kvdb), ___lock(lock), ___value(value_type()), ___reach_end(true) {
            xSemaphoreTake(___lock, portMAX_DELAY);
            if (___kvdb != NULL) [[likely]] {
                fdb_kv_iterator_init(___kvdb, &___iterator);
                ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);
            }
            xSemaphoreGive(___lock);
        }

        reference operator*() const {
            ___value = ___iterator.curr_kv;
            return ___value;
        }

        pointer operator->() {
            ___value = ___iterator.curr_kv;
            return &___value;
        }

        Iterator& operator++() {
            xSemaphoreTake(___lock, portMAX_DELAY);
            if (___kvdb != NULL) [[likely]] {
                ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);
            }
            xSemaphoreGive(___lock);
            return *this;
        }

        Iterator& operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator& lfs, const Iterator& rhs) {
            return lfs.___reach_end == rhs.___reach_end;
        };

        friend bool operator!=(const Iterator& lfs, const Iterator& rhs) {
            return lfs.___reach_end != rhs.___reach_end;
        }

       protected:
        struct fdb_kv_iterator ___iterator;
        fdb_kvdb_t             ___kvdb;
        SemaphoreHandle_t      ___lock;
        mutable value_type     ___value;
        volatile bool          ___reach_end;
    };

   public:
    explicit PersistentMap(struct fdb_default_kv* default_kv = NULL) {
        static SemaphoreHandle_t lock{xSemaphoreCreateCounting(1, 1)};
        __lock = lock;
        assert(__lock != NULL);

        static struct fdb_kvdb kvdb {};
        __kvdb = &kvdb;

        auto ret{fdb_kvdb_init(__kvdb, "edgelab_db", "kvdb0", default_kv, NULL)};
        assert(ret != FDB_NO_ERR);
        assert(__kvdb != NULL);
    }

    ~PersistentMap() = default;

    PersistentMap(const PersistentMap&)            = delete;
    PersistentMap& operator=(const PersistentMap&) = delete;

    ValType* operator[](KeyType key) {
        volatile const Guard guard(this);
        static ValType       kv{};
        return fdb_kv_get_obj(__kvdb, key, &kv);
    }

    template <typename TargetType> TargetType* get(const ValType* kv, TargetType* data_buf, size_t size) {
        volatile const Guard guard(this);
        struct fdb_blob      blob;
        if (kv != NULL || kv->value_len == size) [[likely]] {
            fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
                          fdb_kv_to_blob(const_cast<fdb_kv_t>(kv), fdb_blob_make(&blob, data_buf, size)));
        }
        return data_buf;
    }

    bool emplace(KeyType key, const fdb_blob_t blob) {
        volatile const Guard guard(this);
        return fdb_kv_set_blob(__kvdb, key, blob) == FDB_NO_ERR;
    }

    bool erase(KeyType key) {
        volatile const Guard guard(this);
        return fdb_kv_del(__kvdb, key) == FDB_NO_ERR;
    }

    void clear() {
        volatile const Guard   guard(this);
        struct fdb_kv_iterator iterator;
        fdb_kv_iterator_init(__kvdb, &iterator);
        while (fdb_kv_iterate(__kvdb, &iterator)) fdb_kv_del(__kvdb, iterator.curr_kv.name);
    }

    bool reset() {
        volatile const Guard guard(this);
        return fdb_kv_set_default(__kvdb) == FDB_NO_ERR;
    }

    bool destory() {
        m_lock();
        return fdb_kvdb_deinit(__kvdb) == FDB_NO_ERR;
    }

    template <typename ContainedType>
    PersistentMap& operator<<(const types::el_map_kv_t<KeyType, ContainedType, ValType>& rhs) {
        struct fdb_blob blob;
        if constexpr (types::is_c_str<ContainedType>::value != std::true_type::value) {
            emplace(rhs.key, fdb_blob_make(&blob, &rhs.value, rhs.size));
        } else {
            emplace(rhs.key, fdb_blob_make(&blob, rhs.value, rhs.size));
        }
        return *this;
    }

    template <typename ContainedType>
    PersistentMap& operator>>(types::el_map_kv_t<KeyType, ContainedType, ValType>& rhs) {
        volatile const Guard guard(this);

        struct fdb_blob blob;
        ValType         kv{};
        ValType*        p_kv{fdb_kv_get_obj(__kvdb, rhs.key, &kv)};
        if (p_kv == NULL) return *this;

        if constexpr (types::is_c_str<ContainedType>::value != std::true_type::value) {
            if (p_kv->value_len != rhs.size) return *this;
            ContainedType buffer{};
            rhs.size =
              fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
                            fdb_kv_to_blob(const_cast<fdb_kv_t>(p_kv), fdb_blob_make(&blob, &buffer, p_kv->value_len)));
            rhs.value      = buffer;
            rhs.underlying = *p_kv;
        } else {
            char* buffer{new char[p_kv->value_len]()};
            rhs.size =
              fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
                            fdb_kv_to_blob(const_cast<fdb_kv_t>(p_kv), fdb_blob_make(&blob, buffer, p_kv->value_len)));
            rhs.value         = buffer;
            rhs.__call_delete = true;
        }

        return *this;
    }

    Iterator begin() { return Iterator(__kvdb, __lock); }
    Iterator end() { return Iterator(NULL, __lock); }
};

}  // namespace edgelab::data

#endif
#endif
