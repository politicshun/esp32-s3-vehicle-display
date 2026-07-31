#pragma once

#include <stdint.h>
#include <stdbool.h>

// Core data share
// HARNESS-TODO: drive_mode/odo_km/range_km/power_kw/regen_kw/sys_temp_c는
// docs/hardware/vehicle.dbc 기준 CAN ID 0x302~0x307 placeholder다.
// 실차 DBC로 확정되지 않았으니, 이 필드들의 값/스케일을 신뢰하고 쓰면 안 된다.
typedef struct {
    uint16_t speed;         // 속도 (km/h)
    uint8_t  soc;           // 배터리 잔량 (%)
    float    pack_volt;     // 배터리 전압 (V)
    uint16_t dtc_code;      // 고장 코드
    bool     ble_connected; // BLE 연동 상태
    uint8_t  drive_mode;    // 주행 모드: 0=P,1=R,2=N,3=D (placeholder, CAN 0x302)
    uint32_t odo_km;        // 누적 주행거리 (placeholder, CAN 0x303)
    uint16_t range_km;      // 예상 주행가능거리 (placeholder, CAN 0x304)
    float    power_kw;      // 실시간 출력 (placeholder, CAN 0x305)
    float    regen_kw;      // 회생제동 출력 (placeholder, CAN 0x306)
    int8_t   sys_temp_c;    // 시스템/팩 온도 (placeholder, CAN 0x307)
} VehicleData_t;

void vehicle_data_init(void);
void vehicle_data_set(const VehicleData_t *src);
void vehicle_data_get(VehicleData_t *dst);