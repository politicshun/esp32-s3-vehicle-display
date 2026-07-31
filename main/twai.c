#include "driver/twai.h"
#include "vehicle_data.h"
#include "pin_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TWAI_TASK";

// 2026-07-31: 차량단(인버터) 설계가 아직 안 끝나서, 이 프로젝트가 CAN 스펙을 먼저 정하고
// 인버터 쪽이 거기 맞추기로 함 — 아래는 "실차값 추측"이 아니라 docs/hardware/vehicle.dbc
// (=Desktop cluster.dbc 최종본, GenMsgCycleTime/VAL_ 포함)에 우리가 직접 정의한 스펙이다.
// 단, 인버터 쪽 실구현/실기 검증 전이므로 "우리가 정했다" != "실물로 확인됐다".
// Period(GenMsgCycleTime): InvMsg1=100ms, InvMsg2=200ms — 아직 코드에서 타임아웃/끊김 감지에는
// 안 쓰고 있음 (HARNESS-TODO: 필요해지면 이 값 기준으로 최종 수신시각 추적 로직 추가).
#define CAN_ID_INV_MSG1 0x100  // InvMsg1: Speed/DriveMode/DTC/DClinkVoltage/SOC/Temp
#define CAN_ID_INV_MSG2 0x200  // InvMsg2: DriveRange/RegenPower/Odometer/Power

static bool twai_start_with_retry(void) {
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO_NUM, CAN_RX_GPIO_NUM, TWAI_MODE_NORMAL);
    // TODO: 실제 버스 속도 확인 필요 (500kbps 가정 중 - 250k/1M인 차량도 흔함)
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

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
    // HARNESS-TODO: KVASER 벤치 테스트용 임시 raw 로그. 실차 연결 전 삭제할 것.
    ESP_LOGI(TAG, "RX id=0x%03lX dlc=%d data=%02X %02X %02X %02X %02X %02X %02X %02X",
             rx_msg->identifier, rx_msg->data_length_code,
             rx_msg->data[0], rx_msg->data[1], rx_msg->data[2], rx_msg->data[3],
             rx_msg->data[4], rx_msg->data[5], rx_msg->data[6], rx_msg->data[7]);

    VehicleData_t current_data;
    vehicle_data_get(&current_data);

    switch (rx_msg->identifier) {
        case CAN_ID_INV_MSG1:
            // cluster.dbc InvMsg1 (BU_ INVERTER->CLUSTER, DLC8, 100ms) 바이트 배치:
            // byte0=Speed byte1=DriveMode byte2=DTC byte3~4=(예약) byte5=DClinkVoltage
            // byte6=SOC byte7=Temp — 전부 raw unsigned(@1+), offset은 산술로만 적용(2의 보수 아님)
            current_data.speed = (int16_t)rx_msg->data[0] - 10;       // factor1, offset-10, [-10|245]
            current_data.drive_mode = rx_msg->data[1];                // VAL_: 0=P 1=R 2=N 3=D
            current_data.dtc_code = rx_msg->data[2];                  // [0|255], 단일 열거값
            // byte3~4: 예약(미사용)
            current_data.pack_volt = (float)rx_msg->data[5];          // DClinkVoltage, factor1, [0|80]
            current_data.soc = rx_msg->data[6];                       // [0|100]
            current_data.sys_temp_c = (int16_t)rx_msg->data[7] - 20;  // factor1, offset-20, [-20|235]
            break;
        case CAN_ID_INV_MSG2:
            // cluster.dbc InvMsg2 (BU_ INVERTER->CLUSTER, DLC8, 200ms) 바이트 배치:
            // byte0=DriveRange byte1=RegenPower byte2~4=Odometer(24bit LE) byte5=Power byte6~7=(예약)
            current_data.range_km = rx_msg->data[0];                  // [0|255]km
            current_data.regen_kw = (float)rx_msg->data[1];           // factor1, [0|255]kW
            current_data.odo_km = (uint32_t)rx_msg->data[2] |
                                  ((uint32_t)rx_msg->data[3] << 8) |
                                  ((uint32_t)rx_msg->data[4] << 16);   // [0|16777215]km
            current_data.power_kw = (float)rx_msg->data[5];           // factor1, [0|255]kW
            // byte6~7: 예약(미사용)
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
