
#include <cstring>
#include <sstream>
#include <string>

#include "edgelab.h"
#include "el_device_esp.h"

extern "C" void app_main(void) {
    // obtain resource
    auto* instance = ReplServer::get_instance();
    auto* device   = Device::get_device();
    // auto* camera  = device->get_camera();
    // auto* display = device->get_display();
    auto* serial = device->get_serial();

    // fetch resource
    auto  model_loader = ModelLoader();
    auto& models       = model_loader.get_models();

    // init components
    instance->init();
    // camera->init(240, 240);
    // display->init();
    serial->init();

    // register algorithms
    el_registered_algorithms.emplace_back(
      el_algorithm_t{.type = 0, .categroy = 0, .parameters = {50, 45, 0, 0, 0, 0}});  // YOLO
    el_registered_algorithms.emplace_back(
      el_algorithm_t{.type = 1, .categroy = 1, .parameters = {80, 0, 0, 0, 0, 0}});   // PFLD
    el_registered_algorithms.emplace_back(
      el_algorithm_t{.type = 1, .categroy = 1, .parameters = {0, 0, 0, 0, 0, 0}});    // FOMO

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
                           {},
                           {});
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
                           {},
                           {});
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
                           {},
                           {});
    instance->register_cmd("RST",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               device->restart();
                               return EL_OK;
                           }),
                           {},
                           {});
    instance->register_cmd("VALGO",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"count\": " << el_registered_algorithms.size() << ", \"models\": [";
                               for (const auto& a : el_registered_algorithms) {
                                   os << "{\"type\": " << unsigned(a.type) << ", \"categroy\": " << unsigned(a.categroy)
                                      << ", \"parameters\" :[";
                                   for (size_t i{0}; i < sizeof(a.parameters); ++i)
                                       os << unsigned(a.parameters[i]) << ", ";
                                   os << "]},";
                               }

                               os << "]}\n";
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }),
                           {},
                           {});
    instance->register_cmd("VMODEL",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"count\": " << models.size() << ", \"models\": [";
                               for (const auto& m : models)
                                   os << "{\"type\": " << unsigned(m.type) << ", \"id\": " << unsigned(m.id)
                                      << ", \"size\": 0x" << std::hex << unsigned(m.size) << ", \"address\": 0x"
                                      << unsigned(m.addr_flash) << "},";

                               os << "]}\n";
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }),
                           {},
                           {});

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
