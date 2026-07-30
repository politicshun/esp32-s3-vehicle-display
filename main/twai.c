#include "driver/twai.h"
#include "vehicle_data.h"
#include "pin_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TWAI_TASK";

// TODO: 아래 두 CAN ID는 실제 차량/BMS DBC 스펙에 맞춰 반드시 확인 후 수정하세요.
// 지금은 임시 placeholder 입니다 (speed=0x100, soc=0x200과 동일한 패턴으로 가정).
#define CAN_ID_PACK_VOLT 0x300
#define CAN_ID_DTC       0x301

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
        case 0x100:
            current_data.speed = rx_msg->data[0];
            break;
        case 0x200:
            current_data.soc = rx_msg->data[0];
            break;
        case CAN_ID_PACK_VOLT:
            // TODO: 실제 스케일/바이트오더 확인 필요. 우선 0.1V 단위 16비트로 가정.
            current_data.pack_volt =
                ((uint16_t)(rx_msg->data[1] << 8 | rx_msg->data[0])) * 0.1f;
            break;
        case CAN_ID_DTC:
            current_data.dtc_code = (uint16_t)(rx_msg->data[1] << 8 | rx_msg->data[0]);
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
