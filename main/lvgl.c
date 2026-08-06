#include "lvgl.h"
#include "vehicle_data.h"
#include "pin_config.h"
#include "bsp.h"
#include "esp_log.h"
#include "esp_timer.h"
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

static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_lcd_touch_handle_t s_touch_handle = NULL;
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;
static lv_indev_drv_t s_indev_drv;

/* 2026-08-06(3차): 페이지 전환(탭) 시 화면이 밴드 단위로 나뉘어 바뀌는 테어링 리포트 —
 * direct_mode 재도전은 과거 두 번(2026-08-04) 실패 이력이 있어 보류
 * (docs/design/toolchain-versions.md, 로직 분석기 실측 필요라 이 세션에서 재시도 불가).
 * 대신 하이브리드: 평소엔 아래 SRAM 버퍼로 빠르게 그리다가, 페이지가 통째로 바뀌는
 * "그 한 프레임"만 패널 자신의 PSRAM fbs[]에 직접 그려 cur_fb_index를 원자적으로 스왑하는
 * 예전 zero-copy 경로로 잠깐 전환해 테어링을 없앤다 — lvgl_request_tearfree_page_switch() */
static void *s_psram_fb0 = NULL;
static void *s_psram_fb1 = NULL;
static lv_color_t *s_sram_draw_buf = NULL;
static bool s_using_psram_draw_buf = false;

/* DIAGNOSTIC(2026-08-05, 임시): handler_us(lv_timer_handler 전체) 중 실제 픽셀 밀어넣기
 * (esp_lcd_panel_draw_bitmap, "zero-copy 스왑"이라던 그 호출)가 차지하는 비중만 따로 재서,
 * LVGL 소프트웨어 래스터라이즈(그리기 자체) vs 패널 드라이버 플러시 중 뭐가 200ms를
 * 먹는지 분리한다. lvgl_ui_task()의 1초 창 출력 로직과 공유하는 파일 스코프 누적치 —
 * 확인 끝나면 이 블록과 flush_cb 안의 타이밍 코드는 원복할 것. */
static int64_t s_flush_us_sum = 0;
static int64_t s_flush_us_max = 0;
static uint32_t s_flush_call_count = 0;

/* ---- LVGL -> 실제 패널로 픽셀 밀어넣기 ---- */
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    int64_t t0 = esp_timer_get_time();
    esp_lcd_panel_draw_bitmap(s_panel_handle, area->x1, area->y1,
                               area->x2 + 1, area->y2 + 1, color_map);
    int64_t dt = esp_timer_get_time() - t0;
    s_flush_us_sum += dt;
    s_flush_call_count++;
    if (dt > s_flush_us_max) s_flush_us_max = dt;
    lv_disp_flush_ready(drv);
}

/* ui.c의 go_to_page()가 lv_obj_set_tile_id() 호출 직전에 부른다. full_refresh=1이라 그
 * 다음 lv_timer_handler() 호출이 곧바로 새 페이지 전체를 이 PSRAM 버퍼로 한 번에 그리고
 * (flush 1콜, cur_fb_index 스왑이라 테어링 없음), lvgl_ui_task()의 루프가 그 직후 자동으로
 * SRAM 버퍼로 되돌린다(아래 while(1) 참고) — flush_cb가 동기 호출이라 그 시점엔 이미 화면
 * 반영이 끝나 있어 프레임 카운터 없이 "한 번 쓰고 바로 복귀"가 안전하다. */
void lvgl_request_tearfree_page_switch(void) {
    if (s_psram_fb0 && s_psram_fb1 && !s_using_psram_draw_buf) {
        lv_disp_draw_buf_init(&s_draw_buf, s_psram_fb0, s_psram_fb1, LCD_H_RES * LCD_V_RES);
        s_using_psram_draw_buf = true;
    }
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
        .num_fbs = 2,
        .dma_burst_size = 64,
        .bounce_buffer_size_px = 20 * LCD_H_RES,
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

    // 페이지 전환 테어프리 스왑용 — 패널이 이미 갖고 있는 num_fbs=2 PSRAM 프레임버퍼
    // 포인터만 얻어둔다(평소엔 안 씀, lvgl_request_tearfree_page_switch() 호출 시에만 사용).
    if (esp_lcd_rgb_panel_get_frame_buffer(s_panel_handle, 2, &s_psram_fb0, &s_psram_fb1) != ESP_OK) {
        ESP_LOGW(TAG, "PSRAM 프레임버퍼 획득 실패 — 페이지 전환 테어프리 스왑 비활성화");
    }

    bool touch_ok = (touch_init() == ESP_OK);
    if (!touch_ok) {
        ESP_LOGW(TAG, "터치 없이 화면만 표시합니다 (터치 재확인 필요)");
    }

    lv_init();

    // LVGL 틱 소스: 원래 코드엔 없었던 부분. 애니메이션/타이머가 이걸로 시간을 잽니다.
    // (별도 esp_timer 콜백 대신, 이 태스크의 20ms 주기 자체를 tick으로 사용)

    // 2026-08-04: 처음엔 RGB panel 드라이버가 이미 갖고 있는 PSRAM 풀스크린 프레임버퍼
    // 2장을 LVGL 드로우 버퍼로 그대로 써서(포인터가 패널 자기 fbs[i] 범위 안이면
    // esp_lcd_panel_rgb.c의 rgb_panel_draw_bitmap()이 CPU memcpy 없이 cur_fb_index만
    // 바꾸는 "zero-copy" 경로를 탄다는 걸 노렸었다. direct_mode(부분 재드로우)는 3/4페이지
    // 반투명 카드/점 영역에서 원인 미상 지직거림이 나서(2026-08-04) full_refresh로 롤백,
    // 그건 지금도 유지한다(아래 아무것도 안 건드림 — direct_mode 재도전과는 무관한 변경).
    //
    // 2026-08-05: 그런데 "zero-copy로 빠르다"던 그 설계가 사실은 정반대였다 — 실측
    // (lv_timer_handler 205ms/프레임, 예산 20ms의 10배)으로 확인됨. 아낀 건 플러시
    // 한 번(측정: 50us, 무시할 수준)뿐이고, 대신 LVGL이 픽셀 하나하나 그리는 모든
    // 렌더링 연산(이미지 블릿/아크 안티에일리어싱/폰트 래스터라이즈)이 SRAM보다 3~4배
    // 느린 PSRAM을 직접 상대하게 만든 게 진짜 비용이었다(Espressif 공식 esp_lvgl_port
    // 성능 문서: "PSRAM에 프레임버퍼를 두면 더 나쁜 결과", 권장 드로우 버퍼 크기는
    // 화면의 10~25%, 내부 SRAM 위치 — https://github.com/espressif/esp-bsp/blob/master/
    // components/esp_lvgl_port/docs/performance.md).
    //
    // 그래서 반대로 되돌린다: LVGL은 작은 내부 SRAM 버퍼에 빠르게 그리고, flush_cb의
    // esp_lcd_panel_draw_bitmap()이 그 결과만 PSRAM 프레임버퍼로 복사하는 "보통" 경로를
    // 탄다(zero-copy 최적화 포기) — SRAM 렌더링으로 버는 시간이 복사 한 번 더 생기는
    // 비용보다 훨씬 크다는 게 위 실측+공식 가이드로 확인됐다. flush_cb 자체는 코드
    // 변경 없음(color_map이 패널 자신의 fb든 별도 SRAM 버퍼든 esp_lcd_panel_draw_bitmap이
    // 알아서 처리).
#define LVGL_DRAW_BUF_LINES 48  /* 800*48px = 화면 높이의 10% */
    size_t draw_buf_bytes = LCD_H_RES * LVGL_DRAW_BUF_LINES * sizeof(lv_color_t);
    /* MALLOC_CAP_DMA는 안 씀 — 이 버퍼는 CPU(LVGL 소프트웨어 렌더러)가 채워넣는 용도지
     * 하드웨어 DMA가 직접 읽는 게 아니다(esp_lcd_panel_draw_bitmap()의 일반 복사 경로가
     * 이 버퍼→PSRAM 프레임버퍼 복사를 CPU로 처리). DMA 가능 내부 SRAM은 BLE 스택 등이
     * 부팅 초반에 먼저 가져가서 풀이 훨씬 좁아 96000바이트(60라인) 할당이 실패했었다
     * (2026-08-06 실기기 재현: "LVGL 드로우 버퍼 할당 실패" 로그, 진단 로그로 확인한
     * largest_free_block=83968 bytes — DMA 플래그 유무와 무관하게 파편화로 60라인은
     * 항상 실패). 40라인(64000 bytes)으로 우선 낮춰서 해결했었다.
     *
     * 2026-08-06(2차): 40라인짜리 버퍼는 이 SRAM 버퍼가 패널 자신의 PSRAM fbs[] 밖에
     * 있어서 esp_lcd_panel_draw_bitmap()이 "일반 복사" 경로(esp_lcd_panel_rgb.c
     * rgb_panel_draw_bitmap의 draw_buf_copy_to_fb=true 분기)를 타는데, 이 경로는
     * 지금 실제로 스캔아웃 중인 라이브 프레임버퍼(fbs[cur_fb_index])에 밴드 단위로
     * 곧바로 덮어쓴다 — 완성된 프레임을 통째로 그린 뒤 원자적으로 스왑하는 예전
     * zero-copy 경로(draw_buffer가 fbs[i] 안에 있을 때만 타는 cur_fb_index 스왑 분기)를
     * 안 타기 때문. full_refresh=1이라 매 프레임 전체 화면을 40라인×12밴드로 나눠 순차
     * 기록하는데, 이게 스캔 도중 실시간으로 겹쳐써서 페이지 전환처럼 화면 전체가 한번에
     * 바뀔 때 "나뉘어서 바뀌는" 테어링으로 보인다는 사용자 리포트(실기기 확인). 근본
     * 해결(스왑 기반 무테어링)은 아니지만, largest_free_block 안에서 최대한 버퍼를
     * 키우면 밴드 수가 줄어(12->10) 한 번의 전체 화면 갱신이 더 빨리 끝나 테어링이
     * 노출되는 시간이 짧아진다 — 48라인(76800 bytes, largest_free_block=83968 대비
     * 여유 7168 bytes)으로 시험, 양산 가능한 수준인지 실기기로 확인 중. */
    ESP_LOGI(TAG, "내부 SRAM 상태: free=%u, largest_block=%u (요청 %u bytes)",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
             (unsigned)draw_buf_bytes);
    lv_color_t *draw_buf1 = heap_caps_malloc(draw_buf_bytes, MALLOC_CAP_INTERNAL);
    if (draw_buf1 == NULL) {
        ESP_LOGE(TAG, "LVGL 드로우 버퍼 할당 실패 (내부 SRAM %u bytes)", (unsigned)draw_buf_bytes);
        vTaskDelete(NULL);
        return;
    }
    s_sram_draw_buf = draw_buf1;
    lv_disp_draw_buf_init(&s_draw_buf, draw_buf1, NULL, LCD_H_RES * LVGL_DRAW_BUF_LINES);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = LCD_H_RES;
    s_disp_drv.ver_res = LCD_V_RES;
    s_disp_drv.flush_cb = lvgl_flush_cb;
    s_disp_drv.draw_buf = &s_draw_buf;
    s_disp_drv.full_refresh = 1;
    lv_disp_drv_register(&s_disp_drv);

    if (touch_ok) {
        lv_indev_drv_init(&s_indev_drv);
        s_indev_drv.type = LV_INDEV_TYPE_POINTER;
        s_indev_drv.read_cb = lvgl_touch_read_cb;
        lv_indev_drv_register(&s_indev_drv);
    }

    lvgl_ui_build();

    TickType_t last_tick = xTaskGetTickCount();

    /* 2026-08-05: "게이지/막대 애니메이션이 끊겨 보인다"는 피드백 — full_refresh=1이라
     * 뭐든 바뀌면 매번 화면 전체(800x480)를 소프트웨어로 다시 그리는데(GPU 없음,
     * CONFIG_LV_USE_GPU_* 전부 꺼짐), CAN 시뮬레이터를 "항상 모든 값이 동시에 움직이게"
     * 바꾼 뒤로는 매 20ms 틱마다 예외 없이 풀스크린 리드로우가 강제된다. 이게 실제로
     * 20ms 예산을 넘기고 있는지 추측 대신 직접 재서 확인한다 — 되돌리기 전에 원인부터
     * 확정(과거 direct_mode 시도를 실측 없이 되돌렸던 전례를 반복하지 않기 위함).
     * loop_us = 루프 한 바퀴 전체(ui_update+lv_timer_handler+vTaskDelay 포함) 실제 간격,
     * handler_us = lv_timer_handler() 자체 실행 시간만. 1초 창으로 모아서 평균/최대/
     * 20ms 초과 횟수를 찍는다 — 매 프레임 로그 찍으면 그 로그 출력 자체가 타이밍을
     * 왜곡하므로 반드시 집계 후 요약만 출력. */
    int64_t window_start_us = esp_timer_get_time();
    int64_t last_loop_us = window_start_us;
    uint32_t frame_count = 0;
    uint32_t handler_over_20ms_count = 0;
    uint32_t loop_over_20ms_count = 0;
    int64_t handler_us_sum = 0, handler_us_max = 0;
    int64_t loop_us_sum = 0, loop_us_max = 0;

    while (1) {
        ui_update();

        // 실제 경과 시간을 LVGL에 알려줘야 애니메이션/입력 디바운스가 정상 동작함
        TickType_t now = xTaskGetTickCount();
        lv_tick_inc(pdTICKS_TO_MS(now - last_tick));
        last_tick = now;

        int64_t handler_start_us = esp_timer_get_time();
        lv_timer_handler();
        int64_t handler_end_us = esp_timer_get_time();

        // 페이지 전환 테어프리 스왑(lvgl_request_tearfree_page_switch()) 직후의 그 한
        // 프레임만 위 lv_timer_handler()가 PSRAM 버퍼로 그렸다 — flush_cb가 동기 호출이라
        // 여기 도달한 시점엔 이미 화면 반영이 끝나 있으므로, 다음 프레임부터는 곧바로
        // 빠른 SRAM 버퍼로 되돌린다.
        if (s_using_psram_draw_buf) {
            lv_disp_draw_buf_init(&s_draw_buf, s_sram_draw_buf, NULL, LCD_H_RES * LVGL_DRAW_BUF_LINES);
            s_using_psram_draw_buf = false;
        }

        int64_t handler_us = handler_end_us - handler_start_us;
        int64_t loop_us = handler_end_us - last_loop_us;
        last_loop_us = handler_end_us;

        frame_count++;
        handler_us_sum += handler_us;
        loop_us_sum += loop_us;
        if (handler_us > handler_us_max) handler_us_max = handler_us;
        if (loop_us > loop_us_max) loop_us_max = loop_us;
        if (handler_us > 20000) handler_over_20ms_count++;
        if (loop_us > 20000) loop_over_20ms_count++;

        if (handler_end_us - window_start_us >= 1000000) {
            ESP_LOGI(TAG, "frame timing: n=%lu handler_avg=%lldus handler_max=%lldus "
                     "handler_over20ms=%lu loop_avg=%lldus loop_max=%lldus loop_over20ms=%lu "
                     "| flush: calls=%lu avg=%lldus max=%lldus sum=%lldus",
                     (unsigned long)frame_count,
                     handler_us_sum / (frame_count ? frame_count : 1), handler_us_max,
                     (unsigned long)handler_over_20ms_count,
                     loop_us_sum / (frame_count ? frame_count : 1), loop_us_max,
                     (unsigned long)loop_over_20ms_count,
                     (unsigned long)s_flush_call_count,
                     s_flush_us_sum / (s_flush_call_count ? s_flush_call_count : 1), s_flush_us_max,
                     s_flush_us_sum);
            window_start_us = handler_end_us;
            frame_count = 0;
            handler_over_20ms_count = 0;
            loop_over_20ms_count = 0;
            handler_us_sum = handler_us_max = 0;
            loop_us_sum = loop_us_max = 0;
            s_flush_us_sum = 0;
            s_flush_us_max = 0;
            s_flush_call_count = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}