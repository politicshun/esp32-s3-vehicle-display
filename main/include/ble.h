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
 * 2026-08-06: 앱 팀용 packed 바이너리(VehicleBlePacket_t) characteristic은 그대로 두고,
 * 보고/데모용으로 nRF Connect 등 범용 스캐너 앱에서 바로 읽을 수 있는 UTF-8 텍스트
 * characteristic을 별도로 추가함(같은 서비스, 새 UUID) — main/ble.c 참고.
 * 2026-07-31에 썼다가 폐기했던 텍스트 포맷과 달리 이번엔 필드 10개 전부 포함.
 * 이 characteristic의 value handle. GATT 서비스 등록 완료 후 채워짐.
 */
extern uint16_t g_vehicle_text_chr_val_handle;

/*
 * 2026-08-03: 커스텀 앱 개발 단계로 넘어가면서, 2026-07-31에 테스트용으로 썼던
 * ASCII 텍스트 payload(nRF Connect 등 범용 스캐너용)를 폐기하고 packed 바이너리로
 * 되돌림 — docs/design/ble-gatt.md의 앱 팀 핸드오프 스펙과 1:1로 맞춘 최종 포맷.
 *
 * 필드는 VehicleData_t와 동일한 의미/실제값(CAN raw 스케일 그대로 노출하지 않음 —
 * 이미 실제 단위로 환산된 값)이지만, 아래 이유로 그대로 보내지 않고 별도 정의함:
 *   1) VehicleData_t는 컴파일러가 자동으로 padding을 넣을 수 있어 MCU와 스마트폰
 *      (다른 아키텍처/컴파일러)에서 struct 크기·정렬이 달라질 수 있음
 *   2) float 대신 고정폭 정수만 사용 — pack_volt/power_kw/regen_kw는 CAN 스펙상
 *      원래 정수 해상도(factor1)라 정밀도 손실 없이 uint8_t로 충분함
 *      (docs/hardware/cluster.dbc, main/twai.c 참고)
 *   3) ble_connected는 내부 상태값이라 무선으로 보낼 필요 없음
 *   4) drive_mode/odo_km/range_km/power_kw/regen_kw/sys_temp_c는 이전 텍스트
 *      포맷에 빠져있었는데(HARNESS-TODO였음), 이번에 전부 포함시킴
 *
 * 멀티바이트 필드(int16_t/uint16_t/uint32_t)는 ESP32 네이티브 바이트오더인
 * little-endian 그대로 나간다 — 앱 쪽 파싱 시 명시적으로 little-endian으로
 * 디코드해야 함(iOS/Android 실기기 자체는 보통 little-endian이라 값 자체는
 * 그대로 재사용 가능하지만, 파서 코드에서 바이트오더를 가정에 맡기지 말고
 * 명시할 것 — 이 프로젝트가 정한 계약).
 */
#define BLE_PROTOCOL_VERSION 1  /* payload 필드 추가/변경 시 증가 — 앱이 구버전 펌웨어 구분용 */

typedef struct __attribute__((packed)) {
    uint8_t  proto_version;  /* BLE_PROTOCOL_VERSION */
    int16_t  speed;          /* km/h, 음수=후진 */
    uint8_t  soc;            /* % */
    uint8_t  pack_volt;      /* V, 정수 해상도 */
    uint8_t  dtc_code;       /* 고장 코드 (단일 열거값, 비트마스크 아님) — 코드값 매핑은
                               * docs/design/ble-gatt.md 참고(HARNESS-TODO: 아직 미확정) */
    uint8_t  drive_mode;     /* 0=P 1=R 2=N 3=D */
    uint32_t odo_km;         /* 누적 주행거리 (km, 실제값) */
    uint16_t range_km;       /* 예상 주행가능거리 (km) */
    uint8_t  power_kw;       /* 실시간 출력 (kW, 정수 해상도) */
    uint8_t  regen_kw;       /* 회생제동 출력 (kW, 정수 해상도) */
    int16_t  sys_temp_c;     /* 시스템 온도 (degC) */
} VehicleBlePacket_t;