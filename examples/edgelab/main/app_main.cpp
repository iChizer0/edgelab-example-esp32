#include <driver/usb_serial_jtag.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include "edgelab.h"
#include "el_serial_esp.h"

extern "C" void app_main(void) {
    ModelLoader model_loader;
    auto        models{model_loader.get_models()};
    printf("found modles: %d\n", models.size());

    usb_serial_jtag_driver_config_t config{
      .tx_buffer_size = 256,
      .rx_buffer_size = 256,
    };

    SerialEsp serial;
    serial.init();

    // int ret = usb_serial_jtag_driver_install(&config);
    // printf("jtag serial: %d\n", ret);



    char* buf = new char[256];

    for (;;) {
        printf("waiting for inputs...\n");
        int size = serial.get_line(buf, 10);;
        if (size > 0) {
            printf("\tsize -> %d\n\tdata ->\n\t", size);
            // printf("%s", buf);
            serial.write_bytes(buf, size);
            printf("\n");
        }
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
