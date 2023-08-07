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

#ifndef _EL_DATA_STORAGE_HPP_
#define _EL_DATA_STORAGE_HPP_

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "el_config_internal.h"

#define CONFIG_EL_LIB_FLASHDB
#ifdef CONFIG_EL_LIB_FLASHDB

    #include <flashdb.h>

namespace edgelab::data {

// namespace traits {

// template <typename T> struct remove_const_from_pointer {
//     using type =
//       typename std::add_pointer<typename std::remove_const<typename std::remove_pointer<T>::type>::type>::type;
// };

// template <typename T>
// struct is_c_str
//     : std::integral_constant<
//         bool,
//         std::is_same<char*, typename remove_const_from_pointer<typename std::decay<T>::type>::type>::value> {};

// template <typename T, size_t Size> inline constexpr size_t bytesof_c_array(T (&)[Size]) { return Size * sizeof(T); }

// template <typename T, size_t Size> inline constexpr size_t sizeof_c_array(T (&)[Size]) { return Size; }

// }  // namespace traits

namespace types {

template <
  typename ValueType,
  typename _ValueType = typename std::decay<ValueType>::type,
  typename std::enable_if<std::is_trivial<_ValueType>::value || std::is_standard_layout<_ValueType>::value>::type>
struct el_storage_kv_t {
    explicit el_storage_kv_t(const char* key_, ValueType&& value_) noexcept
        : key(key_), value(value_), size(sizeof(_ValueType)), {
        EL_ASSERT(size > 0);
    }

    ~el_storage_kv_t() = default;

    const char* key;
    _ValueType  value;
    size_t      size;
};

}  // namespace types

namespace utility {

//

}  // namespace utility

class Storage {
   public:
    // using KeyType = types::ELStorageKeyType;
    // using HandlerType =
    //   typename std::remove_pointer<typename std::decay<types::ELStorageKVHandlerPointerType>::type>::type;

    // currently the consistent of Storage is only ensured on a single instance if there're multiple instances that has same name and save path
    Storage() noexcept : __lock(xSemaphoreCreateCounting(1, 1)), __kvdb(new fdb_kvdb{}) {
        EL_ASSERT(__lock);
        EL_ASSERT(__kvdb);
    }

    ~Storage() { deinit(); };

    Storage(const Storage&)            = delete;
    Storage& operator=(const Storage&) = delete;

    el_err_code_t init(fdb_default_kv* default_kv = nullptr,
                       const char*     name       = CONFIG_EL_STORAGE_NAME,
                       const char*     path       = CONFIG_EL_STORAGE_PATH) {
        volatile const Guard guard(this);
        return fdb_kvdb_init(__kvdb, name, path, default_kv, nullptr) == FDB_NO_ERR ? EL_OK : EL_EINVAL;
    }

    void deinit() {
        volatile const Guard guard(this);
        if (__kvdb && (fdb_kvdb_deinit(__kvdb) == FDB_NO_ERR)) [[likely]] {
            delete __kvdb;
            __kvdb = nullptr;
        }
    }

    // fdb_kv operator[](const char* key) {
    //     volatile const Guard guard(this);
    //     fdb_kv               handler{};
    //     fdb_kv_t             p_handler = fdb_kv_get_obj(__kvdb, key, &handler);
    //     if (p_handler) [[likely]]
    //         handler = *p_handler;
    //     return handler;
    // }

    // size of the buffer should be equal to handler->value_len
    template <typename T, typename std::enable_if<std::is_trivial<T>::value || std::is_standard_layout<T>::value>::type>
    el_storage_kv_t<T>* get(el_storage_kv_t<T>* kv) const {
        volatile const Guard guard(this);
        if (!kv->size || !__kvdb) [[unlikely]]
            return nullptr;

        fdb_kv   handler{};
        fdb_kv_t p_handler = fdb_kv_get_obj(__kvdb, kv->key, &handler);

        if (!p_handler || !p_handler->value_len) [[unlikely]]
            return nullptr;

        unsigned char* buffer = new unsigned char[p_handler->value_len];
        fdb_blob       blob{};
        fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
                      fdb_kv_to_blob(p_handler, fdb_blob_make(&blob, buffer, p_handler->value_len)));
        std::memcpy(kv->value, buffer, std::min(kv->size, p_handler->value_len));
        delete[] buffer;

        return kv;
    }

    template <typename T, typename std::enable_if<std::is_trivial<T>::value || std::is_standard_layout<T>::value>::type>
    T get_value(const char* key) const {
        volatile const Guard guard(this);
        if (!kv->size || !__kvdb) [[unlikely]]
            return nullptr;

        fdb_kv   handler{};
        fdb_kv_t p_handler = fdb_kv_get_obj(__kvdb, kv->key, &handler);

        if (!p_handler || !p_handler->value_len) [[unlikely]]
            return nullptr;

        unsigned char* buffer = new unsigned char[p_handler->value_len];
        fdb_blob       blob{};
        fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
                      fdb_kv_to_blob(p_handler, fdb_blob_make(&blob, buffer, p_handler->value_len)));
        T value;
        std::memcpy(kv->value, buffer, std::min(kv->size, p_handler->value_len));
        delete[] buffer;

        return kv;
    }

    template <typename T, typename std::enable_if<std::is_trivial<T>::value || std::is_standard_layout<T>::value>::type>
    bool emplace(const el_storage_kv_t<T>* kv) {
        volatile const Guard guard(this);
        if (!kv->size || !__kvdb) [[unlikely]]
            return false;
        fdb_blob blob{};
        return fdb_kv_set_blob(__kvdb, kv->key, fdb_blob_make(&blob, kv->value, kv->size)) == FDB_NO_ERR;
    }

    bool erase(const char* key) {
        volatile const Guard guard(this);
        if (!__kvdb) [[unlikely]]
            return false;
        return fdb_kv_del(__kvdb, key) == FDB_NO_ERR;
    }

    void clear() {
        volatile const Guard guard(this);
        if (!__kvdb) [[unlikely]]
            return;
        struct fdb_kv_iterator iterator;
        fdb_kv_iterator_init(__kvdb, &iterator);
        while (fdb_kv_iterate(__kvdb, &iterator)) fdb_kv_del(__kvdb, iterator.curr_kv.name);
    }

    bool reset() {
        volatile const Guard guard(this);
        return __kvdb ? fdb_kv_set_default(__kvdb) == FDB_NO_ERR : false;
    }

    // template <typename KT, typename CT, typename HPT>
    // Storage& operator<<(const types::el_storage_kv_t<KT, CT, HPT>& rhs) {
    //     struct fdb_blob blob;

    //     if constexpr (std::is_pointer<CT>::value) [[unlikely]]
    //         emplace(rhs.key, fdb_blob_make(&blob, rhs.value, rhs.size_in_bytes));
    //     else
    //         emplace(rhs.key, fdb_blob_make(&blob, &rhs.value, rhs.size_in_bytes));

    //     return *this;
    // }

    // currently we're always allocating new memory on the heap for CT types
    // TODO: automatically determine whether to allocate new memory based on the type of CT
    // template <typename KT, typename CT, typename HPT> Storage& operator>>(types::el_storage_kv_t<KT, CT, HPT>& rhs) {
    //     volatile const Guard guard(this);
    //     if (!__kvdb) [[unlikely]]
    //         return *this;

    //     using CTNoRef = typename std::remove_reference<CT>::type;
    //     using HT      = typename std::remove_pointer<HPT>::type;

    //     // HPT of rhs should always be pointer type (currently we're not going to check it)
    //     HT  kv;
    //     HPT p_kv{fdb_kv_get_obj(__kvdb, rhs.key, &kv)};

    //     if (!p_kv) [[unlikely]]
    //         return *this;

    //     rhs.handler              = new HT(*p_kv);
    //     rhs.__call_handel_delete = true;

    //     struct fdb_blob blob;
    //     if constexpr (traits::is_c_str<CTNoRef>::value || std::is_array<CTNoRef>::value ||
    //                   std::is_pointer<CTNoRef>::value) {
    //         using CIT = typename std::remove_const<
    //           typename std::remove_pointer<typename std::remove_all_extents<CTNoRef>::type>::type>::type;
    //         using CIPT = typename std::add_pointer<CIT>::type;

    //         CIPT buffer{new CIT[p_kv->value_len]};
    //         rhs.size_in_bytes =
    //           fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
    //                         fdb_kv_to_blob(const_cast<fdb_kv_t>(p_kv), fdb_blob_make(&blob, buffer, p_kv->value_len)));

    //         rhs.value               = buffer;
    //         rhs.__call_value_delete = true;
    //     } else {
    //         CTNoRef value;
    //         rhs.size_in_bytes =
    //           fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
    //                         fdb_kv_to_blob(const_cast<fdb_kv_t>(p_kv), fdb_blob_make(&blob, &value, p_kv->value_len)));

    //         rhs.value = value;
    //     }
    //     rhs.size = rhs.size_in_bytes / sizeof(CTNoRef);

    //     return *this;
    // }

   protected:
    inline void m_lock() const noexcept { xSemaphoreTake(__lock, portMAX_DELAY); }
    inline void m_unlock() const noexcept { xSemaphoreGive(__lock); }

    struct Guard {
        Guard(const Storage* const storage) noexcept : ___storage(storage) { ___storage->m_lock(); }
        ~Guard() noexcept { ___storage->m_unlock(); }
        Guard(const Guard&)            = delete;
        Guard& operator=(const Guard&) = delete;

       private:
        const Storage* const ___storage;
    };

    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = cconst har*;
        using pointer           = const char**;
        using reference         = const char* const&;

        explicit Iterator(const Storage* const storage)
            : ___storage(storage), ___kvdb(storage->__kvdb), ___iterator(), ___value(), ___reach_end(true) {
            if (!___storage) return;
            volatile const Guard guard(___storage);
            ___kvdb = storage->__kvdb;
            if (___kvdb) [[likely]] {
                fdb_kv_iterator_init(___kvdb, &___iterator);
                ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);
            }
        }

        reference operator*() const {
            ___value = ___iterator.curr_kv.name;
            return ___value;
        }

        pointer operator->() const {
            ___value = ___iterator.curr_kv.name;
            return &___value;
        }

        Iterator& operator++() {
            if (!___storage) [[unlikely]]
                return *this;
            volatile const Guard guard(___storage);

            if (___kvdb) [[likely]]
                ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);

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
        const Storage* const ___storage;
        fdb_kvdb_t           ___kvdb;
        fdb_kv_iterator      ___iterator;
        value_type           ___value;
        volatile bool        ___reach_end;
    };

   public:
    Iterator begin() { return Iterator(this); }
    Iterator end() { return Iterator(nullptr); }
    Iterator cbegin() { return Iterator(this); }
    Iterator cend() { return Iterator(nullptr); }

   private:
    mutable SemaphoreHandle_t __lock;
    fdb_kvdb_t                __kvdb;
};

}  // namespace edgelab::data

#endif
#endif
