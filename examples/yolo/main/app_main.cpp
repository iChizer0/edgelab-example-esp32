#include <inttypes.h>
#include <stdio.h>

#include "edgelab.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// // #include "yolo_model_data.h"

#define kTensorArenaSize (1024 * 768)

uint16_t color[] = {
  0x0000,
  0x03E0,
  0x001F,
  0x7FE0,
  0xFFFF,
};

extern "C" void app_main(void) {
    // Device*  device  = Device::get_device();
    // Display* display = device->get_display();
    // Camera*  camera  = device->get_camera();

    // camera->init(240, 240);
    // display->init();

    ModelLoader model_loader;
    auto models{model_loader.get_models()};

    printf("found modles: %d\n", models.size());



    for (int i = 1000; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

//     auto* engine       = new InferenceEngine<EngineName::TFLite>();
//     auto* tensor_arena = heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
//     engine->init(tensor_arena, kTensorArenaSize);

//     // algorithm->init();

//     // const uint32_t* flash_addr = (const uint32_t*)0x400000;
//     // for (size_t i = 0; i < 4; ++i) {
//     //   uint32_t buf = *flash_addr++;
//     //   printf("p-> %p, data -> %ld", flash_addr, buf);
//     // }

//     // printf("model addr -> %p\n", g_yolo_model_data);
//     static const esp_partition_t* p =
//       esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "models");

//     printf("mem addr -> %p, flash addr -> %#08x\n", p, int(p->address));

//     const uint8_t*          flash_mapped_ptr;
//     esp_err_t               ret;
//     spi_flash_mmap_handle_t handle;
//     ret = spi_flash_mmap(0x400000, 0x800000 - 0x400000, SPI_FLASH_MMAP_DATA, (const void**)&flash_mapped_ptr, &handle);

//     const uint8_t* data_ptr = flash_mapped_ptr;
//     // static const size_t model_size = 0x41400;  // 267264

//     if (ret == ESP_OK) {
//         printf("mmap ptr -> %p\n", data_ptr);

//         for (int i = 0; i < 64; ++i) {
//             printf("%#02x\t", int(*(data_ptr + i)));
//             if ((i + 1) % 8 == 0) printf("\n");
//         }
//         printf("\n");

//         uint8_t model = 0b00000000;  // pre 1
//         uint8_t index = 0;

//         const int32_t model_max_size = 0x100000;

// // #define MODEL_MAGIC_NUM 0x4C485400
// #define MODEL_MAGIC_NUM 0x4C485400

//         for (uint8_t i = 0; i < 4; i++) {
//             int offset      = i * model_max_size;
//             index               = i + 1;
//             const uint8_t* model_addr = data_ptr + offset;

//             if (((*(uint32_t*)model_addr) & 0xFFFFFF00) == MODEL_MAGIC_NUM) {
//                 index = (*(uint32_t*)model_addr) & 0xFF;  // get index form model header
//                 model_addr += 4;
//                 printf("ok!\n");
//             }
//             else {
//                 printf("error loading model -> \n\tmodel_addr: %p\n\toffset: %#08x\n\tmagic (reversed): %#08x\n",
//                        model_addr,
//                        int(offset),
//                        int(((*(uint32_t*)model_addr) & 0xFFFFFF00)));

//                 continue;
//                 // for (int i = 1000; i >= 0; i--) {
//             }

//             printf("p -> %p\n", model_addr);

//             engine->load_model(model_addr, model_max_size);
//             model |= 1 << (index);

//             // if (::tflite::GetModel((void*)model_addr)->version() == TFLITE_SCHEMA_VERSION) {
//             //     model |= 1 << (index);  // if model vaild, then set bit
//             // }
//         }

//     }                //     printf("Restarting in %d seconds...\n", i);
//                 //     vTaskDelay(1000 / portTICK_PERIOD_MS);
//                 // }
//                 // printf("Restarting now.\n");
//                 // fflush(stdout);
//                 // esp_restart();
//             }

//             printf("p -> %p\n", model_addr);

//             engine->load_model(model_addr, model_max_size);
//             model |= 1 << (index);

//             // if (::tflite::GetModel((void*)model_addr)->version() == TFLITE_SCHEMA_VERSION) {
//             //     model |= 1 << (index);  // if model vaild, then set bit
//             // }
//         }

//     }
//     }

// #define TEST_MAIN

// #ifdef TEST_MAIN
//     auto* algorithm = new YOLO(engine);

//     printf("done\n");
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

//         // {
//         //     el_box_t b{
//         //       .x      = 1,
//         //       .y      = 2,
//         //       .w      = 3,
//         //       .h      = 4,
//         //       .score  = 6,
//         //       .target = 5,
//         //     };
//         //     // el_printf("\tp_b -> %p\n", &b);
//         //     el_printf("\ttest0 -> cx_cy_w_h: [%d, %d, %d, %d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//         //     // el_printf("\ttest1 -> cx_cy_w_h: [%3d, %d, %d, %d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//         //     // el_printf("\ttest1 -> cx_cy_w_h: [%3d, %d, %d, %d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//         //     // el_printf("\ttest2 -> cx_cy_w_h: [%3d, %3d, %d, %d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//         //     // el_printf("\ttest3 -> cx_cy_w_h: [%3d, %3d, %3d, %d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//         //     el_printf("\ttest4 -> cx_cy_w_h: [%3d, %3d, %3d, %3d] t: [%d] s: [%d]\n", b.x, b.y, b.w, b.h, b.target, b.score);
//         // }

//         el_printf("preprocess: %d, run: %d, postprocess: %d\n", preprocess_time, run_time, postprocess_time);
//         display->show(&img);
//         camera->stop_stream();
//     }
// #endif
// }

// #include <stdio.h>

// #include "el_data.h"
// #include "esp_chip_info.h"
// #include "esp_flash.h"
// #include "esp_system.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"
// #include "freertos/task.h"
// #include "nvs_flash.h"

// extern "C" void app_main() {
//     printf("==================== test flash ====================\n");

//     static int boot_count  = 0;
//     static int boot_time[] = {1, 2, 3, 4};

//     static struct fdb_default_kv_node default_kv_table[] = {
//       {"username", (void*)("armink"), 0},              /* string KV */
//       {"password", (void*)("123456"), 0},              /* string KV */
//       {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
//       {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
//     };

//     struct fdb_default_kv default_kv;
//     default_kv.kvs = default_kv_table;
//     default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

//     edgelab::data::PersistentMap map(CONFIG_EL_DATA_PERSISTENT_MAP_NAME, CONFIG_EL_DATA_PERSISTENT_MAP_PATH, &default_kv);
//     // map.destory();
//     int i = 0;


//     printf("quering init ->\n");
//     for (const auto& a : map) printf("\t%s\n", a.name);


//     printf("test %d\n", i);
//     auto t0 = el_make_map_kv("t0", 0);
//     map << t0;
//     printf("quering %d ->\n", i);
//     for (const auto& a : map) printf("\t%s\n", a.name);

//     printf("test %d\n", ++i);
//     map.erase("t0");
//     printf("quering %d ->\n", i);
//     for (const auto& a : map) printf("\t%s\n", a.name);

//     printf("test %d\n", ++i);
//     map.erase("not exist");
//     printf("quering %d ->\n", i);
//     for (const auto& a : map) printf("\t%s\n", a.name);

//     printf("test %d\n", ++i);
//     map["t0"];

//     printf("test %d\n", ++i);
//     auto t1 = el_make_map_kv("t0", 0);
//     map >> t1;
//     printf("\t value: %d\n", t1.value);

//     printf("test %d\n", ++i);
//     auto t2 = el_make_map_kv("boot_count", 0);
//     map >> t2;
//     printf("\t boot_count: %d\n", t2.value);

//     printf("test %d\n", ++i);
//     auto t3 = el_make_map_kv("boot_count", t2.value + 1);
//     map << t3;
//     printf("\t setting boot_count to %d\n", t3.value);

//     printf("test %d\n", ++i);
//     auto t4 = el_make_map_kv("boot_count", 0);
//     map >> t4;
//     printf("\t boot_count: %d\n", t4.value);

//     printf("test %d\n", ++i);
//     auto t5 = el_make_map_kv("string", "hello");
//     map << t5;
//     for (const auto& a : map) printf("\t%s\n", a.name);
          
//     printf("test %d\n", ++i);
//     auto t6 = el_make_map_kv("string", "");
//     map >> t6;
//     printf("\t string: %s\n", t6.value);

//     printf("test %d\n", ++i);
//     auto t7 = el_make_map_kv("string", "hello world!");
//     map << t7;
//     printf("\t string: %s\n", t6.value);

//     printf("test %d\n", ++i);
//     auto t8 = el_make_map_kv("string", "");
//     map >> t8;
//     printf("\t string: %s\n", t8.value);

//     for (int i = 1000; i >= 0; i--) {
//         printf("Restarting in %d seconds...\n", i);
//         vTaskDelay(1000 / portTICK_PERIOD_MS);
//     }
//     printf("Restarting now.\n");
//     fflush(stdout);
//     esp_restart();
// }
