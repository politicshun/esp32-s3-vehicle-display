#pragma once

#include <stdint.h>
#include <stdbool.h>

/*
 * BLE(NimBLE) GAP + GATT 서버 초기화
 * app_main()에서 vehicle_data_init() 이후, ble_sync_task 생성 전에 1회 호출.
 */
void ble_init(void);

/*
 * 현재 연결 핸들 (연결 안 됨 = BLE_HS_CONN_HANDLE_NONE)
 * ble_sync_task에서 notify를 보낼 때 사용.
 */
extern uint16_t g_ble_conn_handle;

/*
 * vehicle 데이터 characteristic의 value handle.
 * GATT 서비스 등록 완료 후 채워짐. notify 시 사용.
 */
extern uint16_t g_vehicle_chr_val_handle;

/*
 * 실제 무선 구간(BLE)으로 내려보내는 패킷 포맷.
 * VehicleData_t를 그대로 보내지 않는 이유는 아래 두 가지:
 *   1) VehicleData_t는 컴파일러가 자동으로 padding을 넣을 수 있어
 *      MCU와 스마트폰(다른 아키텍처/컴파일러)에서 struct 크기·정렬이 달라질 수 있음
 *   2) ble_connected는 내부 상태값이라 굳이 무선으로 보낼 필요가 없음
 * 그래서 필요한 필드만 골라 packed 구조체로 별도 정의함.
 */
typedef struct __attribute__((packed)) {
    uint16_t speed;      // km/h
    uint8_t  soc;         // %
    float    pack_volt;   // V
    uint16_t dtc_code;    // 고장 코드
} VehicleBlePacket_t;