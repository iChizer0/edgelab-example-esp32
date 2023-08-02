
#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>

#include "edgelab.h"
#include "el_device_esp.h"

extern "C" void app_main(void) {
    // obtain resource
    auto* instance = ReplServer::get_instance();
    auto* device   = Device::get_device();
    auto* camera   = device->get_camera();
    // auto* display = device->get_display();
    auto* serial = device->get_serial();

    // fetch resource
    auto  model_loader = ModelLoader();
    auto& models       = model_loader.get_models();

    // init components
    instance->init();
    camera->init(240, 240);
    // display->init();
    serial->init();

    // register sensor (camera only)
    struct el_sensor_t {
        uint8_t id;
        uint8_t type;
        uint8_t parameters[6];
    };
    std::unordered_map<uint8_t, el_sensor_t> registered_sensors;
    registered_sensors.emplace(0u, el_sensor_t{.id = 0, .type = 0, .parameters = {240, 240, 0, 0, 0, 0}});

    // register algorithms
    register_algorithms();

    // init temporary variables
    el_img_t img;
    auto*    engine{new InferenceEngine<EngineName::TFLite>()};
    auto     model_loaded{false};

    // register repl commands
    instance->register_cmd("ID",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"id\": " << device->get_device_id() << "}\n";
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }),
                           nullptr,
                           nullptr);
    instance->register_cmd("NAME",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"id\": \"" << device->get_device_name() << "\"}\n";
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }),
                           nullptr,
                           nullptr);
    instance->register_cmd("VERSION",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"edgelab-cpp-sdk\": \"" << EL_VERSION
                                  << "\", \"hardware\": " << device->get_chip_revision_id() << "}\n";
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }),
                           nullptr,
                           nullptr);
    instance->register_cmd("RST",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               device->restart();
                               return EL_OK;
                           }),
                           nullptr,
                           nullptr);
    instance->register_cmd("SENSOR",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"count\": " << registered_sensors.size() << ", \"sensors\": [";
                               for (const auto& kv : registered_sensors) {
                                   auto v{std::get<1>(kv)};
                                   os << "{ \"id\": " << unsigned(v.id) << ", \"type\": " << unsigned(v.type)
                                      << ", \"parameters\" :[";
                                   for (size_t i{0}; i < sizeof(v.parameters); ++i)
                                       os << unsigned(v.parameters[i]) << ", ";
                                   os << "]},";
                               }

                               os << "]}\n";
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }),
                           nullptr,
                           nullptr);
    instance->register_cmd("VALGO",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"count\": " << el_registered_algorithms.size() << ", \"algorithms\": [";
                               for (const auto& kv : el_registered_algorithms) {
                                   auto v{std::get<1>(kv)};
                                   os << "{ \"id\": " << unsigned(v.id) << ", \"type\": " << unsigned(v.type)
                                      << ", \"categroy\": " << unsigned(v.categroy)
                                      << ", \"input_type\": " << unsigned(v.input_type) << ", \"parameters\" :[";
                                   for (size_t i{0}; i < sizeof(v.parameters); ++i)
                                       os << unsigned(v.parameters[i]) << ", ";
                                   os << "]},";
                               }

                               os << "]}\n";
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }),
                           nullptr,
                           nullptr);
    instance->register_cmd("VMODEL",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"count\": " << models.size() << ", \"models\": [";
                               for (const auto& m : models)
                                   os << "{\"id\": " << unsigned(m.id) << ", \"type\": " << unsigned(m.type)
                                      << ", \"address\": 0x" << std::hex << unsigned(m.addr_flash) << ", \"size\": 0x"
                                      << unsigned(m.size) << "},";

                               os << "]}\n";
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }),
                           nullptr,
                           nullptr);
    instance->register_cmd(
      "MODEL", "", "MODEL_ID", nullptr, nullptr, el_repl_cmd_write_cb_t([&](int argc, char** argv) -> EL_STA {
          auto model_id{static_cast<uint8_t>(*argv[0] - '0')};
          auto it{std::find_if(models.begin(), models.end(), [&](const auto& v) { return v.id == model_id; })};
          if (it == models.end()) return EL_EINVAL;

          // TODO: call free when switching models, move heap_caps_malloc to port/el_memory or el_system
          static auto* tensor_arena{heap_caps_malloc(it->size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)};
          engine->init(tensor_arena, it->size);

          auto ret{engine->load_model(it->addr_memory, it->size)};
          if (ret == EL_OK) model_loaded = true;
          return ret;
      }));
    instance->register_cmd("SAMPLE",
                           "",
                           "SENSOR_ID,SEND_DATA",
                           nullptr,
                           nullptr,
                           el_repl_cmd_write_cb_t([&](int argc, char** argv) -> EL_STA {
                               auto sensor_id{static_cast<uint8_t>(*argv[0] - '0')};
                               auto it{registered_sensors.find(sensor_id)};
                               if (it == registered_sensors.end()) return EL_EINVAL;

                               auto  os{std::ostringstream(std::ios_base::ate)};
                               char* buffer{nullptr};

                               // camera
                               if (it->second.type == 0u) {
                                   camera->start_stream();
                                   camera->get_frame(&img);
                                   camera->stop_stream();

                                   auto size{img.width * img.height * 3};
                                   auto rgb_img{el_img_t{.data   = new uint8_t[size]{},
                                                         .size   = size,
                                                         .width  = img.width,
                                                         .height = img.height,
                                                         .format = EL_PIXEL_FORMAT_RGB888,
                                                         .rotate = img.rotate}};
                                   auto ret{rgb_to_rgb(&img, &rgb_img)};
                                   if (ret != EL_OK) {
                                       delete[] rgb_img.data;
                                       return ret;
                                   }
                                   auto jpeg_rgb_img{el_img_t{.data   = new uint8_t[size]{},
                                                              .size   = size,
                                                              .width  = rgb_img.width,
                                                              .height = rgb_img.height,
                                                              .format = EL_PIXEL_FORMAT_RGB888,
                                                              .rotate = rgb_img.rotate}};

                                   ret = rgb_to_jpeg(&rgb_img, &jpeg_rgb_img);
                                   if (ret != EL_OK) {
                                       delete[] rgb_img.data;
                                       delete[] jpeg_rgb_img.data;
                                       return ret;
                                   }
                                   delete[] rgb_img.data;

                                   auto data_buffer_size{*argv[1] == '1' ? (4ul * jpeg_rgb_img.size + 2) / 3ul : 0ul};

                                   os << "{\"sensor_id\": " << unsigned(it->first)
                                      << ", \"type\": " << unsigned(it->second.type) << ", \"status\": " << int(EL_OK)
                                      << ", \"size\": " << unsigned(img.size) << ", \"data\": \"";
                                   if (data_buffer_size) {
                                       buffer = new char[data_buffer_size]{};
                                       el_base64_encode(jpeg_rgb_img.data, jpeg_rgb_img.size, buffer);
                                       std::string ss(buffer);
                                       ss.erase(std::find(ss.begin(), ss.end(), '\0'), ss.end());
                                       os << ss.c_str();
                                   }
                                   os << "\"}\n";

                                   delete[] jpeg_rgb_img.data;
                               }

                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               if (buffer) delete[] buffer;
                               return EL_OK;
                           }));
    instance->register_cmd(
      "INVOKE", "", "ALGORITHM_ID", nullptr, nullptr, el_repl_cmd_write_cb_t([&](int argc, char** argv) -> EL_STA {
          auto algorithm_id{static_cast<uint8_t>(*argv[0] - '0')};
          auto it{el_registered_algorithms.find(algorithm_id)};
          if (it == el_registered_algorithms.end() || !model_loaded) return EL_EINVAL;
          auto os{std::ostringstream(std::ios_base::ate)};

          // yolo
          if (it->second.id == 0u) {
              auto* algorithm{new YOLO(engine)};

              auto ret{algorithm->run(&img)};
              if (ret != EL_OK) {
                  delete algorithm;
                  return ret;
              }

              auto preprocess_time{algorithm->get_preprocess_time()};
              auto run_time{algorithm->get_run_time()};
              auto postprocess_time{algorithm->get_postprocess_time()};

              os << "{\"algorithm_id\": " << unsigned(it->first) << ", \"type\": " << unsigned(it->second.type)
                 << ", \"status\": " << int(ret) << ", \"preprocess_time\": " << unsigned(preprocess_time)
                 << ", \"run_time\": " << unsigned(run_time) << ", \"postprocess_time\": " << unsigned(postprocess_time)
                 << ", \"results\": [";
              for (const auto& box : algorithm->get_results())
                  os << "{\"cx\": " << unsigned(box.x) << ", \"cy\": " << unsigned(box.y)
                     << ", \"w\": " << unsigned(box.w) << ", \"h\": " << unsigned(box.h)
                     << ", \"target\": " << unsigned(box.target) << ", \"score\": " << unsigned(box.score) << "}";
              os << "]}\n";

              delete algorithm;
          }

          auto str{os.str()};
          serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
          return EL_OK;
      }));

// enter service pipeline
// TODO: pipeline builder
ServiceLoop:

    instance->loop(serial->get_char());

    goto ServiceLoop;
}
