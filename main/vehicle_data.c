#include "vehicle_data.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static VehicleData_t g_vehicle_data;
static SemaphoreHandle_t xVehicleMutex = NULL;

void vehicle_data_init(void) {
    if (xVehicleMutex == NULL) {
        xVehicleMutex = xSemaphoreCreateMutex();
    }
}

void vehicle_data_set(const VehicleData_t *src) {
    if (xVehicleMutex != NULL && xSemaphoreTake(xVehicleMutex, portMAX_DELAY) == pdTRUE) {
        g_vehicle_data = *src;
        xSemaphoreGive(xVehicleMutex);
    }
}

void vehicle_data_get(VehicleData_t *dst) {
    if (xVehicleMutex != NULL && xSemaphoreTake(xVehicleMutex, portMAX_DELAY) == pdTRUE) {
        *dst = g_vehicle_data;
        xSemaphoreGive(xVehicleMutex);
    }
}

// UI 하트비트. 뮤텍스를 쓰지 않는 이유는 vehicle_data.h의 선언부 주석 참고.
// 여기(공유 상태 모듈)에 두는 이유: lvgl.c가 쓰고 twai.c가 읽으므로, 둘 중 한쪽에
// 두면 CAN 모듈이 UI 헤더를 포함하거나 그 반대가 되어야 한다.
static volatile uint32_t s_ui_heartbeat_ms = 0;

void ui_heartbeat_mark(void) {
    // twai.c의 now_ms()와 같은 시간 기준(FreeRTOS 틱)을 써야 비교가 성립한다.
    s_ui_heartbeat_ms = (uint32_t)pdTICKS_TO_MS(xTaskGetTickCount());
}

uint32_t ui_heartbeat_last_ms(void) {
    return s_ui_heartbeat_ms;
}

void vehicle_data_set_link_status(bool rx_stale, bool tx_healthy) {
    if (xVehicleMutex != NULL && xSemaphoreTake(xVehicleMutex, portMAX_DELAY) == pdTRUE) {
        g_vehicle_data.can_rx_stale = rx_stale;
        g_vehicle_data.can_tx_healthy = tx_healthy;
        xSemaphoreGive(xVehicleMutex);
    }
}