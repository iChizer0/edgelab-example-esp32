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

#include "el_flash.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define FDB_LOG_TAG "[main]"

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
static SemaphoreHandle_t s_lock = NULL;

extern void kvdb_basic_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_string_sample(fdb_kvdb_t kvdb);
extern void kvdb_type_blob_sample(fdb_kvdb_t kvdb);
extern void tsdb_sample(fdb_tsdb_t tsdb);

static void lock(fdb_db_t db) { xSemaphoreTake(s_lock, portMAX_DELAY); }

static void unlock(fdb_db_t db) { xSemaphoreGive(s_lock); }

static fdb_time_t get_time(void) {
    /* Using the counts instead of timestamp.
     * Please change this function to return RTC time.
     */
    return ++counts;
}

int flashdb_demo(void) {
    fdb_err_t result;

    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateCounting(1, 1);
        assert(s_lock != NULL);
    }

#ifdef FDB_USING_KVDB
    { /* KVDB Sample */
        struct fdb_default_kv default_kv;

        default_kv.kvs = default_kv_table;
        default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
        /* set the lock and unlock function if you want */
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, lock);
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, unlock);
        /* Key-Value database initialization
         *
         *       &kvdb: database object
         *       "env": database name
         * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
         *        NULL: The user data if you need, now is empty.
         */
        result = fdb_kvdb_init(&kvdb, "env", "fdb_kvdb1", &default_kv, NULL);

        if (result != FDB_NO_ERR) {
            return -1;
        }

        /* run basic KV samples */
        kvdb_basic_sample(&kvdb);
        /* run string KV samples */
        kvdb_type_string_sample(&kvdb);
        /* run blob KV samples */
        kvdb_type_blob_sample(&kvdb);
    }
#endif /* FDB_USING_KVDB */

#ifdef FDB_USING_TSDB
    { /* TSDB Sample */
        /* set the lock and unlock function if you want */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_LOCK, lock);
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_SET_UNLOCK, unlock);
        /* Time series database initialization
         *
         *       &tsdb: database object
         *       "log": database name
         * "fdb_tsdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         *    get_time: The get current timestamp function.
         *         128: maximum length of each log
         *        NULL: The user data if you need, now is empty.
         */
        result = fdb_tsdb_init(&tsdb, "log", "fdb_tsdb1", get_time, 128, NULL);
        /* read last saved time for simulated timestamp */
        fdb_tsdb_control(&tsdb, FDB_TSDB_CTRL_GET_LAST_TIME, &counts);

        if (result != FDB_NO_ERR) {
            return -1;
        }

        /* run TSDB sample */
        tsdb_sample(&tsdb);
    }
#endif /* FDB_USING_TSDB */

    return 0;
}

extern "C" void app_main() {
    printf("FlashDB ESP32 SPI Flash Demo\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi, ", chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    uint32_t size_flash_chip;
    esp_flash_get_size(NULL, &size_flash_chip);

    printf("%ldMB %s flash\n", size_flash_chip, (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    flashdb_demo();

    for (int i = 1000; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
