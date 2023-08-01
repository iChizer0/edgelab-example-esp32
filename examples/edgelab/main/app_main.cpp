
#include <stdio.h>

#include "edgelab.h"
#include "el_device_esp.h"

extern "C" void app_main(void) {
    // obtain resource
    auto* instance = ReplServer::get_instance();
    auto* device   = Device::get_device();
    // auto* camera  = device->get_camera();
    // auto* display = device->get_display();
    auto* serial = device->get_serial();

    // init components
    instance->init();
    // camera->init(240, 240);
    // display->init();
    serial->init();

    // register repl commands
    instance->register_cmd("ID",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               char* data{new char[32]{}};
                               auto  size{sprintf(data, "{\"id\": %ld}\n", device->get_device_id())};
                               serial->write_bytes(data, size);
                               delete[] data;
                               return EL_OK;
                           }),
                           el_repl_cmd_read_cb_t([]() { return EL_OK; }),
                           el_repl_cmd_write_cb_t([](int, char**) { return EL_OK; }));
    instance->register_cmd("NAME",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&]() {
                               char* data{new char[64]{}};
                               auto  size{sprintf(data, "{\"name\": \"%s\"}\n", device->get_device_name())};
                               serial->write_bytes(data, size);
                               delete[] data;
                               return EL_OK;
                           }),
                           el_repl_cmd_read_cb_t([]() { return EL_OK; }),
                           el_repl_cmd_write_cb_t([](int, char**) { return EL_OK; }));
    instance->register_cmd(
      "VERSION",
      "",
      "",
      el_repl_cmd_exec_cb_t([&]() {
          char* data{new char[96]{}};
          auto  size{sprintf(
            data, "{\"edgelab-cpp-sdk\": \"%s\", \"hardware\": %ld}\n", EL_VERSION, device->get_chip_revision_id())};
          serial->write_bytes(data, size);
          delete[] data;
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

    // enter service pipeline
ServiceLoop:
    instance->loop(serial->get_char());

    //     // el_img_t img;
    //     // camera->start_stream();
    //     // camera->get_frame(&img);
    //     // display->show(&img);
    //     // camera->stop_stream();

    goto ServiceLoop;
}
