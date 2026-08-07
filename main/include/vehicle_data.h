#pragma once

#include <stdint.h>
#include <stdbool.h>

// Core data share
// 2026-07-31: 차량단(인버터) 설계가 아직 안 끝나서, 이 프로젝트가 CAN 스펙을 먼저 정하고
// 인버터 쪽이 거기 맞추기로 함(사용자 확인). 즉 아래 필드들은 "실차값 추측"이 아니라
// docs/hardware/cluster.dbc(=Desktop cluster.dbc 최종본)에 우리가 직접 정의한 스펙이다.
// 다만 인버터 쪽 실구현/실기 검증 전이므로 "우리가 정했다" != "실물로 확인됐다"는 여전히 유효.
// InvMsg1(CAN 0x100, 100ms)/InvMsg2(CAN 0x200, 200ms) 2개 메시지에 패킹돼 있다
// (main/twai.c 참고). 2026-07-31 CAN 최적화: 신호를 서브시스템별이 아니라 갱신
// 우선도 기준으로 재배치함 — InvMsg1=우선도 높음(운전 중 계속 바뀌거나 지연 시
// 안전 문제), InvMsg2=우선도 낮음(서서히 바뀌는 상태값). Odometer는 24bit(factor1)
// ->16bit(factor5)로 압축(실사용 상한 30만km 기준, docs/hardware/cluster.dbc CM_ 참고).
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

    // 2026-08-07: CAN 링크 상태. 위 필드들과 달리 "수신한 값"이 아니라 "수신이 되고 있는지"다.
    // CAN이 끊기면 위 신호들은 마지막 값 그대로 멈춰 있으므로, UI가 그걸 최신값으로
    // 오해하지 않게 하려면 이 두 필드가 필요하다. main/twai.c의 twai_tx_task가 500ms마다 갱신.
    bool     can_rx_stale;    // InvMsg1/InvMsg2 중 하나라도 주기 대비 타임아웃 (표시 중인 값이 옛날 값)
    bool     can_tx_healthy;  // ClusterAlive가 실제로 버스에 나가고 있는지 (TEC/tx_failed 기준)
} VehicleData_t;

void vehicle_data_init(void);
void vehicle_data_set(const VehicleData_t *src);
void vehicle_data_get(VehicleData_t *dst);

// 링크 상태 두 필드만 원자적으로 갱신한다.
// vehicle_data_get() -> 수정 -> vehicle_data_set() 패턴을 쓰지 않는 이유: 그 패턴은
// get과 set 사이가 원자적이지 않다. 지금까지는 writer가 twai_rx_task 하나뿐이라 문제가
// 없었지만, twai_tx_task가 두 번째 writer로 들어오면서 두 태스크의 read-modify-write가
// 겹치면 서로의 갱신을 덮어쓴다 (예: TX가 읽은 직후 RX가 speed를 갱신 -> TX가 set하면서
// 그 speed가 옛날 값으로 되돌아감). 필드 단위 setter는 뮤텍스 안에서 해당 필드만 건드려
// 이 문제를 원천 차단한다.
void vehicle_data_set_link_status(bool rx_stale, bool tx_healthy);

// UI 렌더 태스크 생존 표시(하트비트). lvgl_ui_task가 한 프레임 그릴 때마다 mark()를
// 부르고, main/twai.c의 twai_tx_task가 last_ms()로 신선도를 확인해 ClusterAlive의
// ClusterStatus bit4에 싣는다.
//
// **일부러 뮤텍스를 쓰지 않는다.** 감지하려는 대상 중 하나가 "UI가 뮤텍스를 잡거나
// 기다리다 멈춘 상태"인데, 감지 수단이 같은 뮤텍스를 잡으면 감지하는 쪽도 같이 멈춰서
// 영영 아무것도 못 알린다. 32bit 정렬 변수의 저장/적재는 Xtensa에서 원자적이므로
// volatile 단일 워드로 충분하다(찢어진 값을 읽을 수 없음).
void     ui_heartbeat_mark(void);
uint32_t ui_heartbeat_last_ms(void);  // 0 = 부팅 후 한 번도 렌더링 안 됨