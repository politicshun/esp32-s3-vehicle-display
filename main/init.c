#include "pin_config.h"
#include "bsp.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "HW_INIT";

/*
 * CH422G는 "레지스터 주소 + 데이터"가 아니라, 기능별로 서로 다른
 * I2C 디바이스 주소를 사용하는 특이한 칩입니다 (ESPHome/커뮤니티 드라이버 기준 확인됨).
 *   0x24 : 모드 레지스터 (EXIO0~7을 push-pull 출력으로 쓸지 설정)
 *   0x38 : 출력 레지스터 (EXIO0~7의 실제 HIGH/LOW 상태, 1바이트 전체를 매번 덮어씀)
 *   0x26 : 입력 레지스터 (이번 프로젝트에선 미사용)
 */
#define CH422G_ADDR_MODE   0x24
#define CH422G_ADDR_OUT    0x38
#define CH422G_MODE_OUTPUT 0x01  // EXIO0~7을 push-pull 출력 모드로 활성화

i2c_master_bus_handle_t g_i2c_bus_handle = NULL;

static i2c_master_dev_handle_t s_ch422g_mode_dev = NULL;
static i2c_master_dev_handle_t s_ch422g_out_dev = NULL;

// 출력 레지스터는 8비트를 통째로 덮어쓰는 방식이라, 마지막으로 쓴 값을
// 캐싱해뒀다가 특정 비트만 켜고/끄고 나머지는 유지해야 합니다.
static uint8_t s_ch422g_out_shadow = 0x00;
static SemaphoreHandle_t s_ch422g_mutex = NULL;

static esp_err_t ch422g_flush_output(void) {
    esp_err_t err = i2c_master_transmit(s_ch422g_out_dev, &s_ch422g_out_shadow, 1, -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CH422G 출력 레지스터 쓰기 실패: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t ch422g_write_exio(uint8_t bit_mask) {
    if (s_ch422g_mutex == NULL) {
        s_ch422g_mutex = xSemaphoreCreateMutex();
    }
    esp_err_t err = ESP_FAIL;
    if (xSemaphoreTake(s_ch422g_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_ch422g_out_shadow |= bit_mask;
        err = ch422g_flush_output();
        ESP_LOGI(TAG, "CH422G EXIO SET: mask=0x%02X -> shadow=0x%02X",
                 bit_mask, s_ch422g_out_shadow);
        xSemaphoreGive(s_ch422g_mutex);
    }
    return err;
}

esp_err_t ch422g_clear_exio(uint8_t bit_mask) {
    if (s_ch422g_mutex == NULL) {
        s_ch422g_mutex = xSemaphoreCreateMutex();
    }
    esp_err_t err = ESP_FAIL;
    if (xSemaphoreTake(s_ch422g_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_ch422g_out_shadow &= ~bit_mask;
        err = ch422g_flush_output();
        ESP_LOGI(TAG, "CH422G EXIO CLR: mask=0x%02X -> shadow=0x%02X",
                 bit_mask, s_ch422g_out_shadow);
        xSemaphoreGive(s_ch422g_mutex);
    }
    return err;
}

static esp_err_t i2c_bus_init(void) {
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    return i2c_new_master_bus(&bus_config, &g_i2c_bus_handle);
}

static esp_err_t ch422g_add_devices(void) {
    i2c_device_config_t mode_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CH422G_ADDR_MODE,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_master_bus_add_device(g_i2c_bus_handle, &mode_cfg, &s_ch422g_mode_dev);
    if (err != ESP_OK) return err;

    i2c_device_config_t out_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CH422G_ADDR_OUT,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    return i2c_master_bus_add_device(g_i2c_bus_handle, &out_cfg, &s_ch422g_out_dev);
}

static esp_err_t ch422g_init(void) {
    esp_err_t err = ch422g_add_devices();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CH422G I2C 디바이스 등록 실패: %s", esp_err_to_name(err));
        return err;
    }

    // 1) 출력 레지스터를 먼저 0(전부 LOW)으로 초기화
    //    -> LCD_RST/CTP_RST가 있다면 이 순간 "리셋 상태(LOW)"로 들어감
    s_ch422g_out_shadow = 0x00;
    err = ch422g_flush_output();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CH422G가 응답하지 않습니다. I2C 배선/전원을 확인하세요.");
        return err;
    }

    // 2) 모드 레지스터: EXIO0~7을 push-pull 출력으로 활성화
    uint8_t mode_val = CH422G_MODE_OUTPUT;
    err = i2c_master_transmit(s_ch422g_mode_dev, &mode_val, 1, -1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CH422G 모드 설정 실패: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "CH422G 초기화 완료 (I2C 응답 확인됨)");
    return ESP_OK;
}

void bsp_hardware_init(void) {
    ESP_LOGI(TAG, "Initializing System Hardware...");

    if (i2c_bus_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C 버스 초기화 실패");
        return;
    }

    if (ch422g_init() != ESP_OK) {
        ESP_LOGE(TAG, "CH422G 초기화 실패 - 이후 LCD/터치/CAN 동작이 비정상일 수 있습니다");
    }

    // LCD_RST/CTP_RST를 LOW(리셋 상태)로 최소 10ms 유지 후 HIGH로 해제
    vTaskDelay(pdMS_TO_TICKS(10));
    ch422g_write_exio(CH422G_EXIO_LCD_RST | CH422G_EXIO_CTP_RST);
    vTaskDelay(pdMS_TO_TICKS(10));

    // LCD 전원 + 백라이트 켜기
    ch422g_write_exio(CH422G_EXIO_LCD_VDD_EN | CH422G_EXIO_LCD_DISP);

    // CAN/USB 공용 스위치를 CAN 모드로 고정
    // (Waveshare 공식 문서 기준: EXIO5 Pull high = CAN 모드, Low = USB 모드)
    ch422g_write_exio(CH422G_EXIO_USB_SEL);

    ESP_LOGI(TAG, "Hardware init sequence complete");
}