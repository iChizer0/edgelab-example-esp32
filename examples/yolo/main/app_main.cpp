// #include <inttypes.h>
// #include <stdio.h>

// #include "edgelab.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "yolo_model_data.h"

// #define kTensorArenaSize (1024 * 768)

// uint16_t color[] = {
//   0x0000,
//   0x03E0,
//   0x001F,
//   0x7FE0,
//   0xFFFF,
// };

// extern "C" void app_main(void) {
//     Device*  device  = Device::get_device();
//     Display* display = device->get_display();
//     Camera*  camera  = device->get_camera();

//     camera->init(240, 240);
//     display->init();

//     auto* engine       = new InferenceEngine<EngineName::TFLite>();
//     auto* tensor_arena = heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
//     engine->init(tensor_arena, kTensorArenaSize);
//     engine->load_model(g_yolo_model_data, g_yolo_model_data_len);
//     auto* algorithm = new YOLO(engine);
//     // algorithm->init();

//     EL_LOGI("done");
//     while (1) {
//         el_img_t img;
//         camera->start_stream();
//         camera->get_frame(&img);
//         algorithm->run(&img);
//         uint32_t preprocess_time  = algorithm->get_preprocess_time();
//         uint32_t run_time         = algorithm->get_run_time();
//         uint32_t postprocess_time = algorithm->get_postprocess_time();
//         uint8_t  i                = 0;
//         for (const auto box : algorithm->get_results()) {
//             el_printf("\tbox -> cx_cy_w_h: [%d, %d, %d, %d] t: [%d] s: [%d]\n",
//                       box.x,
//                       box.y,
//                       box.w,
//                       box.h,
//                       box.target,
//                       box.score);

//             uint16_t y = box.y - box.h / 2;
//             uint16_t x = box.x - box.w / 2;
//             el_draw_rect(&img, x, y, box.w, box.h, color[++i % 5], 4);
//         }

//         {
//             el_box_t b{
//               .x      = 1,
//               .y      = 2,
//               .w      = 3,
//               .h      = 4,
//               .score  = 6,
//               .target = 5,
//             };
//             // el_printf("\tp_b -> %p\n", &b);
//             el_printf("\ttest0 -> cx_cy_w_h: [%d, %d, %d, %d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//             // el_printf("\ttest1 -> cx_cy_w_h: [%3d, %d, %d, %d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//             // el_printf("\ttest1 -> cx_cy_w_h: [%3d, %d, %d, %d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//             // el_printf("\ttest2 -> cx_cy_w_h: [%3d, %3d, %d, %d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//             // el_printf("\ttest3 -> cx_cy_w_h: [%3d, %3d, %3d, %d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//             el_printf("\ttest4 -> cx_cy_w_h: [%3d, %3d, %3d, %3d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//         }

//         el_printf("preprocess: %d, run: %d, postprocess: %d\n", preprocess_time, run_time, postprocess_time);
//         display->show(&img);
//         camera->stop_stream();
//     }
// }

/* FlashDB ESP32 SPI Flash Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <flashdb.h>
#include <stdio.h>

// #include "el_flash.hpp"
#include "esp_chip_info.h"
#include "esp_flash.h"
// #include "esp_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#define FDB_USING_KVDB

#define FDB_LOG_TAG "[main]"

// extern "C" {

  static uint32_t boot_count    = 0;
static time_t   boot_time[10] = {0, 1, 2, 3};
/* default KV nodes */
static struct fdb_default_kv_node default_kv_table[] = {
  {"username", (void*)("armink"), 0},                       /* string KV */
  {"password", (void*)("123456"), 0},                       /* string KV */
  {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
  {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
};
/* KVDB object */
static struct fdb_kvdb kvdb = {0};
/* TSDB object */
struct fdb_tsdb tsdb = {0};
/* counts for simulated timestamp */
static int               counts = 0;
static SemaphoreHandle_t ss_lock = NULL;

void kvdb_basic_sample(fdb_kvdb_t kvdb);
// extern void kvdb_type_string_sample(fdb_kvdb_t kvdb);
// extern void kvdb_type_blob_sample(fdb_kvdb_t kvdb);
// extern void tsdb_sample(fdb_tsdb_t tsdb);

void kvdb_basic_sample(fdb_kvdb_t kvdb) {
    struct fdb_blob blob;
    int             boot_count = 0;

    printf("==================== kvdb_basic_sample ====================\n");

    { /* GET the KV value */
        /* get the "boot_count" KV value */
        fdb_kv_get_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
        /* the blob.saved.len is more than 0 when get the value successful */
        if (blob.saved.len > 0) {
            printf("get the 'boot_count' value is %d\n", boot_count);
        } else {
            printf("get the 'boot_count' failed\n");
        }
    }

    { /* CHANGE the KV value */
        /* increase the boot count */
        boot_count++;
        /* change the "boot_count" KV's value */
        fdb_kv_set_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
        printf("set the 'boot_count' value to %d\n", boot_count);
    }

    printf("===========================================================\n");
}

static void lock(fdb_db_t db) { xSemaphoreTake(ss_lock, portMAX_DELAY); }

static void unlock(fdb_db_t db) { xSemaphoreGive(ss_lock); }

static fdb_time_t get_time(void) {
    /* Using the counts instead of timestamp.
     * Please change this function to return RTC time.
     */
    return ++counts;
}

int flashdb_demo(void) {
    fdb_err_t result;

    if (ss_lock == NULL) {
        ss_lock = xSemaphoreCreateCounting(1, 1);
        assert(ss_lock != NULL);
    }

#define FDB_USING_KVDB

#ifdef FDB_USING_KVDB
    { /* KVDB Sample */
        struct fdb_default_kv default_kv;

        default_kv.kvs = default_kv_table;
        default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
        /* set the lock and unlock function if you want */
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void*)lock);
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void*)unlock);
        /* Key-Value database initialization
         *
         *       &kvdb: database object
         *       "env": database name
         * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
         *        NULL: The user data if you need, now is empty.
         */
        result = fdb_kvdb_init(&kvdb, "edgelab_db", "kvdb0", &default_kv, NULL);

        printf("---------kvkvkvkv");

        if (result != FDB_NO_ERR) {
            return -1;
        }

        printf("---------kvkvkvkv");

        /* run basic KV samples */
        kvdb_basic_sample(&kvdb);
        /* run string KV samples */
        // kvdb_type_string_sample(&kvdb);
        /* run blob KV samples */
        // kvdb_type_blob_sample(&kvdb);
    }
#endif /* FDB_USING_KVDB */



    return 0;
}

// }

#include "el_data.h"

extern "C" void app_main() {
    printf("FlashDB ESP32 SPI Flash Demo\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi, ", chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    uint32_t size_flash_chip;
    esp_flash_get_size(NULL, &size_flash_chip);

    printf("%ldMB %s flash\n",
           size_flash_chip / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    static uint32_t boot_count    = 0;
    static time_t   boot_time[10] = {0, 1, 2, 3};

    static struct fdb_default_kv_node default_kv_table[] = {
      {"username", (void*)("armink"), 0},              /* string KV */
      {"password", (void*)("123456"), 0},              /* string KV */
      {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
      {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
    };

    struct fdb_default_kv default_kv;
    default_kv.kvs = default_kv_table;
    default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

    edgelab::data::PersistentMap map;
    // map.reset();
   
  //  map.reset();

    // auto v = map["boot_time"];

    // printf("-------------- %s\n", v.name);

    // auto c = map["boot_time"];

    // printf("-------------- %s\n", c.name);

    for (const auto& a : map) 
      printf("-------------- %s\n", a.name);

// {
//     fdb_kv_t  cur_kv{map["boot_countss"]};
//     auto r = map.get<decltype(boot_count)>(cur_kv);
//     printf("rres %ld \n", r);
// }

//     boot_count += 1;

//     struct fdb_blob blob;
//     fdb_blob_make(&blob, &boot_count, sizeof(boot_count));
//     map.emplace("boot_count", &blob);

//     {
//     fdb_kv_t cur_kv{map["boot_count"]};
//     auto   r = map.get<decltype(boot_count)>(cur_kv);
//     printf("rres %ld \n", r);
//     }

    int test1 = 0;

    auto t0 = el_make_map_kv("oo", "not ok");
    auto t1 = edgelab::data::types::el_make_map_kv("test1", test1);

    map << t1 << t0;

    for (const auto& a : map) printf("-------------- %s\n", a.name);

    auto ss = edgelab::data::types::el_make_map_kv("oo", "");
    map >> t1 >> ss;

printf("------------------------------\n ");
    for (int i = 0; i < ss.size; i ++)
      printf("%c", ss.value[i]);

    printf("test1 _> %d, %s %s\n", t1.value, t0.value, ss.value);

    test1 = 3;
    auto t2    = edgelab::data::types::el_make_map_kv("test1", test1);
    map << t2;

    auto t3 = edgelab::data::types::el_make_map_kv("test1", int());
    map >> t3;
    printf("test3 _> %d\n", t3.value);

    // edgelab::data::types::Map mm;
    // mm << xx << xx;

    // size_t   data_size;
    // uint8_t* data_buf = (uint8_t*)malloc(data_size);

    // fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, data_buf, data_size));

    // printf("==================== kvdb_basic_sample ====================\n");

    // { /* GET the KV value */
    //     /* get the "boot_count" KV value */
    //     fdb_kv_get_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
    //     /* the blob.saved.len is more than 0 when get the value successful */
    //     if (blob.saved.len > 0) {
    //         printf("get the 'boot_count' value is %d\n", boot_count);
    //     } else {
    //         printf("get the 'boot_count' failed\n");
    //     }
    // }

    // { /* CHANGE the KV value */
    //     /* increase the boot count */
    //     boot_count++;
    //     /* change the "boot_count" KV's value */
    //     fdb_kv_set_blob(kvdb, "boot_count", fdb_blob_make(&blob, &boot_count, sizeof(boot_count)));
    //     printf("set the 'boot_count' value to %d\n", boot_count);
    // }

    // printf("===========================================================\n");
    // }

    // map.erase("username");

    //   for (const auto& a : map) printf("after e---------- %s\n", a.name);

    // for (const auto& a : map) printf("after e it---------- %s\n", a.name);

    for (int i = 1000; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
