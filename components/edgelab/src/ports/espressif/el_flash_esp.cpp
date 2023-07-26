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

    el_flash_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "db");

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

// #include <flashdb.h>

// #include <cstddef>
// #include <iterator>
// #include <memory>
// #include <utility>

// namespace edgelab::flash {

// static uint32_t boot_count    = 0;
// static time_t   boot_time[10] = {0, 1, 2, 3};

// static struct fdb_default_kv_node default_kv_table[] = {
//   {"username", (void*)("armink"), 0},              /* string KV */
//   {"password", (void*)("123456"), 0},              /* string KV */
//   {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
//   {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
// };

// template <typename KeyType = const char*, typename ValType = struct fdb_kv> class PersistentMap {
//    private:
//     mutable SemaphoreHandle_t __lock;
//     fdb_kvdb_t                __kvdb;

//     inline void m_lock() const noexcept { xSemaphoreTake(__lock, portMAX_DELAY); }
//     inline void m_unlock() const noexcept { xSemaphoreGive(__lock); }

//     struct Guard {
//         Guard(const PersistentMap<KeyType, ValType>* const persistent_map) noexcept
//             : ___persistent_map(persistent_map) {
//             ___persistent_map->m_lock();
//         }

//         ~Guard() noexcept { ___persistent_map->m_unlock(); }

//         const PersistentMap<KeyType, ValType>* const ___persistent_map;
//     };

//    public:
//     PersistentMap() {
//         static SemaphoreHandle_t lock{xSemaphoreCreateCounting(1, 1)};
//         __lock = lock;

//         static struct fdb_kvdb kvdb {};
//         __kvdb = &kvdb;

//         struct fdb_default_kv default_kv;

//         default_kv.kvs = default_kv_table;
//         default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

//         auto ret{fdb_kvdb_init(__kvdb, "edgelab_db", "kvdb0", &default_kv, NULL)};

//         assert(__lock != NULL);
//         assert(ret != FDB_NO_ERR);
//     };

//     ~PersistentMap() = default;

//     PersistentMap(const PersistentMap&)            = delete;
//     PersistentMap& operator=(const PersistentMap&) = delete;
   
//     ValType& operator[](KeyType key) {
//         volatile const Guard guard(this);
//         static ValType       kv{};
//         return *fdb_kv_get_obj(__kvdb, key, &kv);
//     }

//    protected:
//     struct Iterator {
//         using iterator_category = std::forward_iterator_tag;
//         using difference_type   = std::ptrdiff_t;
//         using value_type        = ValType;
//         using pointer           = value_type*;
//         using reference         = value_type&;

//         explicit Iterator(fdb_kvdb_t kvdb, SemaphoreHandle_t lock)
//             : ___kvdb(kvdb), ___lock(lock), ___value(value_type()), ___reach_end(true) {
//             xSemaphoreTake(___lock, portMAX_DELAY);
//             if (kvdb != NULL) {
//                 fdb_kv_iterator_init(___kvdb, &___iterator);
//                 ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);
//             }
//             xSemaphoreGive(___lock);
//         }

//         reference operator*() const {
//             ___value = ___iterator.curr_kv;
//             return ___value;
//         }
//         pointer operator->() {
//             ___value = ___iterator.curr_kv;
//             return &___value;
//         }
//         Iterator& operator++() {
//             xSemaphoreTake(___lock, portMAX_DELAY);
//             ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);
//             xSemaphoreGive(___lock);
//             return *this;
//         }
//         Iterator operator++(int) {
//             Iterator tmp = *this;
//             ++(*this);
//             return tmp;
//         }

//         friend bool operator==(const Iterator& lfs, const Iterator& rhs) {
//             return lfs.___reach_end == rhs.___reach_end;
//         };
//         friend bool operator!=(const Iterator& lfs, const Iterator& rhs) {
//             return lfs.___reach_end != rhs.___reach_end;
//         }

//         struct fdb_kv_iterator ___iterator;
//         fdb_kvdb_t             ___kvdb;
//         SemaphoreHandle_t      ___lock;
//         mutable value_type     ___value;
//         volatile bool          ___reach_end;
//     };

//    public:
//     Iterator begin() { return Iterator(__kvdb, __lock); }
//     Iterator end() { return Iterator(NULL, __lock); }
// };

// }  // namespace edgelab::flash
