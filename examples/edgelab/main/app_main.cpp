
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

#include "edgelab.h"
#include "el_device_esp.h"

extern "C" void app_main(void) {
    // fetch hardware resource
    auto* device = Device::get_device();
    auto* camera = device->get_camera();
    // auto* display = device->get_display();
    auto* serial   = device->get_serial();
    auto* instance = ReplServer::get_instance();

    // fetch software resource (TODO: safely delete)
    auto* model_loader{new ModelLoader()};
    auto* engine{new InferenceEngine<EngineName::TFLite>()};

    // init components (TODO: safely deinit)
    camera->init(240, 240);
    // display->init();
    serial->init();
    instance->init();

    // register sensors
    std::unordered_map<uint8_t, el_sensor_t> registered_sensors;
    registered_sensors.emplace(0u, el_sensor_t{.id = 0, .type = 0, .parameters = {240, 240, 0, 0, 0, 0}});  // camera

    // register algorithms
    register_algorithms();

    // init temporary variables
    el_model_t*  current_model{nullptr};
    el_sensor_t* current_sensor{nullptr};
    el_img_t*    current_img{nullptr};

    // register repl commands
    instance->register_cmd("ID",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"id\": \"" << std::uppercase << std::hex << device->get_device_id()
                                  << std::resetiosflags(std::ios_base::basefield)
                                  << "\", \"timestamp\": " << el_get_time_ms() << "}\n";
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
                               os << "{\"id\": \"" << device->get_device_name()
                                  << "\", \"timestamp\": " << el_get_time_ms() << "}\n";
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
                               os << "{\"edgelab-cpp-sdk\": \"v" << EL_VERSION << "\", \"hardware\": \"v"
                                  << unsigned(device->get_chip_revision_id())
                                  << "\", \"timestamp\": " << el_get_time_ms() << "}\n";
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
                                   os << "{\"id\": " << unsigned(kv.second.id)
                                      << ", \"type\": " << unsigned(kv.second.type) << ", \"parameters\" :[";
                                   for (size_t i{0}; i < sizeof(kv.second.parameters); ++i)
                                       os << unsigned(kv.second.parameters[i]) << ", ";
                                   os << "]}, ";
                               }
                               os << "], \"timestamp\": " << el_get_time_ms() << "}\n";
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
                                   os << "{\"id\": " << unsigned(v.id) << ", \"type\": " << unsigned(v.type)
                                      << ", \"categroy\": " << unsigned(v.categroy)
                                      << ", \"input_type\": " << unsigned(v.input_type) << ", \"parameters\" :[";
                                   for (size_t i{0}; i < sizeof(v.parameters); ++i)
                                       os << unsigned(v.parameters[i]) << ", ";
                                   os << "]}, ";
                               }
                               os << "], \"timestamp\": " << el_get_time_ms() << "}\n";
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }),
                           nullptr,
                           nullptr);
    instance->register_cmd(
      "VMODEL",
      "",
      "",
      el_repl_cmd_exec_cb_t([&]() {
          auto  os{std::ostringstream(std::ios_base::ate)};
          auto& models{model_loader->get_models()};
          os << "{\"count\": " << models.size() << ", \"models\": [";
          for (const auto& m : models)
              os << "{\"id\": " << unsigned(m.id) << ", \"type\": " << unsigned(m.type) << ", \"address\": 0x"
                 << std::hex << unsigned(m.addr_flash) << ", \"size\": 0x" << unsigned(m.size) << "},";
          os << std::resetiosflags(std::ios_base::basefield) << "], \"timestamp\": " << el_get_time_ms() << "}\n";
          auto str{os.str()};
          serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
          return EL_OK;
      }),
      nullptr,
      nullptr);
    instance->register_cmd(
      "MODEL", "", "MODEL_ID", nullptr, nullptr, el_repl_cmd_write_cb_t([&](int argc, char** argv) -> EL_STA {
          auto   model_id{static_cast<uint8_t>(*argv[0] - '0')};
          auto   os{std::ostringstream(std::ios_base::ate)};
          auto&  models{model_loader->get_models()};
          auto   it{std::find_if(models.begin(), models.end(), [&](const auto& v) { return v.id == model_id; })};
          EL_STA ret{it != models.end() ? EL_OK : EL_EINVAL};
          if (ret != EL_OK) goto ModelReply;

          // TODO: move heap_caps_malloc to port/el_memory or el_system
          static auto* tensor_arena{heap_caps_malloc(it->size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)};
          if (current_model) memset(tensor_arena, 0, it->size);
          ret = engine->init(tensor_arena, it->size);
          if (ret != EL_OK) goto ModelReply;

          ret = engine->load_model(it->addr_memory, it->size);
          if (ret != EL_OK) goto ModelReply;

          if (current_model) delete current_model;
          current_model = new el_model_t{*it};

      ModelReply:
          os << "{\"model_id\": " << int(current_model ? current_model->id : -1) << ", \"status\": " << int(ret)
             << ", \"timestamp\": " << el_get_time_ms() << "}\n";
          auto str{os.str()};
          serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
          return EL_OK;
      }));
    instance->register_cmd("SAMPLE",
                           "",
                           "SENSOR_ID,SEND_DATA",
                           nullptr,
                           nullptr,
                           el_repl_cmd_write_cb_t([&](int argc, char** argv) -> EL_STA {
                               auto   sensor_id{static_cast<uint8_t>(*argv[0] - '0')};
                               auto   it{registered_sensors.find(sensor_id)};
                               auto   os{std::ostringstream(std::ios_base::ate)};
                               auto   finded{it != registered_sensors.end()};
                               EL_STA ret{finded ? EL_OK : EL_EINVAL};
                               if (ret != EL_OK || argc < 2) goto SampleReplyError;

                               // camera
                               if (it->second.type == 0u) {
                                   camera->start_stream();
                                   if (!current_img) current_img = new el_img_t{.data = nullptr};
                                   camera->get_frame(current_img);
                                   camera->stop_stream();

                                   os << "{\"sensor_id\": " << unsigned(it->first)
                                      << ", \"type\": " << unsigned(it->second.type) << ", \"status\": " << int(ret)
                                      << ", \"size\": " << unsigned(current_img->size) << ", \"data\": \"";
                                   if (*argv[1] == '1') {
                                       auto size{current_img->width * current_img->height * 3};
                                       auto rgb_img{el_img_t{.data   = new uint8_t[size]{},
                                                             .size   = size,
                                                             .width  = current_img->width,
                                                             .height = current_img->height,
                                                             .format = EL_PIXEL_FORMAT_RGB888,
                                                             .rotate = current_img->rotate}};
                                       ret = rgb_to_rgb(current_img, &rgb_img);
                                       if (ret != EL_OK) {
                                           delete[] rgb_img.data;
                                           goto SampleReplyError;
                                       }
                                       auto jpeg_img{el_img_t{.data   = new uint8_t[size]{},
                                                              .size   = size,
                                                              .width  = rgb_img.width,
                                                              .height = rgb_img.height,
                                                              .format = EL_PIXEL_FORMAT_JPEG,
                                                              .rotate = rgb_img.rotate}};
                                       ret = rgb_to_jpeg(&rgb_img, &jpeg_img);
                                       delete[] rgb_img.data;
                                       if (ret != EL_OK) {
                                           delete[] jpeg_img.data;
                                           goto SampleReplyError;
                                       };

                                       auto buffer{new char[(4ul * jpeg_img.size + 2) / 3ul]{}};
                                       el_base64_encode(jpeg_img.data, jpeg_img.size, buffer);
                                       delete[] jpeg_img.data;
                                       std::string ss(buffer);
                                       ss.erase(std::find(ss.begin(), ss.end(), '\0'), ss.end());
                                       os << ss.c_str();
                                       delete[] buffer;
                                   }
                                   os << "\"}, \"timestamp\": " << el_get_time_ms() << "}\n";

                                   goto SampleReply;
                               } else
                                   goto SampleReplyError;

                           SampleReplyError:
                               os << "{\"sensor_id\": " << unsigned(sensor_id)
                                  << ", \"type\": " << int(finded ? it->second.type : -1)
                                  << ", \"status\": " << int(ret) << ", \"timestamp\": " << el_get_time_ms() << "}\n";
                           SampleReply:
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }));
    // instance->register_cmd(
    //   "INVOKE", "", "ALGORITHM_ID", nullptr, nullptr, el_repl_cmd_write_cb_t([&](int argc, char** argv) -> EL_STA {
    //       auto algorithm_id{static_cast<uint8_t>(*argv[0] - '0')};
    //       auto it{el_registered_algorithms.find(algorithm_id)};
    //       if (it == el_registered_algorithms.end() || !model_loaded) return EL_EINVAL;
    //       auto os{std::ostringstream(std::ios_base::ate)};

    //       // yolo
    //       if (it->second.id == 0u) {
    //           auto* algorithm{new YOLO(engine)};

    //           auto ret{algorithm->run(&img)};
    //           if (ret != EL_OK) {
    //               delete algorithm;
    //               return ret;
    //           }

    //           auto preprocess_time{algorithm->get_preprocess_time()};
    //           auto run_time{algorithm->get_run_time()};
    //           auto postprocess_time{algorithm->get_postprocess_time()};

    //           os << "{\"algorithm_id\": " << unsigned(it->first) << ", \"type\": " << unsigned(it->second.type)
    //              << ", \"status\": " << int(ret) << ", \"preprocess_time\": " << unsigned(preprocess_time)
    //              << ", \"run_time\": " << unsigned(run_time) << ", \"postprocess_time\": " << unsigned(postprocess_time)
    //              << ", \"results\": [";

    //           os << edgelab::algorithm::utility::el_results_2_string(algorithm->get_results());
    //           //   for (const auto& box : algorithm->get_results())
    //           //       os << "{\"cx\": " << unsigned(box.x) << ", \"cy\": " << unsigned(box.y)
    //           //          << ", \"w\": " << unsigned(box.w) << ", \"h\": " << unsigned(box.h)
    //           //          << ", \"target\": " << unsigned(box.target) << ", \"score\": " << unsigned(box.score) << "}, ";
    //           os << "]}\n";

    //           delete algorithm;
    //       }

    //       auto str{os.str()};
    //       serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
    //       return EL_OK;
    //   }));

// enter service pipeline (TODO: pipeline builder)
ServiceLoop:
    instance->loop(serial->get_char());

    goto ServiceLoop;
}
