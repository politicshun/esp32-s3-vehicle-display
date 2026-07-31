#pragma once

#include <stdint.h>
#include <stdbool.h>

// Core data share
// 2026-07-31: 차량단(인버터) 설계가 아직 안 끝나서, 이 프로젝트가 CAN 스펙을 먼저 정하고
// 인버터 쪽이 거기 맞추기로 함(사용자 확인). 즉 아래 필드들은 "실차값 추측"이 아니라
// docs/hardware/vehicle.dbc(=Desktop cluster.dbc 최종본)에 우리가 직접 정의한 스펙이다.
// 다만 인버터 쪽 실구현/실기 검증 전이므로 "우리가 정했다" != "실물로 확인됐다"는 여전히 유효.
// InvMsg1(CAN 0x100, 100ms)/InvMsg2(CAN 0x200, 200ms) 2개 메시지에 패킹돼 있다
// (main/twai.c 참고). 2026-07-31 CAN 최적화: 신호를 서브시스템별이 아니라 갱신
// 우선도 기준으로 재배치함 — InvMsg1=우선도 높음(운전 중 계속 바뀌거나 지연 시
// 안전 문제), InvMsg2=우선도 낮음(서서히 바뀌는 상태값). Odometer는 24bit(factor1)
// ->16bit(factor5)로 압축(실사용 상한 30만km 기준, docs/hardware/vehicle.dbc CM_ 참고).
typedef struct {
    int16_t  speed;         // 속도 (km/h, 음수=후진). InvMsg1 byte0, raw-10, [-10|245]
    uint8_t  soc;           // 배터리 잔량 (%). InvMsg2 byte0, [0|100]
    float    pack_volt;     // DC 링크 전압 (V, 정수 해상도). InvMsg2 byte1, [0|80]
    uint8_t  dtc_code;      // 고장 코드 (단일 열거값, 비트마스크 아님). InvMsg1 byte2, [0|255]
    bool     ble_connected; // BLE 연동 상태 (CAN 무관, main/ble.c)
    uint8_t  drive_mode;    // 주행 모드: 0=P,1=R,2=N,3=D (VAL_ 테이블, cluster.dbc). InvMsg1 byte1
    uint32_t odo_km;        // 누적 주행거리. InvMsg2 byte4~5 (16bit, factor5, [0|327675])
    uint16_t range_km;      // 예상 주행가능거리 (0~255km 해상도). InvMsg2 byte3
    float    power_kw;      // 실시간 출력 (kW, 정수 해상도). InvMsg1 byte3
    float    regen_kw;      // 회생제동 출력 (kW, 정수 해상도). InvMsg1 byte4
    int16_t  sys_temp_c;    // 시스템 온도 (degC). InvMsg2 byte2, raw-20, [-20|235] — int8_t 범위(127) 초과라 int16_t 사용
} VehicleData_t;

void vehicle_data_init(void);
void vehicle_data_set(const VehicleData_t *src);
void vehicle_data_get(VehicleData_t *dst);