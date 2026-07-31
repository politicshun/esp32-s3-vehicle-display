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
 * 2026-07-31: BLE 테스트를 nRF Connect 같은 범용 스캐너 앱으로 하기로 함(사용자 확인) —
 * packed 바이너리 구조체 대신, 사람이 바로 읽을 수 있는 ASCII 텍스트를 notify/read
 * payload로 보낸다. 앱에서 hex 대신 "UTF-8 String" 보기 옵션으로 바로 실측값 확인 가능.
 * (이전 VehicleBlePacket_t packed 구조체 방식은 커스텀 앱이 파싱해야만 값을 볼 수 있어서,
 * 지금 테스트 단계 목적엔 안 맞아 폐기함 — 필요해지면 다시 도입 가능)
 */
#define BLE_VEHICLE_TEXT_BUF_LEN 64