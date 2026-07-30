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