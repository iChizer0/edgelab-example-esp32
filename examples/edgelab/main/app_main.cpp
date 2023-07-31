#include <driver/usb_serial_jtag.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include "edgelab.h"

extern "C" void app_main(void) {
    ModelLoader model_loader;
    auto        models{model_loader.get_models()};
    printf("found modles: %d\n", models.size());

    usb_serial_jtag_driver_config_t config{
      .tx_buffer_size = 256,
      .rx_buffer_size = 256,
    };

    int ret = usb_serial_jtag_driver_install(&config);
    printf("jtag serial: %d\n", ret);

    char* buf = new char[256];

    for (int i = 1000; i >= 0; i--) {
        // printf(".");
        int size = usb_serial_jtag_read_bytes(buf, 10, 100 / portTICK_PERIOD_MS);
        if (size > 0) {
            printf("\tsize -> %d\n\tdata -> ", size);
            for (int i = 0; i < size; ++i) printf("%c", buf[i]);
            printf("\n");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
