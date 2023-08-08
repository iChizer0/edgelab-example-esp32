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

namespace types {

template <typename ValueType> struct el_storage_kv_t {
    explicit el_storage_kv_t(const char* key_, ValueType&& value_, size_t size_ = sizeof(ValueType)) noexcept
        : key(key_), value(value_), size(size_) {
        EL_ASSERT(size > 0);
    }

    ~el_storage_kv_t() = default;

    const char* key;
    ValueType   value;
    size_t      size;
};

}  // namespace types

namespace utility {

template <typename ValueType,
          typename ValueTypeNoRef = typename std::remove_reference<ValueType>::type,
          typename ValueTypeNoCV  = typename std::remove_cv<ValueType>::type,
          typename std::enable_if<std::is_trivial<ValueTypeNoRef>::value ||
                                  std::is_standard_layout<ValueTypeNoRef>::value>::type* = nullptr>
edgelab::data::types::el_storage_kv_t<ValueTypeNoCV> el_make_storage_kv(const char* key, ValueType&& value) {
    return edgelab::data::types::el_storage_kv_t<ValueTypeNoCV>(key, std::forward<ValueTypeNoCV>(value));
}

// template <typename DataType> edgelab::data::types::el_storage_kv_t<ValueType> el_storage_kv(DataType&& data) {

// }

}  // namespace utility

class Storage {
   public:
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

    // size of the buffer should be equal to handler->value_len
    template <typename ValueType, typename std::enable_if<!std::is_const<ValueType>::value>::type* = nullptr>
    bool get(types::el_storage_kv_t<ValueType>& kv) const {
        volatile const Guard guard(this);
        if (!kv.size || !__kvdb) [[unlikely]]
            return false;

        fdb_kv   handler{};
        fdb_kv_t p_handler = fdb_kv_get_obj(__kvdb, kv.key, &handler);

        if (!p_handler || !p_handler->value_len) [[unlikely]]
            return false;

        unsigned char* buffer = new unsigned char[p_handler->value_len];
        fdb_blob       blob{};
        fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
                      fdb_kv_to_blob(p_handler, fdb_blob_make(&blob, buffer, p_handler->value_len)));
        std::memcpy(&kv.value, buffer, std::min(kv.size, static_cast<size_t>(p_handler->value_len)));
        delete[] buffer;

        return true;
    }

    template <typename ValueType,
              typename std::enable_if<!std::is_const<ValueType>::value &&
                                      std::is_lvalue_reference<ValueType>::value>::type* = nullptr>
    bool get(types::el_storage_kv_t<ValueType>&& kv) const {
        return get(static_cast<types::el_storage_kv_t<ValueType>&>(kv));
    }

    template <
      typename ValueType,
      typename ValueTypeNoCVRef = typename std::remove_cv<typename std::remove_reference<ValueType>::type>::type,
      typename std::enable_if<std::is_trivial<ValueTypeNoCVRef>::value ||
                              std::is_standard_layout<ValueTypeNoCVRef>::value>::type* = nullptr>
    ValueTypeNoCVRef get_value(const char* key) const {
        ValueTypeNoCVRef value{};
        auto             kv                            = utility::el_make_storage_kv(key, value);
        bool             is_ok __attribute__((unused)) = get(kv);
        EL_ASSERT(is_ok);
        return value;
    }

    template <typename KVType> Storage& operator>>(KVType&& kv) {
        bool is_ok __attribute__((unused)) = get(std::forward<KVType>(kv));
        EL_ASSERT(is_ok);
        return *this;
    }

    template <typename ValueType> bool emplace(const types::el_storage_kv_t<ValueType>& kv) {
        volatile const Guard guard(this);
        if (!kv.size || !__kvdb) [[unlikely]]
            return false;
        fdb_blob blob{};
        return fdb_kv_set_blob(__kvdb, kv.key, fdb_blob_make(&blob, &kv.value, kv.size)) == FDB_NO_ERR;
    }

    template <typename KVType> Storage& operator<<(KVType&& kv) {
        bool is_ok __attribute__((unused)) = emplace(std::forward<KVType>(kv));
        EL_ASSERT(is_ok);
        return *this;
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
        using value_type        = const char*;
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
            if (!___storage || !___kvdb) [[unlikely]]
                return *this;
            volatile const Guard guard(___storage);
            ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);

            return *this;
        }

        Iterator operator++(int) {
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
        mutable value_type   ___value;
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
