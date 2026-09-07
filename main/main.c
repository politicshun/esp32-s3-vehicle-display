#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "vehicle_data.h"
#include "cluster_settings.h"
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
    cluster_settings_init();
    twai_init();   // twai_rx_task/twai_tx_task가 공유하는 드라이버 lifecycle 락 생성
    ble_init();

    // CORE 0 : CAN & BLE
    // twai_tx(prio 9)는 twai_rx(prio 10)보다 한 단계 낮다 — 수신 지연이 계기 표시에
    // 직결되는 반면, ClusterAlive는 500ms 주기라 몇 ms 밀려도 무방하다.
    xTaskCreatePinnedToCore(twai_rx_task,  "twai_rx",  4096, NULL, 10, NULL, 0);
    xTaskCreatePinnedToCore(twai_tx_task,  "twai_tx",  3072, NULL,  9, NULL, 0);
    xTaskCreatePinnedToCore(ble_sync_task, "ble_sync", 4096, NULL,  5, NULL, 0);

    // CORE 1 : LVGL
    // 2026-09-03 실기기 확인: Voltline 5탭 UI(중첩 위젯-빌더 헬퍼 호출 체인이 이전 4탭
    // 구조보다 훨씬 깊음)에서 8192bytes 내부 SRAM 스택으로는 ui_init() 도중 스택이 넘쳐
    // LVGL 내부 힙(lv_mem, tlsf 풀)이 깨지는 것으로 추정되는 hang 재현(lv_obj_create ->
    // lv_mem_alloc 안에서 영구 정지, task watchdog이 IDLE1 미응답으로 5초마다 반복
    // 트리거 — 매번 완전히 동일한 PC/SP, 진행 없음). Xtensa 윈도우드 레지스터 ABI는
    // 콜스택 프레임당 스택 소모가 큰 편이라 원인으로 유력함.
    //
    // 그냥 xTaskCreatePinnedToCore()로 스택만 16384로 올리면 hang은 없어지지만(실기기
    // 확인) 내부 SRAM에서 8KB를 더 떼어가면서 largest_free_block이 79872->71680으로
    // 줄어 LVGL 드로우 버퍼(76800 bytes, lvgl.c)가 못 들어간다 — largest_block과
    // drawbuf 요구량 사이 여유가 원래 3072 bytes뿐이라(2026-08-06 문서화된 파편화
    // 이력) 내부 SRAM 쪽 정적 예약을 조금만 늘려도 곧바로 깨진다. 그래서 스택 자체를
    // PSRAM에 둬서(MALLOC_CAP_SPIRAM) 내부 SRAM 예산을 전혀 안 건드리게 한다 — TCB만
    // 내부 RAM에 남고(xTaskCreatePinnedToCoreWithCaps 요구사항), 렌더 데이터(드로우
    // 버퍼)는 여전히 SRAM에 그대로라 lvgl.c의 PSRAM-느림 이슈(2026-08-05)와는 무관.
    TaskHandle_t lvgl_task_handle = NULL;
    BaseType_t lvgl_task_ok = xTaskCreatePinnedToCoreWithCaps(
        lvgl_ui_task, "lvgl_ui", 16384, NULL, 5, &lvgl_task_handle, 1, MALLOC_CAP_SPIRAM);
    if (lvgl_task_ok != pdPASS) {
        ESP_LOGE(TAG, "lvgl_ui 태스크 생성 실패(PSRAM 스택) — 시스템 중단");
    }
}