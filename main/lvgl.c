#include "lvgl.h"
#include "vehicle_data.h"
#include "pin_config.h"
#include "bsp.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui/ui.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch_gt911.h"

static const char *TAG = "LVGL_TASK";

#define LCD_H_RES 800
#define LCD_V_RES 480

// LVGL 드로우 버퍼: 화면 전체가 아니라 1/10 높이 정도만 잡아도 충분히 빠릅니다.
// PSRAM에 할당 (내부 SRAM은 다른 용도로 아껴둠).
#define LVGL_BUF_HEIGHT (LCD_V_RES / 10)

static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_lcd_touch_handle_t s_touch_handle = NULL;
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;
static lv_indev_drv_t s_indev_drv;

/* ---- LVGL -> 실제 패널로 픽셀 밀어넣기 ---- */
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_draw_bitmap(s_panel_handle, area->x1, area->y1,
                               area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

/* ---- GT911 터치 좌표를 LVGL 인풋 이벤트로 변환 ---- */
static void lvgl_touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t touch_x[1] = {0};
    uint16_t touch_y[1] = {0};
    uint16_t touch_strength[1] = {0};
    uint8_t touch_cnt = 0;

    esp_lcd_touch_read_data(s_touch_handle);
    bool touched = esp_lcd_touch_get_coordinates(s_touch_handle, touch_x, touch_y,
                                                  touch_strength, &touch_cnt, 1);

    if (touched && touch_cnt > 0) {
        data->point.x = touch_x[0];
        data->point.y = touch_y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* ---- RGB LCD 패널 초기화 (esp_lcd_rgb_panel) ---- */
static esp_err_t lcd_panel_init(void) {
    // 핀맵 출처: Waveshare 공식 문서 "LCD Interface" 표 (RGB565, 5-6-5비트)
    // data_gpio_nums 순서는 RGB565 LSB->MSB: B3..B7, G2..G7, R3..R7
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = 16 * 1000 * 1000,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_pulse_width = 4,
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .vsync_pulse_width = 4,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .flags.pclk_active_neg = true,
        },
        .data_width = 16,
        .num_fbs = 1,
        .hsync_gpio_num = LCD_HSYNC_GPIO,
        .vsync_gpio_num = LCD_VSYNC_GPIO,
        .de_gpio_num = LCD_DE_GPIO,
        .pclk_gpio_num = LCD_PCLK_GPIO,
        .disp_gpio_num = -1, // 별도 DISP 핀 없음 (백라이트는 CH422G EXIO2로 제어함)
        .data_gpio_nums = {
            LCD_B3_GPIO, LCD_B4_GPIO, LCD_B5_GPIO, LCD_B6_GPIO, LCD_B7_GPIO,
            LCD_G2_GPIO, LCD_G3_GPIO, LCD_G4_GPIO, LCD_G5_GPIO, LCD_G6_GPIO, LCD_G7_GPIO,
            LCD_R3_GPIO, LCD_R4_GPIO, LCD_R5_GPIO, LCD_R6_GPIO, LCD_R7_GPIO,
        },
        .flags.fb_in_psram = true,
    };

    esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &s_panel_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_rgb_panel 실패: %s", esp_err_to_name(err));
        return err;
    }

    // 주의: LCD_RST는 CH422G EXIO3을 통해 bsp_hardware_init()에서 이미 해제됨
    //       (esp_lcd_panel_reset()을 여기서 또 호출하면 안 됨 - GPIO 리셋 핀이 없음)
    err = esp_lcd_panel_init(s_panel_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_panel_init 실패: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "RGB LCD 패널 초기화 완료 (%dx%d)", LCD_H_RES, LCD_V_RES);
    return ESP_OK;
}

/* ---- GT911 터치 컨트롤러 초기화 (bsp_hardware_init()에서 만든 I2C_NUM_0 버스 재사용) ---- */
static esp_err_t touch_init(void) {
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_config.scl_speed_hz = 400000;

    esp_err_t err = esp_lcd_new_panel_io_i2c(g_i2c_bus_handle, &io_config, &io_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "터치 IO 생성 실패: %s", esp_err_to_name(err));
        return err;
    }

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        // 리셋/인터럽트 핀은 GPIO가 아니라 CH422G EXIO1/GPIO4를 씀.
        // 리셋은 bsp_hardware_init()에서 이미 처리했으므로 여기선 -1(미사용).
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    err = esp_lcd_touch_new_i2c_gt911(io_handle, &tp_cfg, &s_touch_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GT911 초기화 실패: %s (I2C 배선/CH422G 리셋 순서 확인)",
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "GT911 터치 초기화 완료");
    return ESP_OK;
}

static void lvgl_ui_build(void) {
    ui_init(lv_scr_act());
}

void lvgl_ui_task(void *pvParameters) {
    ESP_LOGI(TAG, "LVGL UI Task Started on Core 1");

    if (lcd_panel_init() != ESP_OK) {
        ESP_LOGE(TAG, "LCD 초기화 실패 - UI 태스크를 종료합니다");
        vTaskDelete(NULL);
        return;
    }

    bool touch_ok = (touch_init() == ESP_OK);
    if (!touch_ok) {
        ESP_LOGW(TAG, "터치 없이 화면만 표시합니다 (터치 재확인 필요)");
    }

    lv_init();

    // LVGL 틱 소스: 원래 코드엔 없었던 부분. 애니메이션/타이머가 이걸로 시간을 잽니다.
    // (별도 esp_timer 콜백 대신, 이 태스크의 20ms 주기 자체를 tick으로 사용)

    static lv_color_t *buf1 = NULL;
    buf1 = heap_caps_malloc(LCD_H_RES * LVGL_BUF_HEIGHT * sizeof(lv_color_t),
                             MALLOC_CAP_SPIRAM);
    if (buf1 == NULL) {
        ESP_LOGE(TAG, "LVGL 드로우 버퍼 할당 실패 (PSRAM 부족)");
        vTaskDelete(NULL);
        return;
    }
    lv_disp_draw_buf_init(&s_draw_buf, buf1, NULL, LCD_H_RES * LVGL_BUF_HEIGHT);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = LCD_H_RES;
    s_disp_drv.ver_res = LCD_V_RES;
    s_disp_drv.flush_cb = lvgl_flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&s_disp_drv);

    if (touch_ok) {
        lv_indev_drv_init(&s_indev_drv);
        s_indev_drv.type = LV_INDEV_TYPE_POINTER;
        s_indev_drv.read_cb = lvgl_touch_read_cb;
        lv_indev_drv_register(&s_indev_drv);
    }

    lvgl_ui_build();

    TickType_t last_tick = xTaskGetTickCount();

    while (1) {
        ui_update();

        // 실제 경과 시간을 LVGL에 알려줘야 애니메이션/입력 디바운스가 정상 동작함
        TickType_t now = xTaskGetTickCount();
        lv_tick_inc(pdTICKS_TO_MS(now - last_tick));
        last_tick = now;

        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}