
#include <stdio.h>

#include "edgelab.h"
#include "el_device_esp.h"

extern "C" void app_main(void) {
    auto* instance = ReplServer::get_instance();
    auto* device   = Device::get_device();
    // auto* camera  = device->get_camera();
    // auto* display = device->get_display();
    auto* serial = device->get_serial();

    instance->init();
    // camera->init(240, 240);
    // display->init();
    serial->init();

    instance->register_cmd("TEST",
                           "",
                           "",
                           el_repl_cmd_exec_cb_t([&instance]() {
                               instance->print_help();
                               return EL_OK;
                           }),
                           el_repl_cmd_read_cb_t([]() { return EL_OK; }),
                           el_repl_cmd_write_cb_t([](int, char**) { return EL_OK; }));

ServiceLoop:

    instance->loop(serial->echo());

    //     auto

    //     // el_img_t img;
    //     // camera->start_stream();
    //     // camera->get_frame(&img);
    //     // display->show(&img);
    //     // camera->stop_stream();

    goto ServiceLoop;
}
