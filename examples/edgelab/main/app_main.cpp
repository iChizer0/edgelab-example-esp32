
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

    // fetch resource (TODO: safely delete)
    auto* model_loader{new ModelLoader()};
    auto* engine{new InferenceEngine<EngineName::TFLite>()};

    // register algorithms
    register_algorithms();

    // register sensors
    std::unordered_map<uint8_t, el_sensor_t> el_registered_sensors;
    el_registered_sensors.emplace(0u, el_sensor_t{.id = 0, .type = 0, .parameters = {240, 240, 0, 0, 0, 0}});  // camera

    // init persistent map (TODO: move fdb func call to persistent map)
    int32_t        boot_count{0};
    el_algorithm_t default_algorithm{el_registered_algorithms[0]};  // yolo
    el_sensor_t    default_sensor{el_registered_sensors[0]};        // camera
    uint8_t        default_model_id{0};                             // yolo model

    struct fdb_default_kv_node el_default_persistent_map[]{
      {new char[]{"boot_count"}, &boot_count, sizeof(boot_count)},
      {new char[]{"algorithm_config"}, &default_algorithm, sizeof(default_algorithm)},
      {new char[]{"sensor_config"}, &default_sensor, sizeof(default_sensor)},
      {new char[]{"model_id"}, &default_model_id, sizeof(default_model_id)},
    };
    fdb_default_kv default_kv{.kvs = el_default_persistent_map,
                              .num = sizeof(el_default_persistent_map) / sizeof(el_default_persistent_map[0])};
    auto*          persistent_map{
      new PersistentMap(CONFIG_EL_DATA_PERSISTENT_MAP_NAME, CONFIG_EL_DATA_PERSISTENT_MAP_PATH, &default_kv)};

    // init temporary variables (TODO: current sensors should be a list)
    const el_algorithm_t* current_algorithm{nullptr};
    const el_model_t*     current_model{nullptr};
    const el_sensor_t*    current_sensor{nullptr};
    el_img_t*             current_img{nullptr};

    // fetch configs from persistent map (TODO: value check)
    {
        auto boot_count_kv{el_make_map_kv("boot_count", boot_count)};
        auto algorithm_config_kv{el_make_map_kv("algorithm_config", default_algorithm)};
        auto sensor_config_kv{el_make_map_kv("sensor_config", default_sensor)};
        auto model_id_kv{el_make_map_kv("model_id", default_model_id)};

        *persistent_map >> boot_count_kv >> algorithm_config_kv >> sensor_config_kv >> model_id_kv;
        *persistent_map << el_make_map_kv("boot_count", static_cast<decltype(boot_count)>(boot_count_kv.value + 1u));

        el_registered_algorithms[algorithm_config_kv.value.id] = algorithm_config_kv.value;
        el_registered_sensors[sensor_config_kv.value.id]       = sensor_config_kv.value;
        current_algorithm = &el_registered_algorithms[algorithm_config_kv.value.id];
        current_model     = &model_loader->get_models()[model_id_kv.value];
        current_sensor    = &el_registered_sensors[sensor_config_kv.value.id];
    }

    // init components (TODO: safely deinit)
    // display->init();
    serial->init();
    instance->init();

    // register repl commands
    instance->register_cmd("ID",
                           "Get device ID",
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
                           "Get device name",
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
                           "Get version details",
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
                           "Reboot device",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               device->restart();
                               return EL_OK;
                           }),
                           nullptr,
                           nullptr);
    instance->register_cmd("SENSOR?",
                           "Get available sensors",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"count\": " << el_registered_sensors.size() << ", \"sensors\": [";
                               for (const auto& kv : el_registered_sensors) {
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
    instance->register_cmd(
      "SENSOR",
      "Enable/Disable a sensor by sensor ID",
      "SENSOR_ID,ENABLE_SENSOR",
      nullptr,
      nullptr,
      el_repl_cmd_write_cb_t([&](int argc, char** argv) -> EL_STA {
          auto   sensor_id{static_cast<uint8_t>(std::atoi(argv[0]))};
          auto   enable{std::atoi(argv[1]) != 0 ? true : false};
          auto   it{el_registered_sensors.find(sensor_id)};
          auto   os{std::ostringstream(std::ios_base::ate)};
          auto   found{it != el_registered_sensors.end()};
          EL_STA ret{found ? EL_OK : EL_EINVAL};
          if (ret != EL_OK) [[unlikely]]
              goto SensorReply;

          // camera
          if (it->second.id == 0 && camera) {
              ret = enable ? camera->init(it->second.parameters[0], it->second.parameters[1]) : camera->deinit();
              if (ret != EL_OK) goto SensorReply;
              current_sensor = enable ? &el_registered_sensors[it->first] : nullptr;
          } else
              ret = EL_ENOTSUP;

      SensorReply:
          os << "{\"sensor_id\": " << unsigned(sensor_id) << ", \"enabled\": " << unsigned(current_sensor ? 1 : 0)
             << ", \"status\": " << int(ret) << ", \"timestamp\": " << el_get_time_ms() << "}\n";
          auto str{os.str()};
          serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
          return EL_OK;
      }));
    instance->register_cmd("ALGO?",
                           "Get available algorithms",
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
      "MODEL?",
      "Get available models",
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
      "MODEL",
      "Load a model by model ID",
      "MODEL_ID",
      nullptr,
      nullptr,
      el_repl_cmd_write_cb_t([&](int argc, char** argv) -> EL_STA {
          auto   model_id{static_cast<uint8_t>(std::atoi(argv[0]))};
          auto   os{std::ostringstream(std::ios_base::ate)};
          auto&  models{model_loader->get_models()};
          auto   it{std::find_if(models.begin(), models.end(), [&](const auto& v) { return v.id == model_id; })};
          EL_STA ret{it != models.end() ? EL_OK : EL_EINVAL};
          if (ret != EL_OK) goto ModelReply;

          // TODO: move heap_caps_malloc to port/el_memory or el_system
          static auto* tensor_arena{heap_caps_malloc(it->size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)};
          memset(tensor_arena, 0, it->size);
          ret = engine->init(tensor_arena, it->size);
          if (ret != EL_OK) goto ModelInitError;

          ret = engine->load_model(it->addr_memory, it->size);
          if (ret != EL_OK) goto ModelInitError;

          current_model = &models[model_id];
          goto ModelReply;

      ModelInitError:
          current_model = nullptr;

      ModelReply:
          os << "{\"model_id\": " << model_id << ", \"status\": " << int(ret) << ", \"timestamp\": " << el_get_time_ms()
             << "}\n";
          auto str{os.str()};
          serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
          return EL_OK;
      }));
    instance->register_cmd("SAMPLE",
                           "Sample data from a sensor by sensor ID",
                           "SENSOR_ID,SEND_DATA",
                           nullptr,
                           nullptr,
                           el_repl_cmd_write_cb_t([&](int argc, char** argv) -> EL_STA {
                               auto   sensor_id{static_cast<uint8_t>(std::atoi(argv[0]))};
                               auto   send_data{std::atoi(argv[1]) != 0 ? true : false};
                               auto   it{el_registered_sensors.find(sensor_id)};
                               auto   os{std::ostringstream(std::ios_base::ate)};
                               auto   finded{it != el_registered_sensors.end()};
                               EL_STA ret{finded ? EL_OK : EL_EINVAL};
                               // TODO: lookup from current sensor list
                               if (ret != EL_OK || !current_sensor || current_sensor->id != sensor_id) [[unlikely]]
                                   goto SampleReplyError;

                               // camera (TODO: get sensor and allocate buffer from sensor list)
                               if (it->second.type == 0u && camera && camera->is_present()) {
                                   ret = camera->start_stream();
                                   if (ret != EL_OK) goto SampleReplyError;
                                   if (!current_img) [[unlikely]]
                                       current_img = new el_img_t{.data   = nullptr,
                                                                  .size   = 0,
                                                                  .width  = 0,
                                                                  .height = 0,
                                                                  .format = EL_PIXEL_FORMAT_UNKNOWN,
                                                                  .rotate = EL_PIXEL_ROTATE_UNKNOWN};
                                   ret = camera->get_frame(current_img);
                                   if (ret != EL_OK) goto SampleReplyError;
                                   ret = camera->stop_stream();
                                   if (ret != EL_OK) goto SampleReplyError;

                                   os << "{\"sensor_id\": " << unsigned(it->first)
                                      << ", \"type\": " << unsigned(it->second.type) << ", \"status\": " << int(ret)
                                      << ", \"size\": " << unsigned(current_img->size) << ", \"data\": \"";
                                   if (send_data) {
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
                                   os << "\", \"timestamp\": " << el_get_time_ms() << "}\n";

                                   goto SampleReply;
                               } else
                                   goto SampleReplyError;

                           SampleReplyError:
                               os << "{\"sensor_id\": " << unsigned(sensor_id)
                                  << ", \"type\": " << unsigned(finded ? it->second.type : 0xff)
                                  << ", \"status\": " << int(ret)
                                  << ", \"size\": 0, \"data\": \"\", \"timestamp\": " << el_get_time_ms() << "}\n";
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
