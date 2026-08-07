#include "vehicle_data.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

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

void vehicle_data_set_link_status(bool rx_stale, bool tx_healthy) {
    if (xVehicleMutex != NULL && xSemaphoreTake(xVehicleMutex, portMAX_DELAY) == pdTRUE) {
        g_vehicle_data.can_rx_stale = rx_stale;
        g_vehicle_data.can_tx_healthy = tx_healthy;
        xSemaphoreGive(xVehicleMutex);
    }
}