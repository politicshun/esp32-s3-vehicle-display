#pragma once

#include <stdint.h>
#include <stdbool.h>

// Core data share
typedef struct {
    uint16_t speed;         // 속도 (km/h)
    uint8_t  soc;           // 배터리 잔량 (%)
    float    pack_volt;     // 배터리 전압 (V)
    uint16_t dtc_code;      // 고장 코드
    bool     ble_connected; // BLE 연동 상태
} VehicleData_t;

void vehicle_data_init(void);
void vehicle_data_set(const VehicleData_t *src);
void vehicle_data_get(VehicleData_t *dst);