

#include <stdio.h>

#include "edgelab.h"

struct some_config_t {
    int val;
};

extern "C" void app_main() {
    printf("Data Example:\n");

    printf("Getting data delegate...\n");
    DataDelegate* data_dalegate = DataDelegate::get();

    printf("Geting storage handler from data delegate...\n");
    Storage* storage = data_dalegate->get_storage_handler();

    printf("Init storage...\n");
    storage->init();

    printf("Quering storage keys ->\n");
    for (const auto& k : *storage) printf("\t%s\n", k);

    printf("Clearing storage...\n");
    storage->clear();

    printf("Quering storage keys ->\n");
    for (const auto& k : *storage) printf("\t%s\n", k);

    {
        printf("Emplace KV to storage ->\n");

        auto kv    = el_make_storage_kv("key_1", 1);
        bool is_ok = storage->emplace(kv);
        printf("\tstore KV (%s, %d), %s\n", kv.key, kv.value, is_ok ? "ok" : "fail");

        int value = 2;
        is_ok     = storage->emplace(el_make_storage_kv("key_2", value));
        printf("\tstore KV (key_2, 2), %s\n", is_ok ? "ok" : "fail");

        kv = el_make_storage_kv("key_3", 3);
        *storage << kv << el_make_storage_kv("key_4", 4);
        printf("\tstore KV (key_3, 3), (key_4, 4)\n");

        char str[] = "Hello EdgeLab!";
        *storage << el_make_storage_kv("key_5", str);
        printf("\tstore KV (key_5, %s)\n", str);

        some_config_t some_config = some_config_t{.val = 42};
        auto          kv_1        = el_make_storage_kv_from_type(some_config);
        *storage << kv_1;
        printf("\tstore type KV (%s, some_config_t{ int val = %d; })\n", kv_1.key, kv_1.value.val);
    }

    {
        printf("Get KV from storage ->\n");

        auto kv    = el_make_storage_kv("key_1", 0);
        bool is_ok = storage->get(kv);
        printf("\tget KV (%s, %d), %s\n", kv.key, kv.value, is_ok ? "ok" : "fail");

        int value = storage->get_value<int>("key_2");
        printf("\tget KV (key_2, %d), %s\n", value, value == 2 ? "ok" : "fail");

        storage->get(el_make_storage_kv("key_3", value));
        printf("\tget KV (key_3, %d), %s\n", value, value == 3 ? "ok" : "fail");

        *storage >> el_make_storage_kv("key_4", value);
        printf("\tget KV (key_4, %d), %s\n", value, is_ok ? "ok" : "fail");

        char str[32] = "";
        *storage >> el_make_storage_kv("key_5", str);
        printf("\tget KV (key_5, %s)\n", str);
    }

    printf("Quering storage keys ->\n");
    for (const auto& k : *storage) printf("\t%s\n", k);

    // printf("quering init ->\n");
    // for (const auto& a : *storage) printf("\t%s\n", a);

    // struct fdb_default_kv default_kv;
    // default_kv.kvs = default_kv_table;
    // default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

    // edgelab::data::Storage map(CONFIG_EL_DATA_PERSISTENT_MAP_NAME, CONFIG_EL_DATA_PERSISTENT_MAP_PATH, &default_kv);
    // // map.destory();
    // int i = 0;

    // printf("quering init ->\n");
    // for (const auto& a : map) printf("\t%s\n", a.name);

    // printf("test %d\n", i);
    // auto t0 = el_make_storage_kv("t0", 0);
    // map << t0;
    // printf("quering %d ->\n", i);
    // for (const auto& a : map) printf("\t%s\n", a.name);

    // printf("test %d\n", ++i);
    // map.erase("t0");
    // printf("quering %d ->\n", i);
    // for (const auto& a : map) printf("\t%s\n", a.name);

    // printf("test %d\n", ++i);
    // map.erase("not exist");
    // printf("quering %d ->\n", i);
    // for (const auto& a : map) printf("\t%s\n", a.name);

    // printf("test %d\n", ++i);
    // map["t0"];

    // printf("test %d\n", ++i);
    // auto t1 = el_make_storage_kv("t0", 0);
    // map >> t1;
    // printf("\t value: %d\n", t1.value);

    // printf("test %d\n", ++i);
    // auto t2 = el_make_storage_kv("boot_count", 0);
    // map >> t2;
    // printf("\t boot_count: %d\n", t2.value);

    // printf("test %d\n", ++i);
    // auto t3 = el_make_storage_kv("boot_count", t2.value + 1);
    // map << t3;
    // printf("\t setting boot_count to %d\n", t3.value);

    // printf("test %d\n", ++i);
    // auto t4 = el_make_storage_kv("boot_count", 0);
    // map >> t4;
    // printf("\t boot_count: %d\n", t4.value);

    // printf("test %d\n", ++i);
    // auto t5 = el_make_storage_kv("string", "hello");
    // map << t5;
    // for (const auto& a : map) printf("\t%s\n", a.name);

    // printf("test %d\n", ++i);
    // auto t6 = el_make_storage_kv("string", "");
    // map >> t6;
    // printf("\t string: %s\n", t6.value);

    // printf("test %d\n", ++i);
    // auto t7 = el_make_storage_kv("string", "hello world!");
    // map << t7;
    // printf("\t string: %s\n", t6.value);

    // printf("test %d\n", ++i);
    // auto t8 = el_make_storage_kv("string", "");
    // map >> t8;
    // printf("\t string: %s\n", t8.value);

    for (int i = 1000; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
