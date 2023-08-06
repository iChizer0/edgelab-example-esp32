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

namespace traits {

template <typename T> struct remove_const_from_pointer {
    using type =
      typename std::add_pointer<typename std::remove_const<typename std::remove_pointer<T>::type>::type>::type;
};

template <typename T>
struct is_c_str
    : std::integral_constant<
        bool,
        std::is_same<char*, typename remove_const_from_pointer<typename std::decay<T>::type>::type>::value> {};

template <typename T, size_t Size> inline constexpr size_t bytesof_c_array(T (&)[Size]) { return Size * sizeof(T); }

template <typename T, size_t Size> inline constexpr size_t sizeof_c_array(T (&)[Size]) { return Size; }

}  // namespace traits

namespace types {

using ELStorageKeyType              = const char*;
using ELStorageKVHandlerPointerType = fdb_kv_t;

// please be careful when using pointer type as ValueType, please always access value from el_storage_kv_t when using value
template <typename KeyType, typename ValueType, typename HandlerPointerType> struct el_storage_kv_t {
    explicit el_storage_kv_t(KeyType            key_,
                             ValueType          value_,
                             size_t             size_              = 0ul,
                             size_t             size_in_bytes_     = 0ul,
                             HandlerPointerType handler_           = nullptr,
                             bool               call_value_delete  = false,
                             bool               call_handel_delete = false) noexcept
        : key(key_),
          value(value_),
          size(size_),
          size_in_bytes(size_in_bytes_),
          handler(handler_),
          __call_value_delete(call_value_delete),
          __call_handel_delete(call_handel_delete) {
        assert(size > 0);
        assert(size_in_bytes > 0);
        assert(size_in_bytes > size);
    }

    ~el_storage_kv_t() {
        if constexpr (std::is_pointer<ValueType>::value)
            if (__call_value_delete && value) delete[] value;
        if constexpr (std::is_pointer<HandlerPointerType>::value)
            if (__call_handel_delete && handler) delete handler;
    }

    using ValueTypeBase = typename std::decay<ValueType>::type;
    operator ValueTypeBase() const { return static_cast<ValueTypeBase>(value); }

    KeyType            key;
    ValueType          value;
    size_t             size;
    size_t             size_in_bytes;
    HandlerPointerType handler;

    // TODO: this should be protected, use friend to enable access from other classes
    bool __call_value_delete;
    bool __call_handel_delete;
};

}  // namespace types

namespace utility {

// TODO: move to utility namespace
template <
  typename KeyType,
  typename ValueType,
  typename KeyTypeBase   = typename std::remove_const<typename std::decay<KeyType>::type>::type,
  typename ValueTypeBase = typename std::remove_const<typename std::decay<ValueType>::type>::type,
  typename std::enable_if<std::is_convertible<KeyType, edgelab::data::types::ELStorageKeyType>::value ||
                          std::is_same<KeyTypeBase, edgelab::data::types::ELStorageKeyType>::value>::type* = nullptr>
inline constexpr edgelab::data::types::
  el_storage_kv_t<KeyTypeBase, ValueTypeBase, edgelab::data::types::ELStorageKVHandlerPointerType>
  el_make_storage_kv(KeyType&& key, ValueType&& value) noexcept {
    using ValueTypeNoRef = typename std::remove_reference<ValueType>::type;
    size_t size{0ul};
    size_t size_in_bytes{0ul};

    if constexpr (edgelab::data::traits::is_c_str<ValueTypeNoRef>::value) {
        size = size_in_bytes = std::strlen(value) + 1ul;  // add 1 for '\0' terminator
    } else if constexpr (std::is_array<ValueTypeNoRef>::value) {
        size          = edgelab::data::traits::sizeof_c_array(std::forward<ValueType>(value));
        size_in_bytes = edgelab::data::traits::bytesof_c_array(std::forward<ValueType>(value));
    } else if constexpr (!std::is_pointer<ValueTypeNoRef>::value) {
        size          = 1ul;
        size_in_bytes = sizeof(value);
    } else if constexpr (std::is_trivially_constructible<ValueTypeNoRef>::value) {
        size          = 1ul;
        size_in_bytes = sizeof(*value);
    }

    return edgelab::data::types::
      el_storage_kv_t<KeyTypeBase, ValueTypeBase, edgelab::data::types::ELStorageKVHandlerPointerType>(
        std::forward<KeyTypeBase>(key), std::forward<ValueTypeBase>(value), size_in_bytes);
}

}  // namespace utility

class Storage {
   public:
    using KeyType = types::ELStorageKeyType;
    using HandlerType =
      typename std::remove_pointer<typename std::decay<types::ELStorageKVHandlerPointerType>::type>::type;

    // currently the consistent of Storage is only ensured on a single instance if there're multiple instances that has same name and save path
    explicit Storage(struct fdb_default_kv* default_kv = nullptr,
                     const char*            name       = CONFIG_EL_STORAGE_NAME,
                     const char*            path       = CONFIG_EL_STORAGE_PATH) noexcept
        : __lock(xSemaphoreCreateCounting(1, 1)) {
        volatile const Guard guard(this);
        __kvdb = new struct fdb_kvdb();
        [[maybe_unused]] auto ret{fdb_kvdb_init(__kvdb, name, path, default_kv, nullptr)};
        assert(ret == FDB_NO_ERR);
    }

    ~Storage() {
        volatile const Guard guard(this);
        if (__kvdb && (fdb_kvdb_deinit(__kvdb) == FDB_NO_ERR)) [[likely]]
            delete __kvdb;
    };

    Storage(const Storage&)            = delete;
    Storage& operator=(const Storage&) = delete;

    template <typename KT,
              typename std::enable_if<std::is_convertible<KT, KeyType>::value ||
                                      std::is_same<typename std::decay<KT>::type, KeyType>::value>::type* = nullptr>
    HandlerType operator[](KT&& key) const {
        volatile const Guard                         guard(this);
        HandlerType                                  handler;
        typename std::add_pointer<HandlerType>::type p_handler{
          fdb_kv_get_obj(__kvdb, static_cast<KeyType>(key), &handler)};
        if (p_handler) handler = *p_handler;
        return handler;
    }

    // size of the buffer should be equal to handler->value_len
    template <typename TargetType> TargetType* get(const HandlerType* handler, TargetType* buffer) const {
        volatile const Guard guard(this);
        struct fdb_blob      blob;
        if (__kvdb && handler && handler->value_len) [[likely]] {
            fdb_blob_read(
              reinterpret_cast<fdb_db_t>(__kvdb),
              fdb_kv_to_blob(const_cast<fdb_kv_t>(handler), fdb_blob_make(&blob, buffer, handler->value_len)));
        }
        return buffer;
    }

    template <typename KT,
              typename std::enable_if<std::is_convertible<KT, KeyType>::value ||
                                      std::is_same<typename std::decay<KT>::type, KeyType>::value>::type* = nullptr>
    bool emplace(KT&& key, const fdb_blob_t blob) {
        volatile const Guard guard(this);
        return fdb_kv_set_blob(__kvdb, static_cast<KeyType>(key), blob) == FDB_NO_ERR;
    }

    template <typename KT,
              typename std::enable_if<std::is_convertible<KT, KeyType>::value ||
                                      std::is_same<typename std::decay<KT>::type, KeyType>::value>::type* = nullptr>
    bool erase(KT&& key) {
        volatile const Guard guard(this);
        return fdb_kv_del(__kvdb, static_cast<KeyType>(key)) == FDB_NO_ERR;
    }

    void clear() {
        volatile const Guard   guard(this);
        struct fdb_kv_iterator iterator;
        if (!__kvdb) return;
        fdb_kv_iterator_init(__kvdb, &iterator);
        while (fdb_kv_iterate(__kvdb, &iterator)) fdb_kv_del(__kvdb, iterator.curr_kv.name);
    }

    bool reset() {
        volatile const Guard guard(this);
        return __kvdb ? fdb_kv_set_default(__kvdb) == FDB_NO_ERR : false;
    }

    template <typename KT, typename CT, typename HPT>
    Storage& operator<<(const types::el_storage_kv_t<KT, CT, HPT>& rhs) {
        struct fdb_blob blob;

        if constexpr (std::is_pointer<CT>::value) [[unlikely]]
            emplace(rhs.key, fdb_blob_make(&blob, rhs.value, rhs.size_in_bytes));
        else
            emplace(rhs.key, fdb_blob_make(&blob, &rhs.value, rhs.size_in_bytes));

        return *this;
    }

    // currently we're always allocating new memory on the heap for CT types
    // TODO: automatically determine whether to allocate new memory based on the type of CT
    template <typename KT, typename CT, typename HPT> Storage& operator>>(types::el_storage_kv_t<KT, CT, HPT>& rhs) {
        volatile const Guard guard(this);
        if (!__kvdb) [[unlikely]]
            return *this;

        using CTNoRef = typename std::remove_reference<CT>::type;
        using HT      = typename std::remove_pointer<HPT>::type;

        // HPT of rhs should always be pointer type (currently we're not going to check it)
        HT  kv;
        HPT p_kv{fdb_kv_get_obj(__kvdb, rhs.key, &kv)};

        if (!p_kv) [[unlikely]]
            return *this;

        rhs.handler              = new HT(*p_kv);
        rhs.__call_handel_delete = true;

        struct fdb_blob blob;
        if constexpr (traits::is_c_str<CTNoRef>::value || std::is_array<CTNoRef>::value ||
                      std::is_pointer<CTNoRef>::value) {
            using CIT = typename std::remove_const<
              typename std::remove_pointer<typename std::remove_all_extents<CTNoRef>::type>::type>::type;
            using CIPT = typename std::add_pointer<CIT>::type;

            CIPT buffer{new CIT[p_kv->value_len]};
            rhs.size_in_bytes =
              fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
                            fdb_kv_to_blob(const_cast<fdb_kv_t>(p_kv), fdb_blob_make(&blob, buffer, p_kv->value_len)));

            rhs.value               = buffer;
            rhs.__call_value_delete = true;
        } else {
            CTNoRef value;
            rhs.size_in_bytes =
              fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
                            fdb_kv_to_blob(const_cast<fdb_kv_t>(p_kv), fdb_blob_make(&blob, &value, p_kv->value_len)));

            rhs.value = value;
        }
        rhs.size = rhs.size_in_bytes / sizeof(CTNoRef);

        return *this;
    }

   protected:
    inline void m_lock() const noexcept { xSemaphoreTake(__lock, portMAX_DELAY); }
    inline void m_unlock() const noexcept { xSemaphoreGive(__lock); }

    template <typename T> struct Iterator {
        using IterateType       = typename std::add_pointer<T>::type;
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = HandlerType;
        using pointer           = value_type*;
        using reference         = value_type&;

        explicit Iterator(IterateType persisitent_map)
            : ___persisitent_map(persisitent_map), ___iterator(), ___value(), ___reach_end(true) {
            if (!___persisitent_map) return;
            ___persisitent_map->m_lock();
            ___kvdb = persisitent_map->__kvdb;
            if (___kvdb) [[likely]] {
                fdb_kv_iterator_init(___kvdb, &___iterator);
                ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);
            }
            ___persisitent_map->m_unlock();
        }

        reference operator*() const {
            ___value = ___iterator.curr_kv;
            return ___value;
        }

        pointer operator->() const {
            ___value = ___iterator.curr_kv;
            return &___value;
        }

        Iterator& operator++() {
            if (!___persisitent_map) [[unlikely]]
                return *this;
            ___persisitent_map->m_lock();
            if (___kvdb) [[likely]]
                ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);
            ___persisitent_map->m_unlock();
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
        IterateType ___persisitent_map;

        fdb_kvdb_t             ___kvdb;
        struct fdb_kv_iterator ___iterator;
        mutable value_type     ___value;
        volatile bool          ___reach_end;
    };

   public:
    Iterator<Storage> begin() { return Iterator<Storage>(this); }
    Iterator<Storage> end() { return Iterator<Storage>(nullptr); }
    Iterator<Storage> cbegin() { return Iterator<Storage>(this); }
    Iterator<Storage> cend() { return Iterator<Storage>(nullptr); }

   private:
    struct Guard {
        Guard(const Storage* const persistent_map) noexcept : ___persistent_map(persistent_map) {
            ___persistent_map->m_lock();
        }

        ~Guard() noexcept { ___persistent_map->m_unlock(); }

        Guard(const Guard&)            = delete;
        Guard& operator=(const Guard&) = delete;

        const Storage* const ___persistent_map;
    };

    mutable SemaphoreHandle_t __lock;
    fdb_kvdb_t                __kvdb;
};

}  // namespace edgelab::data

#endif
#endif
