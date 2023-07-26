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

#ifndef _EL_PERSISTENT_MAP_HPP_
#define _EL_PERSISTENT_MAP_HPP_

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

#define CONFIG_EL_LIB_FLASHDB

#ifdef CONFIG_EL_LIB_FLASHDB

    #include <flashdb.h>

namespace edgelab::data {

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

    ValType& operator[](KeyType key) {
        volatile const Guard guard(this);
        static ValType       kv{};
        return *fdb_kv_get_obj(__kvdb, key, &kv);
    }

    void emplace(KeyType key, const fdb_blob_t blob) {
        volatile const Guard guard(this);
        fdb_kv_set_blob(__kvdb, key, blob);
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

    Iterator begin() { return Iterator(__kvdb, __lock); }

    Iterator end() { return Iterator(NULL, __lock); }
};

}  // namespace edgelab::data

#endif
#endif
