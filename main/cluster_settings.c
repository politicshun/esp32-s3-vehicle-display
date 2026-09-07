#include "cluster_settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static ClusterSettings_t g_settings;
static SemaphoreHandle_t xSettingsMutex = NULL;

void cluster_settings_init(void) {
    if (xSettingsMutex == NULL) {
        xSettingsMutex = xSemaphoreCreateMutex();
    }
    // main/ui/ui_tile_setup.c 빌드 시 위젯 초기값과 동일(슬라이더/스위치/버튼매트릭스
    // 기본 상태) — 여기서 어긋나면 부팅 직후 CAN으로 나가는 값과 화면에 보이는 값이
    // 서로 다르게 시작한다.
    g_settings.brightness_pct = 100;
    g_settings.regen_level = 2;
    g_settings.auto_headlight = false;
    g_settings.auto_day_night = false;
    g_settings.units_mph = false;
}

void cluster_settings_set(const ClusterSettings_t *src) {
    if (xSettingsMutex != NULL && xSemaphoreTake(xSettingsMutex, portMAX_DELAY) == pdTRUE) {
        g_settings = *src;
        xSemaphoreGive(xSettingsMutex);
    }
}

void cluster_settings_get(ClusterSettings_t *dst) {
    if (xSettingsMutex != NULL && xSemaphoreTake(xSettingsMutex, portMAX_DELAY) == pdTRUE) {
        *dst = g_settings;
        xSemaphoreGive(xSettingsMutex);
    }
}
