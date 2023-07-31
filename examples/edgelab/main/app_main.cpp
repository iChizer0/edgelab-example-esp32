#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>

#include "edgelab.h"

extern "C" void app_main(void) {
    ModelLoader model_loader;
    auto        models{model_loader.get_models()};
    printf("found modles: %d\n", models.size());


    for (int i = 1000; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
