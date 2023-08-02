
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
      "SAMPLE", "", "SENSOR_ID,SEND_DATA", nullptr, nullptr, el_repl_cmd_write_cb_t([&](int argc, char** argv) {
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

              auto data_buffer_size{*argv[1] == '1' ? 4ul * (img.size / 3ul) : 0ul};

              os << "{\"sensor_id\": " << unsigned(it->first) << ", \"type\": " << unsigned(it->second.type)
                 << ", \"status\": " << int(EL_OK) << ", \"size\": " << unsigned(img.size) << ", \"data\": \"";
              if (data_buffer_size) {
                  buffer = new char[data_buffer_size]{};
                  el_base64_encode(img.data, img.size, buffer);
                  os << buffer;
              }
              os << "\"}\n";
          }

          auto str{os.str()};
          serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
          if (buffer) delete[] buffer;
          return EL_OK;
      }));
    // instance->register_cmd(
    //   "INVOKE", "", "ALGORITHM_ID MODEL_ID", nullptr, nullptr, el_repl_cmd_write_cb_t([&](int argc, char** argv) {
    //       auto sensor_id{static_cast<uint8_t>(*argv[0] - '0')};
    //       auto it{registered_sensors.find(sensor_id)};
    //       if (it == registered_sensors.end()) return EL_EINVAL;

    //       auto os{std::ostringstream(std::ios_base::ate)};

    //       // camera
    //       if (it->second.type == 0u) {
    //           camera->start_stream();
    //           camera->get_frame(&img);
    //           camera->stop_stream();

    //           os << "{\"sensor_id\": " << unsigned(it->first) << ", \"type\": " << unsigned(it->second.type)
    //              << ", \"status\": " << int(EL_OK) << ", \"sample_size\": " << unsigned(img.size) << "}\n";
    //       }

    //       auto str{os.str()};
    //       serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
    //       return EL_OK;
// }));

// enter service pipeline
ServiceLoop:

    instance->loop(serial->get_char());

    //     // el_img_t img;
    // camera->start_stream();
    //     // camera->get_frame(&img);
    //     // display->show(&img);
    //     // camera->stop_stream();

    goto ServiceLoop;
}
