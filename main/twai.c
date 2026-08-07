#include "driver/twai.h"
#include "vehicle_data.h"
#include "pin_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
#define CAN_ID_INV_MSG1 0x100  // InvMsg1(100ms): Speed/DriveMode/DTC/Power/RegenPower
#define CAN_ID_INV_MSG2 0x200  // InvMsg2(200ms): SOC/DClinkVoltage/Temp/DriveRange/Odometer

static bool twai_start_with_retry(void) {
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO_NUM, CAN_RX_GPIO_NUM, TWAI_MODE_NORMAL);
    // TODO: 실제 버스 속도 확인 필요 (500kbps 가정 중 - 250k/1M인 차량도 흔함)
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();

    // 2026-08-07: ACCEPT_ALL -> 하드웨어 어셉턴스 필터(dual filter mode).
    // 이전에는 버스의 모든 프레임이 인터럽트/RX큐/태스크까지 올라온 뒤 switch의 default에서
    // 소프트웨어로 버려졌다. 벤치(Kvaser 2개 ID)에서는 티가 안 나지만 실차 버스에 붙이면
    // 관심 없는 프레임 때문에 RX큐(기본 5개)와 CPU를 그대로 낭비한다.
    //
    // ESP32-S3의 TWAI는 SJA1000 계열 어셉턴스 필터다. dual filter mode + 표준 프레임에서
    // 32bit acceptance code/mask의 비트 배치는:
    //   [31:21]=필터1 ID(11bit)  [20]=필터1 RTR  [19:16]=필터1 data[0] 상위 니블
    //   [15:5] =필터2 ID(11bit)  [4] =필터2 RTR  [3:0] =필터2 data[0] 하위 니블
    // mask는 1 = don't care.
    //
    //   code = (0x100 << 21) | (0x200 << 5) = 0x20000000 | 0x00004000 = 0x20004000
    //   mask = (0xF << 16) | 0xF            = 0x000F000F   (data 니블만 무시, ID/RTR은 정확 일치)
    //
    // 결과: 표준 데이터 프레임 0x100과 0x200 **정확히 두 개만** 통과. RTR도 하드웨어에서 차단.
    // 수신 대상 ID를 추가/변경하면 위 계산을 다시 하고 이 주석도 같이 갱신할 것
    // (ID가 3개 이상 필요해지면 dual filter로는 정확 매칭이 불가능하므로,
    //  마스크를 넓혀 통과시키고 switch의 default에서 거르는 절충이 필요하다).
    twai_filter_config_t f_config = {
        .acceptance_code = ((uint32_t)CAN_ID_INV_MSG1 << 21) | ((uint32_t)CAN_ID_INV_MSG2 << 5),
        .acceptance_mask = (0xFU << 16) | 0xFU,
        .single_filter = false,
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
            // byte0=Speed byte1=DriveMode byte2=DTC byte3=Power byte4=RegenPower byte5~7=(예약)
            // 전부 raw unsigned(@1+), offset은 산술로만 적용(2의 보수 아님)
            current_data.speed = (int16_t)rx_msg->data[0] - 10;       // factor1, offset-10, [-10|245]
            current_data.drive_mode = rx_msg->data[1];                // VAL_: 0=P 1=R 2=N 3=D
            current_data.dtc_code = rx_msg->data[2];                  // [0|255], 단일 열거값
            current_data.power_kw = (float)rx_msg->data[3];           // factor1, [0|255]kW
            current_data.regen_kw = (float)rx_msg->data[4];           // factor1, [0|255]kW
            // byte5~7: 예약(미사용)
            break;
        case CAN_ID_INV_MSG2: {
            // cluster.dbc InvMsg2 (BU_ INVERTER->CLUSTER, DLC8, 200ms, 우선도 낮음) 바이트 배치:
            // byte0=SOC byte1=DClinkVoltage byte2=Temp byte3=DriveRange byte4~5=Odometer(16bit LE) byte6~7=(예약)
            current_data.soc = rx_msg->data[0];                       // [0|100]
            current_data.pack_volt = (float)rx_msg->data[1];          // DClinkVoltage, factor1, [0|80]
            current_data.sys_temp_c = (int16_t)rx_msg->data[2] - 20;  // factor1, offset-20, [-20|235]
            current_data.range_km = rx_msg->data[3];                  // [0|255]km
            // Odometer: 2026-07-31 CAN 최적화로 24bit(factor1)->16bit(factor5) 압축,
            // raw 최대 65535 * 5 = 327,675km까지 커버 (실사용 상한 30만km 기준)
            uint16_t odo_raw = (uint16_t)(rx_msg->data[4] | (rx_msg->data[5] << 8));
            current_data.odo_km = (uint32_t)odo_raw * 5;              // factor5, [0|327675]km
            // byte6~7: 예약(미사용)
            break;
        }
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
                twai_stop();
                twai_driver_uninstall();
                while (!twai_start_with_retry()) {
                    vTaskDelay(pdMS_TO_TICKS(3000));
                }
            }
        } else {
            ESP_LOGW(TAG, "twai_receive 에러: %s", esp_err_to_name(err));
        }
    }
}
