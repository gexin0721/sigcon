#include "main.h"

static const char* TAG = "MAIN";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "sigcon firmware bootstrap started");

    while (true) {
        ESP_LOGI(TAG, "main loop idle");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
