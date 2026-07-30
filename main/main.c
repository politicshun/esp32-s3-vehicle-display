#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "vehicle_data.h"
#include "ble.h"

extern void bsp_hardware_init(void);
extern void twai_rx_task(void *pvParameters);
extern void ble_sync_task(void *pvParameters);
extern void lvgl_ui_task(void *pvParameters);

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32-S3 Multi-Core Vehicle Display System...");

    // BSP HW data sharing & Mutex initialization
    bsp_hardware_init();
    vehicle_data_init();
    ble_init();

    // CORE 0 : CAN & BLE
    xTaskCreatePinnedToCore(twai_rx_task,  "twai_rx",  4096, NULL, 10, NULL, 0);
    xTaskCreatePinnedToCore(ble_sync_task, "ble_sync", 4096, NULL,  5, NULL, 0);

    // CORE 1 : LVGL
    xTaskCreatePinnedToCore(lvgl_ui_task,  "lvgl_ui",  8192, NULL,  5, NULL, 1);
}