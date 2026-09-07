#include "driver/twai.h"
#include "vehicle_data.h"
#include "cluster_settings.h"
#include "pin_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "TWAI_TASK";

// 2026-07-31: 차량단(인버터) 설계가 아직 안 끝나서, 이 프로젝트가 CAN 스펙을 먼저 정하고
// 인버터 쪽이 거기 맞추기로 함 — 아래는 "실차값 추측"이 아니라 docs/hardware/cluster.dbc
// (=Desktop cluster.dbc 최종본, GenMsgCycleTime/VAL_ 포함)에 우리가 직접 정의한 스펙이다.
// 단, 인버터 쪽 실구현/실기 검증 전이므로 "우리가 정했다" != "실물로 확인됐다".
// Period(GenMsgCycleTime): InvMsg1=100ms, InvMsg2=200ms — 아직 코드에서 타임아웃/끊김 감지에는
// 안 쓰고 있음 (HARNESS-TODO: 필요해지면 이 값 기준으로 최종 수신시각 추적 로직 추가).
// 2026-07-31 CAN 최적화 리비전: 신호를 서브시스템별이 아니라 갱신 우선도 기준으로
// 재배치함 — InvMsg1=우선도 높음(운전 중 계속 바뀌거나 지연 시 안전 문제),
// InvMsg2=우선도 낮음(서서히 바뀌는 상태값). docs/hardware/cluster.dbc CM_ 참고.
#define CAN_ID_INV_MSG1 0x100  // InvMsg1(100ms): Speed/DriveMode/DTC/Power/RegenPower/TurnSignal/Highbeam/BrakeAbsWarn/MotorTemp/ControllerTemp
#define CAN_ID_INV_MSG2 0x200  // InvMsg2(200ms): SOC/DClinkVoltage/Temp/DriveRange/Odometer/CellDelta/AmbientTemp
// 2026-09-04: 충전 관련 신호(통상 범위 플레이스홀더, docs/design/can-signals.md 참고) —
// InvMsg1/2 남는 바이트로 부족해서 새 메시지로 뺐다. 충전 중에만 의미 있어 가장 느린 주기.
#define CAN_ID_INV_MSG3 0x400  // InvMsg3(1000ms): ChargePower/TimeToFull

// 2026-08-07: 클러스터 -> 인버터 방향 Alive. 이 저장소가 CAN 스펙을 먼저 정하고 인버터가
// 맞추는 프로젝트라(docs/design/can-signals.md 머리말), ID/주기/페이로드는 사용자 결정으로
// 확정한 값이다 — "실차 관례 추측"이 아님. 인버터측 구현/실기 검증은 아직 대기 중.
#define CAN_ID_CLUSTER_ALIVE 0x300
#define CLUSTER_ALIVE_PERIOD_MS 500

// 2026-09-04: Setup 탭 설정값(밝기/회생레벨/자동상향등/자동주야간/단위) — CLUSTER->
// INVERTER, 통상 범위 플레이스홀더(docs/hardware/cluster.dbc, docs/design/can-signals.md
// 참고). ClusterAlive(헬스체크)와 성격이 달라 새 ID로 분리했고, 자주 안 바뀌는 값이라
// twai_tx_task의 500ms 루프 중 2번에 1번(=1000ms)만 보낸다.
#define CAN_ID_CLUSTER_SETTINGS 0x500
#define CLUSTER_SETTINGS_PERIOD_TICKS 2  // CLUSTER_ALIVE_PERIOD_MS(500ms) 기준 2틱=1000ms

// ClusterStatus(byte1) 비트 정의. cluster.dbc CM_ 및 docs/design/can-signals.md와 동기화할 것.
#define ALIVE_ST_INVMSG1_OK (1 << 0)  // InvMsg1을 타임아웃 내에 받고 있음
#define ALIVE_ST_INVMSG2_OK (1 << 1)  // InvMsg2를 타임아웃 내에 받고 있음
#define ALIVE_ST_BLE_CONN   (1 << 2)  // BLE 앱이 연결돼 있음
#define ALIVE_ST_DTC_ACTIVE (1 << 3)  // dtc_code != 0
#define ALIVE_ST_UI_OK      (1 << 4)  // UI 렌더 태스크가 살아서 프레임을 그리고 있음

// UI 하트비트 임계값. 렌더 루프 실측이 평균 60ms, 부팅 직후 최대 249ms이므로
// 정상 동작 중에 오탐이 나지 않도록 1초로 잡았다. 화면이 멈추면 1~1.5초 안에 감지된다.
// (실측 근거: docs/design/2026-08-07-scheduling-overview.html의 frame timing 표)
//
// 이 비트가 필요한 이유: UI만 멈추면 CAN은 멀쩡하므로 Alive가 계속 정상으로 나간다.
// 인버터 입장에서 클러스터는 완전히 건강해 보이는데 운전자는 얼어붙은 화면을 본다.
// can_rx_stale로 화면을 흐리게 하는 처리도 UI가 살아있어야 그려지므로 이 상황엔 무용.
#define UI_HEARTBEAT_TIMEOUT_MS 1000

// 수신 끊김 판정 임계값 = GenMsgCycleTime의 약 3배(cluster.dbc: InvMsg1=100ms, InvMsg2=200ms).
// 여기서 처음으로 DBC의 주기값을 코드가 실제로 사용한다 (이전까지는 HARNESS-TODO였음).
#define INV_MSG1_TIMEOUT_MS 300
#define INV_MSG2_TIMEOUT_MS 600

// 드라이버 lifecycle(install/uninstall) vs twai_transmit() 경합 보호.
// RX 태스크가 버스오프 복구로 uninstall 하는 도중 TX 태스크가 transmit 하면
// 이미 해제된 드라이버를 건드리게 되므로 반드시 같은 락 아래에서 처리한다.
// (twai_receive()는 RX 태스크 전용 경로라 락 밖에 둔다 — 1초씩 락을 잡으면 TX가 굶는다.)
static SemaphoreHandle_t s_twai_lock = NULL;
static volatile bool s_twai_running = false;

// 마지막 수신 시각(ms). 0 = 부팅 후 한 번도 못 받음.
static volatile uint32_t s_last_rx_msg1_ms = 0;
static volatile uint32_t s_last_rx_msg2_ms = 0;

static inline uint32_t now_ms(void) {
    return (uint32_t)pdTICKS_TO_MS(xTaskGetTickCount());
}

void twai_init(void) {
    if (s_twai_lock == NULL) {
        s_twai_lock = xSemaphoreCreateMutex();
    }
}

static bool twai_start_with_retry(void) {
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO_NUM, CAN_RX_GPIO_NUM, TWAI_MODE_NORMAL);
    // TODO: 실제 버스 속도 확인 필요 (500kbps 가정 중 - 250k/1M인 차량도 흔함)
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

    // 2026-08-07: ACCEPT_ALL -> 하드웨어 어셉턴스 필터. 원래 dual filter mode로
    // 0x100/0x200 딱 두 개만 정확 매칭했는데, 2026-09-04에 InvMsg3(0x400)가 세 번째
    // 수신 ID로 추가되면서 그 방식이 더 이상 안 통한다 — dual/single filter의 mask는
    // "이 비트는 신경 안 씀(1)"만 표현 가능하고 "이 세 비트 중 정확히 하나만 켜짐" 같은
    // 조건은 표현 못 한다(0x100/0x200/0x400은 11bit ID 중 8/9/10번 비트가 각각 하나씩만
    // 켜진 값). 그래서 single filter mode로 바꿔서 ID 8~10번 비트를 통째로 don't-care
    // 처리해 세 값을 전부 통과시키고, 그 대신 0x000/0x300(ClusterAlive와 겹침, 근데
    // self-reception 안 함)/0x500/0x600/0x700도 같이 통과된다 — 이건 handle_rx_message()의
    // switch default에서 소프트웨어로 버린다(원래 주석이 예고했던 절충).
    //
    // ESP32-S3 TWAI(SJA1000 계열) single filter mode, 표준 프레임 32bit 배치:
    //   [31:21]=ID(11bit) [20]=RTR [19:12]=data[0] [11:4]=data[1] [3:0]=미사용
    // mask는 1 = don't care.
    //   - ID bit8~10(0x100/0x200/0x400에 해당, word bit29~31) = don't care
    //   - ID bit0~7(하위 바이트, word bit21~28) = 정확 일치(0) — 세 타깃 ID 전부 하위
    //     바이트가 0x00이라 여기서 이미 대부분의 오검출(예: 시뮬레이터 decoy ID 0x101~0x180)을
    //     걸러낸다.
    //   - RTR(word bit20) = 정확 일치(0), 데이터 프레임만 통과
    //   - data[0]/data[1](word bit4~19), 미사용(word bit0~3) = don't care
    //   code = 0 (신경 쓰는 비트는 전부 0과 일치해야 함)
    //   mask = (0x7 << 29) | (0xFFFF << 4) | 0xF = 0xE00FFFFF
    // 수신 대상 ID가 또 바뀌면 위 계산을 다시 하고 이 주석도 같이 갱신할 것.
    twai_filter_config_t f_config = {
        .acceptance_code = 0x00000000U,
        .acceptance_mask = 0xE00FFFFFU,
        .single_filter = true,
    };

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_driver_install 실패: %s (배선/GPIO 설정 확인 필요)",
                 esp_err_to_name(err));
        return false;
    }

    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_start 실패: %s", esp_err_to_name(err));
        twai_driver_uninstall();
        return false;
    }

    ESP_LOGI(TAG, "TWAI Driver Started Successfully (TX=GPIO%d, RX=GPIO%d)",
             CAN_TX_GPIO_NUM, CAN_RX_GPIO_NUM);
    return true;
}

static void handle_rx_message(const twai_message_t *rx_msg) {
    // cluster.dbc의 InvMsg1/InvMsg2는 표준(11bit) 데이터 프레임이다. 확장 ID 0x100이나
    // RTR 프레임을 표준 0x100으로 오인해서 언패킹하면 안 된다.
    // (하드웨어 필터가 이미 대부분 막지만, 필터 설정이 바뀌었을 때를 대비한 이중 방어)
    if (rx_msg->extd || rx_msg->rtr) {
        return;
    }

    // HARNESS-TODO: KVASER 벤치 테스트용 임시 raw 로그. 실차 연결 전 삭제할 것.
    ESP_LOGI(TAG, "RX id=0x%03lX dlc=%d data=%02X %02X %02X %02X %02X %02X %02X %02X",
             rx_msg->identifier, rx_msg->data_length_code,
             rx_msg->data[0], rx_msg->data[1], rx_msg->data[2], rx_msg->data[3],
             rx_msg->data[4], rx_msg->data[5], rx_msg->data[6], rx_msg->data[7]);

    VehicleData_t current_data;
    vehicle_data_get(&current_data);

    switch (rx_msg->identifier) {
        case CAN_ID_INV_MSG1:
            // cluster.dbc InvMsg1 (BU_ INVERTER->CLUSTER, DLC8, 100ms, 우선도 높음) 바이트 배치:
            // byte0=Speed byte1=DriveMode byte2=DTC byte3=Power byte4=RegenPower
            // byte5=TurnSignal(bit0-1)/Highbeam(bit2)/BrakeAbsWarn(bit3) byte6=MotorTemp byte7=ControllerTemp
            // 전부 raw unsigned(@1+), offset은 산술로만 적용(2의 보수 아님)
            current_data.speed = (int16_t)rx_msg->data[0] - 10;       // factor1, offset-10, [-10|245]
            current_data.drive_mode = rx_msg->data[1];                // VAL_: 0=P 1=R 2=N 3=D
            current_data.dtc_code = rx_msg->data[2];                  // [0|255], 단일 열거값
            current_data.power_kw = (float)rx_msg->data[3];           // factor1, [0|255]kW
            current_data.regen_kw = (float)rx_msg->data[4];           // factor1, [0|255]kW
            // 2026-09-04 추가(통상 범위 플레이스홀더, docs/design/can-signals.md 참고)
            current_data.turn_signal = rx_msg->data[5] & 0x03;        // bit0-1: 0=off 1=left 2=right 3=hazard
            current_data.highbeam = (rx_msg->data[5] & 0x04) != 0;    // bit2
            current_data.brake_abs_warn = (rx_msg->data[5] & 0x08) != 0; // bit3
            current_data.motor_temp_c = (int16_t)rx_msg->data[6] - 20;      // factor1, offset-20, [-20|235]
            current_data.controller_temp_c = (int16_t)rx_msg->data[7] - 20; // factor1, offset-20, [-20|235]
            s_last_rx_msg1_ms = now_ms();
            break;
        case CAN_ID_INV_MSG2: {
            // cluster.dbc InvMsg2 (BU_ INVERTER->CLUSTER, DLC8, 200ms, 우선도 낮음) 바이트 배치:
            // byte0=SOC byte1=DClinkVoltage byte2=Temp byte3=DriveRange byte4~5=Odometer(16bit LE)
            // byte6=CellDelta byte7=AmbientTemp
            current_data.soc = rx_msg->data[0];                       // [0|100]
            current_data.pack_volt = (float)rx_msg->data[1];          // DClinkVoltage, factor1, [0|80]
            current_data.sys_temp_c = (int16_t)rx_msg->data[2] - 20;  // factor1, offset-20, [-20|235]
            current_data.range_km = rx_msg->data[3];                  // [0|255]km
            // Odometer: 2026-07-31 CAN 최적화로 24bit(factor1)->16bit(factor5) 압축,
            // raw 최대 65535 * 5 = 327,675km까지 커버 (실사용 상한 30만km 기준)
            uint16_t odo_raw = (uint16_t)(rx_msg->data[4] | (rx_msg->data[5] << 8));
            current_data.odo_km = (uint32_t)odo_raw * 5;              // factor5, [0|327675]km
            // 2026-09-04 추가(통상 범위 플레이스홀더, docs/design/can-signals.md 참고)
            current_data.cell_delta_mv = rx_msg->data[6];             // factor1, [0|255]mV
            current_data.ambient_temp_c = (int16_t)rx_msg->data[7] - 40; // factor1, offset-40, [-40|215]
            s_last_rx_msg2_ms = now_ms();
            break;
        }
        case CAN_ID_INV_MSG3:
            // cluster.dbc InvMsg3 (BU_ INVERTER->CLUSTER, DLC8, 1000ms, 충전 전용) 바이트
            // 배치: byte0=ChargePower byte1=TimeToFull byte2~7=예약. 2026-09-04(2차):
            // 사용자가 DBC에서 TimeToFull을 16bit(byte1~2, [0|1440]min)에서
            // 8bit(byte1 하나, [0|255]min)로 확정 — 여기도 맞춰 byte2를 더 이상 안 읽는다.
            current_data.charge_power_kw = (float)rx_msg->data[0];    // factor1, [0|255]kW
            current_data.time_to_full_min = rx_msg->data[1];          // factor1, [0|255]min
            break;
        default:
            // 정의 안 된 ID는 조용히 무시 (디버깅 시엔 아래 주석 해제)
            // ESP_LOGD(TAG, "미처리 CAN ID: 0x%03lX", rx_msg->identifier);
            break;
    }

    vehicle_data_set(&current_data);
}

void twai_rx_task(void *pvParameters) {
    // 드라이버 설치/시작이 실패하면 무한 재시도하되, busy-loop을 막기 위해
    // 매 시도 사이에 delay를 둡니다. 계속 실패하면 5초마다 재시도.
    while (!twai_start_with_retry()) {
        ESP_LOGW(TAG, "3초 후 TWAI 재시작 시도...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
    s_twai_running = true;  // 이 시점부터 twai_tx_task가 송신해도 안전

    twai_message_t rx_msg;

    while (1) {
        esp_err_t err = twai_receive(&rx_msg, pdMS_TO_TICKS(1000));
        if (err == ESP_OK) {
            handle_rx_message(&rx_msg);
        } else if (err == ESP_ERR_TIMEOUT) {
            // 1초 동안 수신 없음 - 정상일 수도 있으니 그냥 다음 루프로.
            // (portMAX_DELAY 대신 타임아웃을 둬서 버스 오프 등 상태 점검이 가능하게 함)
            twai_status_info_t status;
            if (twai_get_status_info(&status) == ESP_OK &&
                status.state == TWAI_STATE_BUS_OFF) {
                ESP_LOGE(TAG, "CAN 버스 오프 상태 감지 - 재시작 시도");
                // uninstall~재설치 구간 전체를 락으로 감싼다. TX 태스크가 이 사이에
                // twai_transmit()을 호출하면 해제된 드라이버를 건드리게 된다.
                xSemaphoreTake(s_twai_lock, portMAX_DELAY);
                s_twai_running = false;
                twai_stop();
                twai_driver_uninstall();
                while (!twai_start_with_retry()) {
                    vTaskDelay(pdMS_TO_TICKS(3000));
                }
                s_twai_running = true;
                xSemaphoreGive(s_twai_lock);
            }
        } else {
            ESP_LOGW(TAG, "twai_receive 에러: %s", esp_err_to_name(err));
        }
    }
}

// ClusterAlive(0x300, 500ms, CLUSTER -> INVERTER) 송신 태스크.
//
// twai_rx_task 안에서 같이 보내지 않고 태스크를 분리한 이유: RX 루프는 twai_receive()에
// 최대 1초 블로킹되므로, 거기에 송신을 얹으면 Alive 주기가 수신 트래픽에 끌려다닌다.
// TWAI 드라이버 자체는 스레드 세이프이고, 드라이버 lifecycle만 s_twai_lock으로 막는다.
//
// 주의(벤치 테스트): TWAI_MODE_NORMAL 송신은 다른 노드의 ACK를 필요로 한다. 버스에
// 이 보드만 있으면 ACK 에러 -> 자동 재전송 반복 -> TEC 증가로 이어진다.
// 보드 단독으로 송신을 확인하려면 twai_start_with_retry()의 op_mode를 TWAI_MODE_NO_ACK로
// 바꿔야 한다 (단, 그 모드에서는 수신이 정상 동작하지 않으므로 임시 확인용으로만).
//
// 2026-08-07 실측(COM12, Kvaser 분리 상태): TEC는 128(error passive)에서 **멈추고
// bus-off(256)까지 가지 않는다**. CAN 규격상 error passive 상태의 송신자가 ACK 에러를
// 검출했고 passive error flag를 보내는 동안 dominant 비트를 못 봤다면 TEC를 올리지 않기
// 때문이다. 따라서 "응답 노드 없음"의 최종 상태는 bus-off가 아니라 error passive이고,
// twai_rx_task의 버스오프 감지로는 이 상황을 못 잡는다 — tx_link_healthy()가 TEC를
// 직접 봐야 하는 이유다. (관측값: state=RUNNING TEC=128 tx_failed=0, bus_err만 계속 증가)
// 직전 주기의 송신이 "버스에 실제로 나갔는지"를 컨트롤러 상태로 판정한다.
//
// twai_transmit()의 반환값으로는 이걸 알 수 없다 — ESP-IDF 드라이버 소스를 확인한 결과
// twai_transmit()은 프레임을 TX 버퍼/큐에 넣은 시점에 ESP_OK를 반환하고, 버스에서 ACK를
// 받았는지는 기다리지 않는다. 즉 버스에 아무도 없어도(=아무 데도 도달 못 해도) ESP_OK다.
// 실차에서 인버터가 사라졌을 때 클러스터가 그걸 눈치채려면 TEC/tx_failed_count를 봐야 한다.
static bool tx_link_healthy(uint32_t *last_tx_failed, uint32_t *msgs_pending) {
    twai_status_info_t st;
    if (twai_get_status_info(&st) != ESP_OK) {
        *msgs_pending = 0;
        return false;
    }

    *msgs_pending = st.msgs_to_tx;
    bool failed_grew = (st.tx_failed_count != *last_tx_failed);
    *last_tx_failed = st.tx_failed_count;

    // TEC >= 128 = error passive. ACK를 못 받으면 재전송마다 TEC가 올라가므로 버스에
    // 우리만 남으면 금방 여기 걸린다 — 2026-08-07 실측으로 부팅 약 1초 만에 128 도달.
    // 128을 임계값으로 쓰는 이유: ACK 실패는 error passive에서 TEC 증가가 멈춰 bus-off로
    // 넘어가지 않으므로(위 twai_tx_task 주석 참고), state만 봐서는 영영 RUNNING이다.
    // 복구 시엔 성공 송신마다 TEC가 1씩 내려가며 128 밑으로 떨어지면 정상 판정된다
    // (실측 복구 소요 약 1~2초, TEC 122에서 전환 확인).
    return (st.state == TWAI_STATE_RUNNING) && !failed_grew && (st.tx_error_counter < 128);
}

void twai_tx_task(void *pvParameters) {
    TickType_t last_wake = xTaskGetTickCount();
    uint8_t alive_counter = 0;
    uint32_t last_tx_failed = 0;
    bool tx_healthy = true;
    bool tx_healthy_prev = true;
    uint32_t settings_tick = 0;

    bool ui_ok_prev = true;

    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CLUSTER_ALIVE_PERIOD_MS));

        if (!s_twai_running) {
            // 드라이버 설치 전이거나 버스오프 복구 중 — 송신이 되고 있을 리 없다.
            vehicle_data_set_link_status(true, false);
            continue;
        }

        VehicleData_t data;
        vehicle_data_get(&data);

        uint32_t t = now_ms();
        uint8_t status = 0;
        if (s_last_rx_msg1_ms != 0 && (t - s_last_rx_msg1_ms) < INV_MSG1_TIMEOUT_MS) {
            status |= ALIVE_ST_INVMSG1_OK;
        }
        if (s_last_rx_msg2_ms != 0 && (t - s_last_rx_msg2_ms) < INV_MSG2_TIMEOUT_MS) {
            status |= ALIVE_ST_INVMSG2_OK;
        }
        if (data.ble_connected) {
            status |= ALIVE_ST_BLE_CONN;
        }
        if (data.dtc_code != 0) {
            status |= ALIVE_ST_DTC_ACTIVE;
        }
        // 하트비트가 0이면 부팅 후 아직 한 프레임도 못 그린 상태 — 이것도 UI 비정상으로 본다.
        uint32_t ui_hb = ui_heartbeat_last_ms();
        bool ui_ok = (ui_hb != 0) && ((t - ui_hb) < UI_HEARTBEAT_TIMEOUT_MS);
        if (ui_ok) {
            status |= ALIVE_ST_UI_OK;
        }

        // 부팅 후 경과 초. 65535초(약 18.2시간)에서 saturate — 롤오버시키면 수신측이
        // 재부팅과 구분할 수 없으므로 일부러 포화시킨다.
        uint32_t uptime_s = t / 1000;
        uint16_t uptime_field = (uptime_s > 0xFFFF) ? 0xFFFF : (uint16_t)uptime_s;

        twai_message_t tx_msg = {
            .identifier = CAN_ID_CLUSTER_ALIVE,
            .data_length_code = 8,
            .data = {
                alive_counter,                    // byte0: AliveCounter (0~255 순환)
                status,                           // byte1: ClusterStatus (비트마스크)
                (uint8_t)(uptime_field & 0xFF),   // byte2~3: ClusterUptime (16bit LE, 초)
                (uint8_t)(uptime_field >> 8),
                0, 0, 0, 0,                       // byte4~7: 예약(향후 확장용)
            },
        };
        // .extd/.rtr/.ss/.self 는 지정 초기화로 전부 0 = 표준 데이터 프레임

        if (xSemaphoreTake(s_twai_lock, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (s_twai_running) {
                // 직전 주기 송신의 결말을 먼저 확인한다(500ms면 완료/실패가 확정돼 있다).
                uint32_t msgs_pending = 0;
                tx_healthy = tx_link_healthy(&last_tx_failed, &msgs_pending);

                if (msgs_pending > 0) {
                    // 직전 Alive가 아직 버스에 못 나갔다(ACK 없음 등). 여기서 하나 더 쌓으면
                    // 나중에 버스가 살아났을 때 몇 초 전에 만든 payload가 줄줄이 방출된다 —
                    // 2026-08-07 실기기 실측: 리스너를 늦게 붙이자 uptime 0~3초짜리 프레임
                    // 6개가 한꺼번에 나온 뒤에야 현재값(89초)이 나왔다.
                    // Alive는 "지금 살아있다"를 알리는 메시지라 늦게 배달된 옛 프레임은
                    // 무의미한 정도가 아니라 해롭다(수신측이 방금 부팅했다고 오판한다).
                    // 그래서 쌓지 않고 이번 주기를 건너뛴다 — 다음 주기에 최신값으로 다시 만든다.
                    // (재전송 자체를 끄는 single-shot은 쓰지 않는다. 버스가 붐빌 때 정상적으로
                    //  발생하는 중재 실패까지 버려서 Alive 도달률이 떨어지기 때문이다.)
                    ESP_LOGD(TAG, "ClusterAlive 이번 주기 건너뜀 (미전송 %lu건 대기 중)",
                             (unsigned long)msgs_pending);
                } else {
                    esp_err_t err = twai_transmit(&tx_msg, pdMS_TO_TICKS(50));
                    if (err == ESP_OK) {
                        alive_counter++;
                    } else {
                        // 여기 걸리는 건 큐 적재 자체가 실패한 경우(버스오프 등)뿐이다.
                        // ACK 실패는 여기서 안 잡힌다 — tx_link_healthy()가 담당.
                        ESP_LOGW(TAG, "ClusterAlive 큐 적재 실패: %s", esp_err_to_name(err));
                        tx_healthy = false;
                    }
                }

                // ClusterSettings(0x500): Alive와 달리 "늦게 도착한 옛 값이 해롭다"는
                // 성질이 없어서(그냥 마지막으로 조작한 설정값일 뿐) msgs_pending 스킵
                // 로직을 그대로 갖다 쓰지 않는다 — 500ms 루프 중 2번에 1번만 시도.
                settings_tick++;
                if (settings_tick >= CLUSTER_SETTINGS_PERIOD_TICKS) {
                    settings_tick = 0;
                    ClusterSettings_t cs;
                    cluster_settings_get(&cs);
                    uint8_t flags = (cs.auto_headlight ? 0x01 : 0) |
                                     (cs.auto_day_night ? 0x02 : 0) |
                                     (cs.units_mph ? 0x04 : 0);
                    twai_message_t settings_msg = {
                        .identifier = CAN_ID_CLUSTER_SETTINGS,
                        .data_length_code = 8,
                        .data = {
                            cs.brightness_pct,  // byte0: Brightness [0|100]%
                            cs.regen_level,      // byte1: RegenLevel [0|3]
                            flags,                // byte2: bit0=AutoHeadlight bit1=AutoDayNight bit2=Units
                            0, 0, 0, 0, 0,        // byte3~7: 예약(향후 확장용)
                        },
                    };
                    esp_err_t serr = twai_transmit(&settings_msg, pdMS_TO_TICKS(50));
                    if (serr != ESP_OK) {
                        ESP_LOGD(TAG, "ClusterSettings 큐 적재 실패: %s", esp_err_to_name(serr));
                    }
                }
            }
            xSemaphoreGive(s_twai_lock);
        }

        // UI 하트비트도 상태가 바뀔 때만 로그. UI가 멈춘 상황에서는 화면에 아무것도
        // 띄울 수 없으므로 이 로그와 ClusterAlive bit4가 유일한 통보 수단이다.
        if (ui_ok != ui_ok_prev) {
            ESP_LOGW(TAG, "UI 렌더 태스크 %s (마지막 프레임 %lums 전)",
                     ui_ok ? "정상 복구" : "응답 없음 — 화면이 멈춰 있을 수 있음",
                     (unsigned long)(ui_hb == 0 ? 0 : t - ui_hb));
            ui_ok_prev = ui_ok;
        }

        // 상태가 바뀔 때만 로그 — 500ms마다 찍으면 다른 로그를 덮는다.
        if (tx_healthy != tx_healthy_prev) {
            twai_status_info_t st;
            if (twai_get_status_info(&st) == ESP_OK) {
                ESP_LOGW(TAG, "CAN TX %s (state=%d TEC=%lu tx_failed=%lu arb_lost=%lu bus_err=%lu)",
                         tx_healthy ? "정상 복구" : "이상 — 버스에 응답 노드가 없을 수 있음",
                         (int)st.state, (unsigned long)st.tx_error_counter,
                         (unsigned long)st.tx_failed_count, (unsigned long)st.arb_lost_count,
                         (unsigned long)st.bus_error_count);
            }
            tx_healthy_prev = tx_healthy;
        }

        // UI가 "멈춘 값"을 최신값으로 오해하지 않도록 링크 상태를 공유 데이터에 반영.
        // 수신 신호 중 하나라도 끊기면 stale로 본다 (InvMsg1이 살아있어도 InvMsg2가
        // 죽었으면 SOC/전압/온도는 이미 옛날 값이다).
        bool rx_stale = !(status & ALIVE_ST_INVMSG1_OK) || !(status & ALIVE_ST_INVMSG2_OK);
        vehicle_data_set_link_status(rx_stale, tx_healthy);
    }
}
