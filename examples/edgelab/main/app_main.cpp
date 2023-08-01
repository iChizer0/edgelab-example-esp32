
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
                           el_repl_cmd_read_cb_t([]() { return EL_OK; }),
                           el_repl_cmd_write_cb_t([](int, char**) { return EL_OK; }));
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
                           el_repl_cmd_read_cb_t([]() { return EL_OK; }),
                           el_repl_cmd_write_cb_t([](int, char**) { return EL_OK; }));
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
                           el_repl_cmd_read_cb_t([]() { return EL_OK; }),
                           el_repl_cmd_write_cb_t([](int, char**) { return EL_OK; }));
    instance->register_cmd("RST",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               device->restart();
                               return EL_OK;
                           }),
                           el_repl_cmd_read_cb_t([]() { return EL_OK; }),
                           el_repl_cmd_write_cb_t([](int, char**) { return EL_OK; }));
    instance->register_cmd("VMODEL",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               auto os{std::ostringstream(std::ios_base::ate)};
                               os << "{\"count\": " << models.size() << ", \"models\": [";
                               for (const auto& m : models)
                                   os << "{\"type\": " << unsigned(m.type) << ", \"index\": " << unsigned(m.index)
                                      << ", \"size\": " << unsigned(m.size) << ", \"address\": 0x" << std::hex
                                      << unsigned(m.addr_flash) << "},";

                               os << "]}\n";
                               auto str{os.str()};
                               serial->write_bytes(str.c_str(), std::strlen(str.c_str()));
                               return EL_OK;
                           }),
                           el_repl_cmd_read_cb_t([]() { return EL_OK; }),
                           el_repl_cmd_write_cb_t([](int, char**) { return EL_OK; }));

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
