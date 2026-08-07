#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "vehicle_data.h"
#include "ble.h"

extern void bsp_hardware_init(void);
extern void twai_init(void);
extern void twai_rx_task(void *pvParameters);
extern void twai_tx_task(void *pvParameters);
extern void ble_sync_task(void *pvParameters);
extern void lvgl_ui_task(void *pvParameters);

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32-S3 Multi-Core Vehicle Display System...");

    // 2026-08-03: BLE Just Works bonding 키를 NVS에 저장(CONFIG_BT_NIMBLE_NVS_PERSIST=y)하려면
    // ble_init()보다 먼저 NVS가 초기화돼 있어야 함.
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_ret);

    // BSP HW data sharing & Mutex initialization
    bsp_hardware_init();
    vehicle_data_init();
    twai_init();   // twai_rx_task/twai_tx_task가 공유하는 드라이버 lifecycle 락 생성
    ble_init();

    // CORE 0 : CAN & BLE
    // twai_tx(prio 9)는 twai_rx(prio 10)보다 한 단계 낮다 — 수신 지연이 계기 표시에
    // 직결되는 반면, ClusterAlive는 500ms 주기라 몇 ms 밀려도 무방하다.
    xTaskCreatePinnedToCore(twai_rx_task,  "twai_rx",  4096, NULL, 10, NULL, 0);
    xTaskCreatePinnedToCore(twai_tx_task,  "twai_tx",  3072, NULL,  9, NULL, 0);
    xTaskCreatePinnedToCore(ble_sync_task, "ble_sync", 4096, NULL,  5, NULL, 0);

    // CORE 1 : LVGL
    xTaskCreatePinnedToCore(lvgl_ui_task,  "lvgl_ui",  8192, NULL,  5, NULL, 1);
}