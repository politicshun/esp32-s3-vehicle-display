#include "ui.h"
#include "ui_style.h"
#include "vehicle_data.h"
#include <stdio.h>
#include <stdbool.h>

/* 게이지 뒤에 까는 정적 글로우 배경 (scripts/gen_glow_image.py로 생성, main/ui/ui_glow_*.c).
 * 매 프레임 재계산되는 lv shadow와 달리 한 번만 그려진 비트맵을 그대로 blit하므로 가볍다. */
extern const lv_img_dsc_t ui_glow_speed;
extern const lv_img_dsc_t ui_glow_soc;

/*
 * main/include/vehicle_data.h 의 VehicleData_t 필드:
 *   speed(uint16_t), soc(uint8_t), pack_volt(float), dtc_code(uint16_t), ble_connected(bool),
 *   drive_mode(uint8_t), odo_km(uint32_t), range_km(uint16_t), power_kw(float), regen_kw(float),
 *   sys_temp_c(int8_t)
 *
 * drive_mode/odo_km/range_km/power_kw/regen_kw/sys_temp_c는 docs/hardware/vehicle.dbc
 * 기준 CAN ID 0x302~0x307 placeholder다 (실차 DBC 미대조, main/twai.c의 HARNESS-TODO 참고).
 * 값 자체는 화면에 바인딩돼 있지만 스케일/의미를 신뢰하고 쓰면 안 된다.
 *
 * TRIP/시계/외기온도는 여전히 VehicleData_t에 소스가 없어 "HARNESS-TODO" 정적
 * placeholder로 남아있다. 필드가 추가되면 아래 ui_update()의 해당 블록만 채우면 된다.
 */

/* 속도 게이지 표시 범위 — 실차 최고속도 스펙이 아니라 UI 표시 스케일 값.
 * 클러스터 UI에서 흔히 쓰는 라운드 값(200km/h)으로 설정, 실차 스펙 확정되면 조정 가능. */
#define SPEED_GAUGE_MAX_KMH 200
/* 레드존 시작값도 실차 스펙이 아니라 계기판 장식용 UI 스케일 값 (최대치의 80% 지점). */
#define SPEED_GAUGE_REDLINE_START 160

#define UI_CARD_W 160
#define UI_CARD_H 78

static lv_obj_t *tileview;
static lv_obj_t *pages[4];
static lv_obj_t *page_dots[4];

/* Page 1 — 주행 필수 정보 */
#define UI_SPEED_GRAD_SEGMENTS 8
static lv_obj_t *meter_speed;
/* 값 아크를 진한 블루->시안으로 이어지는 세그먼트 여러 개로 근사해서 그라데이션처럼 보이게 함
 * (LVGL 8.x lv_arc/lv_meter 아크는 단색만 지원, 진짜 그라데이션 아크가 없음) */
static lv_meter_indicator_t *speed_grad_segs[UI_SPEED_GRAD_SEGMENTS];
static int32_t speed_seg_bounds[UI_SPEED_GRAD_SEGMENTS + 1];
static lv_obj_t *lbl_speed;
static lv_obj_t *lbl_gear;        /* drive_mode 바인딩 (placeholder, CAN 0x302) */
static lv_obj_t *lbl_odo;         /* odo_km 바인딩 (placeholder, CAN 0x303) */
static lv_obj_t *lbl_trip_todo;   /* HARNESS-TODO: trip 필드 없음 (범위 밖) */

/* Page 2 — 전력 및 배터리 */
static lv_obj_t *meter_soc;
static lv_meter_indicator_t *soc_value_indic;
static lv_obj_t *lbl_soc_pct;
static lv_obj_t *lbl_pack_volt;
static lv_obj_t *lbl_range;        /* range_km 바인딩 (placeholder, CAN 0x304) */
static lv_obj_t *lbl_power_regen;  /* power_kw/regen_kw 바인딩 (placeholder, CAN 0x305/0x306) */

/* Page 3 — 시스템 상태 및 진단 */
static lv_obj_t *banner_dtc;
static lv_obj_t *lbl_dtc_code;
static lv_obj_t *lbl_pack_volt2;  /* page3 전용 전압 라벨 (page2의 lbl_pack_volt와 별개 위젯) */
static lv_obj_t *card_sys_temp;   /* sys_temp_c 경고 시 테두리 색 전환용 */
static lv_obj_t *lbl_sys_temp;    /* sys_temp_c 바인딩 (placeholder, CAN 0x307) */

/* Page 4 — 기기 조작 및 연결성 */
static lv_obj_t *dot_ble;
static lv_obj_t *lbl_ble_status;
static lv_obj_t *dot_vehicle;

/* ---------------------------------------------------------------------
 * 공통 헬퍼
 * ------------------------------------------------------------------- */
static lv_obj_t *make_page(lv_obj_t *tv, uint8_t col_id)
{
    lv_obj_t *page = lv_tileview_add_tile(tv, col_id, 0, LV_DIR_HOR);
    lv_obj_add_style(page, &ui_style_bg, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
}

/* 빈 공간을 채우는 보조 정보 카드 — 제목(작게) 위, 값(크게) 아래, 테두리 있는 박스.
 * 데이터가 없는 항목은 value_lbl에 "--"만 채우고 ui_update()에서 갱신하지 않는다. */
static lv_obj_t *make_info_card(lv_obj_t *parent, const char *title, lv_obj_t **value_lbl_out)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, UI_CARD_W, UI_CARD_H);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_bg_color(card, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_10, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_border_opa(card, LV_OPA_40, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_obj_add_style(lbl_title, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_title, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_title, title);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *lbl_value = lv_label_create(card);
    lv_obj_add_style(lbl_value, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_value, "--");
    lv_obj_align(lbl_value, LV_ALIGN_BOTTOM_MID, 0, -10);

    if (value_lbl_out) *value_lbl_out = lbl_value;
    return card;
}

/* 64x64 저해상도 글로우 원본(ui_glow_speed/ui_glow_soc)을 target_size로 확대해서 배치.
 * LVGL zoom(256=100%)으로 키우므로 원본 해상도가 작아도 플래시를 거의 안 먹는다. */
static lv_obj_t *make_glow_bg(lv_obj_t *parent, const lv_img_dsc_t *src, int target_size)
{
    lv_obj_t *img = lv_img_create(parent);
    lv_img_set_src(img, src);
    lv_img_set_pivot(img, src->header.w / 2, src->header.h / 2);
    lv_img_set_zoom(img, (target_size * 256) / src->header.w);
    return img;
}

/* 자동차 계기판 느낌의 원형 게이지: 눈금+숫자 스케일 + 위험구간 정적 아크 + (옵션) 값 아크(동적).
 * tick_cnt/major_nth로 눈금 간격을 정하고(예: tick_cnt=21,major_nth=2 -> 10단위 눈금,
 * 20단위 숫자), warn_end > warn_start일 때만 경고색 정적 아크를 깔아준다.
 * value_arc_enabled=false면 단색 값 아크를 만들지 않고 scale_out으로 스케일 포인터만 내보낸다
 * (호출자가 직접 그라데이션 세그먼트 등 커스텀 인디케이터를 붙일 수 있게). */
static lv_obj_t *make_gauge_meter(lv_obj_t *parent, int size, int32_t min, int32_t max,
                                   int tick_cnt, int major_nth,
                                   int32_t warn_start, int32_t warn_end,
                                   bool value_arc_enabled,
                                   lv_meter_indicator_t **value_indic_out,
                                   lv_meter_scale_t **scale_out)
{
    lv_obj_t *meter = lv_meter_create(parent);
    lv_obj_set_size(meter, size, size);
    lv_obj_set_style_bg_color(meter, UI_COLOR_BG, LV_PART_MAIN);
    lv_obj_set_style_border_width(meter, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(meter, UI_COLOR_TEXT_SEC, LV_PART_MAIN);
    lv_obj_set_style_border_opa(meter, LV_OPA_30, LV_PART_MAIN);
    /* NOTE: 시안 글로우(shadow_width)를 시도했으나 LVGL 소프트웨어 그림자 렌더링이 매우 무거워서
     * 애니메이션되는 원형 게이지 2개에 매 프레임 재계산되자 CPU1이 거기 묶여 task watchdog
     * 타임아웃(화면 멈춤 + 발열)까지 발생 — 실기기에서 재현 확인 후 제거함. 정적 위젯에
     * 작은 shadow_width로 다시 시도하는 건 몰라도, 애니메이션 위젯에는 쓰지 말 것. */
    lv_obj_set_style_text_color(meter, UI_COLOR_TEXT_SEC, LV_PART_TICKS);
    lv_obj_clear_flag(meter, LV_OBJ_FLAG_CLICKABLE);

    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_range(meter, scale, min, max, 270, 135);
    lv_meter_set_scale_ticks(meter, scale, tick_cnt, 2, 8, UI_COLOR_TEXT_SEC);
    lv_meter_set_scale_major_ticks(meter, scale, major_nth, 3, 14, UI_COLOR_TEXT_PRI, 10);

    /* 경고구역(고정)은 눈금 바로 안쪽에 얇게, 값 아크(동적)는 그보다 더 안쪽에 두껍게 —
     * 반지름을 분리해서 두 아크가 겹치지 않고 계기판처럼 층이 지게 한다. */
    if (warn_end > warn_start) {
        lv_meter_indicator_t *warn = lv_meter_add_arc(meter, scale, 6, UI_COLOR_RED, -8);
        lv_meter_set_indicator_start_value(meter, warn, warn_start);
        lv_meter_set_indicator_end_value(meter, warn, warn_end);
    }

    if (value_arc_enabled) {
        /* lv_meter_set_indicator_value()는 start=end=v로 설정해서 길이 0인 점이 되어버리므로
         * (헤더 문서에 명시됨) 쓰지 않는다 — start는 min에 고정하고 end만 갱신해야
         * "0부터 현재값까지 채워지는" lv_arc 스타일의 게이지 느낌이 난다. */
        lv_meter_indicator_t *value_indic = lv_meter_add_arc(meter, scale, 14, UI_COLOR_CYAN, -34);
        lv_meter_set_indicator_start_value(meter, value_indic, min);
        lv_meter_set_indicator_end_value(meter, value_indic, min);
        if (value_indic_out) *value_indic_out = value_indic;
    }

    if (scale_out) *scale_out = scale;
    return meter;
}

/* ---------------------------------------------------------------------
 * Page 1 — 주행 필수 정보
 * ------------------------------------------------------------------- */
static void build_page_drive(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv, 0);
    pages[0] = page;

    /* 게이지 뒤에 까는 정적 글로우 배경 (매 프레임 재계산 없음 — 한 번 그려진 비트맵 blit) */
    lv_obj_t *glow_speed = make_glow_bg(page, &ui_glow_speed, 280);
    lv_obj_align(glow_speed, LV_ALIGN_CENTER, 0, -20);

    /* 속도 게이지: 0~200km/h, 10단위 눈금/20단위 숫자, 160~200 레드존(장식용 UI 스케일).
     * 값 아크는 여기서 직접 그라데이션 세그먼트로 붙이므로 value_arc_enabled=false. */
    lv_meter_scale_t *speed_scale = NULL;
    meter_speed = make_gauge_meter(page, 280, 0, SPEED_GAUGE_MAX_KMH, 21, 2,
                                    SPEED_GAUGE_REDLINE_START, SPEED_GAUGE_MAX_KMH,
                                    false, NULL, &speed_scale);
    lv_obj_align(meter_speed, LV_ALIGN_CENTER, 0, -20);
    /* 배경을 투명하게 해서 뒤에 깐 글로우 이미지가 다이얼 면으로 그대로 비치게 함 */
    lv_obj_set_style_bg_opa(meter_speed, LV_OPA_TRANSP, LV_PART_MAIN);

    /* 진한 블루(저값) -> 시안(고값)으로 이어지는 8구간 그라데이션 값 아크 */
    for (int i = 0; i < UI_SPEED_GRAD_SEGMENTS; i++) {
        speed_seg_bounds[i] = (SPEED_GAUGE_MAX_KMH * i) / UI_SPEED_GRAD_SEGMENTS;
        lv_color_t seg_color = lv_color_mix(UI_COLOR_CYAN, UI_COLOR_CYAN_DEEP,
                                             (i * 255) / (UI_SPEED_GRAD_SEGMENTS - 1));
        lv_meter_indicator_t *seg = lv_meter_add_arc(meter_speed, speed_scale, 14, seg_color, -34);
        lv_meter_set_indicator_start_value(meter_speed, seg, speed_seg_bounds[i]);
        lv_meter_set_indicator_end_value(meter_speed, seg, speed_seg_bounds[i]);
        speed_grad_segs[i] = seg;
    }
    speed_seg_bounds[UI_SPEED_GRAD_SEGMENTS] = SPEED_GAUGE_MAX_KMH;

    lbl_speed = lv_label_create(page);
    lv_obj_add_style(lbl_speed, &ui_style_label_big, 0);
    lv_label_set_text(lbl_speed, "0");
    lv_obj_align_to(lbl_speed, meter_speed, LV_ALIGN_CENTER, 0, -12);

    lv_obj_t *unit = lv_label_create(page);
    lv_obj_add_style(unit, &ui_style_label_small, 0);
    lv_label_set_text(unit, "km/h");
    lv_obj_align_to(unit, lbl_speed, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    /* drive_mode(placeholder, CAN 0x302) -> P/R/N/D 배지. 값 자체는 실차 미대조 가정이라
     * 화면엔 뜨지만 신뢰할 수 있는 값은 아님(docs/hardware/vehicle.dbc 참고). */
    lv_obj_t *gear_badge = lv_obj_create(page);
    lv_obj_remove_style_all(gear_badge);
    lv_obj_set_size(gear_badge, 56, 56);
    lv_obj_set_style_radius(gear_badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(gear_badge, 2, 0);
    lv_obj_set_style_border_color(gear_badge, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(gear_badge, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(gear_badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(gear_badge, LV_ALIGN_TOP_LEFT, 16, 16);

    lbl_gear = lv_label_create(gear_badge);
    lv_obj_add_style(lbl_gear, &ui_style_label_mid, 0);
    lv_obj_set_style_text_color(lbl_gear, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_gear, "--");
    lv_obj_center(lbl_gear);

    /* "핵심 표시 항목" 표의 "주행 모드(Drive Mode)" — 배지 아래 캡션으로 항목명만 표시 */
    lv_obj_t *lbl_gear_caption = lv_label_create(page);
    lv_obj_add_style(lbl_gear_caption, &ui_style_label_small, 0);
    lv_label_set_text(lbl_gear_caption, "MODE");
    lv_obj_align_to(lbl_gear_caption, gear_badge, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    lv_obj_t *odo_card = make_info_card(page, "ODO", &lbl_odo);
    lv_obj_align(odo_card, LV_ALIGN_BOTTOM_LEFT, 16, -16);

    lv_obj_t *trip_card = make_info_card(page, "TRIP", &lbl_trip_todo);
    lv_obj_align(trip_card, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
}

/* ---------------------------------------------------------------------
 * Page 2 — 전력 및 배터리
 * ------------------------------------------------------------------- */
static void build_page_battery(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv, 1);
    pages[1] = page;

    /* 게이지 뒤에 까는 정적 글로우 배경 (speed 페이지와 동일한 방식) */
    lv_obj_t *glow_soc = make_glow_bg(page, &ui_glow_soc, 240);
    lv_obj_align(glow_soc, LV_ALIGN_TOP_MID, 0, 8);

    /* SOC 게이지: 0~100%, 10단위 눈금/20단위 숫자, 0~20 레드존(저잔량 경고, UI_SOC_LOW_PCT와 동일 기준) */
    meter_soc = make_gauge_meter(page, 240, 0, 100, 11, 2, 0, UI_SOC_LOW_PCT,
                                  true, &soc_value_indic, NULL);
    lv_obj_align(meter_soc, LV_ALIGN_TOP_MID, 0, 8);
    lv_obj_set_style_bg_opa(meter_soc, LV_OPA_TRANSP, LV_PART_MAIN);

    lbl_soc_pct = lv_label_create(page);
    lv_obj_add_style(lbl_soc_pct, &ui_style_label_big, 0);
    lv_label_set_text(lbl_soc_pct, "0%");
    lv_obj_align_to(lbl_soc_pct, meter_soc, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *volt_card = make_info_card(page, "Pack Voltage", &lbl_pack_volt);
    lv_label_set_text(lbl_pack_volt, "-- V");
    lv_obj_align_to(volt_card, meter_soc, LV_ALIGN_OUT_BOTTOM_MID, 0, 14);

    /* range_km(placeholder, CAN 0x304) */
    lv_obj_t *range_card = make_info_card(page, "Range", &lbl_range);
    lv_obj_align(range_card, LV_ALIGN_BOTTOM_LEFT, 16, -16);

    /* power_kw/regen_kw(placeholder, CAN 0x305/0x306) — 카드 하나에 두 줄로 표시 */
    lv_obj_t *power_card = make_info_card(page, "Power/Regen", &lbl_power_regen);
    lv_obj_align(power_card, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
}

/* ---------------------------------------------------------------------
 * Page 3 — 시스템 상태 및 진단
 * ------------------------------------------------------------------- */
static void build_page_diag(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv, 2);
    pages[2] = page;

    banner_dtc = lv_obj_create(page);
    lv_obj_remove_style_all(banner_dtc);
    lv_obj_set_size(banner_dtc, 800 - 32, 64);
    lv_obj_set_style_radius(banner_dtc, 8, 0);
    lv_obj_set_style_bg_color(banner_dtc, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(banner_dtc, LV_OPA_20, 0);
    lv_obj_align(banner_dtc, LV_ALIGN_TOP_MID, 0, 16);

    lbl_dtc_code = lv_label_create(banner_dtc);
    lv_obj_add_style(lbl_dtc_code, &ui_style_label_mid, 0);
    /* dtc_code CAN ID(0x301)는 docs/design/can-signals.md 기준 실차 DBC 미대조
     * placeholder다 — UI는 raw 값만 그대로 보여준다(의미 해석 시도 안 함). */
    lv_label_set_text(lbl_dtc_code, "DTC: OK");
    lv_obj_center(lbl_dtc_code);

    /* 남는 공간을 채우는 정보 카드 2개(전압/온도) — 가운데 정렬된 큰 카드로 가시성 강화 */
    lv_obj_t *info_row = lv_obj_create(page);
    lv_obj_remove_style_all(info_row);
    lv_obj_set_size(info_row, 800 - 32, 160);
    lv_obj_set_flex_flow(info_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(info_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(info_row, LV_ALIGN_CENTER, 0, 30);
    lv_obj_clear_flag(info_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *volt_card = lv_obj_create(info_row);
    lv_obj_remove_style_all(volt_card);
    lv_obj_set_size(volt_card, 220, 140);
    lv_obj_set_style_radius(volt_card, 12, 0);
    lv_obj_set_style_bg_color(volt_card, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(volt_card, LV_OPA_10, 0);
    lv_obj_set_style_border_width(volt_card, 1, 0);
    lv_obj_set_style_border_color(volt_card, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_border_opa(volt_card, LV_OPA_40, 0);
    lv_obj_clear_flag(volt_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_volt_title = lv_label_create(volt_card);
    lv_obj_add_style(lbl_volt_title, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_volt_title, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_volt_title, "Pack Voltage");
    lv_obj_align(lbl_volt_title, LV_ALIGN_TOP_MID, 0, 16);

    lbl_pack_volt2 = lv_label_create(volt_card);
    lv_obj_add_style(lbl_pack_volt2, &ui_style_label_big, 0);
    lv_label_set_text(lbl_pack_volt2, "-- V");
    lv_obj_align(lbl_pack_volt2, LV_ALIGN_BOTTOM_MID, 0, -16);

    /* sys_temp_c(placeholder, CAN 0x307) — 경고 시 카드 테두리/텍스트를 RED로 전환 */
    card_sys_temp = lv_obj_create(info_row);
    lv_obj_remove_style_all(card_sys_temp);
    lv_obj_set_size(card_sys_temp, 220, 140);
    lv_obj_set_style_radius(card_sys_temp, 12, 0);
    lv_obj_set_style_bg_color(card_sys_temp, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(card_sys_temp, LV_OPA_10, 0);
    lv_obj_set_style_border_width(card_sys_temp, 1, 0);
    lv_obj_set_style_border_color(card_sys_temp, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_border_opa(card_sys_temp, LV_OPA_40, 0);
    lv_obj_clear_flag(card_sys_temp, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_temp_title = lv_label_create(card_sys_temp);
    lv_obj_add_style(lbl_temp_title, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_temp_title, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_temp_title, "Sys Temp");
    lv_obj_align(lbl_temp_title, LV_ALIGN_TOP_MID, 0, 16);

    lbl_sys_temp = lv_label_create(card_sys_temp);
    lv_obj_add_style(lbl_sys_temp, &ui_style_label_big, 0);
    lv_label_set_text(lbl_sys_temp, "--");
    lv_obj_align(lbl_sys_temp, LV_ALIGN_BOTTOM_MID, 0, -16);
}

/* ---------------------------------------------------------------------
 * Page 4 — 기기 조작 및 연결성
 * ------------------------------------------------------------------- */
static lv_obj_t *make_status_dot(lv_obj_t *parent)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 22, 22);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    return dot;
}

static void build_page_connect(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv, 3);
    pages[3] = page;

    dot_ble = make_status_dot(page);
    lv_obj_align(dot_ble, LV_ALIGN_TOP_LEFT, 32, 48);

    lbl_ble_status = lv_label_create(page);
    lv_obj_add_style(lbl_ble_status, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_ble_status, "BLE Disconnected");
    lv_obj_align_to(lbl_ble_status, dot_ble, LV_ALIGN_OUT_RIGHT_MID, 16, 0);

    dot_vehicle = make_status_dot(page);
    lv_obj_align_to(dot_vehicle, dot_ble, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 40);

    lv_obj_t *lbl_vehicle = lv_label_create(page);
    lv_obj_add_style(lbl_vehicle, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_vehicle, "Vehicle Status: OK");
    lv_obj_align_to(lbl_vehicle, dot_vehicle, LV_ALIGN_OUT_RIGHT_MID, 16, 0);

    /* 남는 공간에 실제 고정값(BLE 기기명, main/ble.c의 ble_svc_gap_device_name_set과 동일)을
     * 채워서 지어낸 데이터 없이도 화면을 더 채운다. */
    lv_obj_t *device_row = lv_obj_create(page);
    lv_obj_remove_style_all(device_row);
    lv_obj_set_size(device_row, 800 - 64, 100);
    lv_obj_set_flex_flow(device_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(device_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(device_row, LV_ALIGN_BOTTOM_MID, 0, -110);
    lv_obj_clear_flag(device_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *device_card = lv_obj_create(device_row);
    lv_obj_remove_style_all(device_card);
    lv_obj_set_size(device_card, 220, 90);
    lv_obj_set_style_radius(device_card, 10, 0);
    lv_obj_set_style_bg_color(device_card, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(device_card, LV_OPA_10, 0);
    lv_obj_set_style_border_width(device_card, 1, 0);
    lv_obj_set_style_border_color(device_card, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_border_opa(device_card, LV_OPA_40, 0);
    lv_obj_clear_flag(device_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_device_title = lv_label_create(device_card);
    lv_obj_add_style(lbl_device_title, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_device_title, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_device_title, "DEVICE");
    lv_obj_align(lbl_device_title, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *lbl_device_value = lv_label_create(device_card);
    lv_obj_add_style(lbl_device_value, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl_device_value, UI_COLOR_TEXT_PRI, 0);
    lv_label_set_text(lbl_device_value, "ESP32S3-Cluster");
    lv_obj_align(lbl_device_value, LV_ALIGN_BOTTOM_MID, 0, -12);

    /* HARNESS-TODO: 시계/외기온도 — RTC/온도센서 소스 미확정, 소스 확정 전 stub 유지 */
    lv_obj_t *lbl_clock;
    lv_obj_t *clock_card = make_info_card(page, "Time", &lbl_clock);
    lv_obj_align(clock_card, LV_ALIGN_BOTTOM_LEFT, 16, -16);

    lv_obj_t *lbl_outtemp;
    lv_obj_t *outtemp_card = make_info_card(page, "Out Temp", &lbl_outtemp);
    lv_obj_align(outtemp_card, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
}

/* ---------------------------------------------------------------------
 * 페이지 인디케이터 (하단 중앙 dot 4개)
 * ------------------------------------------------------------------- */
static void build_page_indicator(lv_obj_t *parent)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, 80, 12);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(row, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *dot = lv_obj_create(row);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, i == 0 ? UI_COLOR_CYAN : UI_COLOR_TEXT_SEC, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        page_dots[i] = dot;
    }
}

/* 페이지 전환 시 현재 타일에 대응하는 인디케이터 dot만 CYAN으로 하이라이트 */
static void tileview_event_cb(lv_event_t *e)
{
    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *act = lv_tileview_get_tile_act(tv);

    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_bg_color(page_dots[i],
            pages[i] == act ? UI_COLOR_CYAN : UI_COLOR_TEXT_SEC, 0);
    }
}

/* ---------------------------------------------------------------------
 * public API
 * ------------------------------------------------------------------- */
void ui_init(lv_obj_t *parent)
{
    ui_style_init();

    lv_obj_add_style(parent, &ui_style_bg, 0);

    tileview = lv_tileview_create(parent);
    lv_obj_set_size(tileview, lv_pct(100), lv_pct(100));
    lv_obj_add_style(tileview, &ui_style_bg, 0);

    build_page_drive(tileview);
    build_page_battery(tileview);
    build_page_diag(tileview);
    build_page_connect(tileview);

    build_page_indicator(parent);

    lv_obj_add_event_cb(tileview, tileview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/* speed/soc 게이지+숫자를 한 애니메이션으로 같이 움직여서(exec_cb 안에서 둘 다 갱신)
 * 실차 클러스터처럼 값이 순간 이동하지 않고 부드럽게 스윕되도록 한다. */
#define UI_GAUGE_ANIM_TIME_MS 250

static void anim_speed_exec_cb(void *var, int32_t v)
{
    (void)var;
    /* 세그먼트별로 [start,end] 구간 중 v로 덮이는 만큼만 채운다 — v 이전 구간은 완전히
     * 채워지고, v가 속한 구간은 부분적으로, 이후 구간은 비어있는(start=end) 채로 남는다. */
    for (int i = 0; i < UI_SPEED_GRAD_SEGMENTS; i++) {
        int32_t seg_start = speed_seg_bounds[i];
        int32_t seg_end = speed_seg_bounds[i + 1];
        int32_t filled_end = v > seg_end ? seg_end : (v < seg_start ? seg_start : v);
        lv_meter_set_indicator_end_value(meter_speed, speed_grad_segs[i], filled_end);
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", (unsigned)v);
    lv_label_set_text(lbl_speed, buf);
    /* 자릿수가 바뀌면 라벨 폭도 바뀌는데, align_to는 최초 호출 시점 폭 기준으로 한 번만
     * 위치를 잡아서 이후 폭이 늘어난 만큼 오른쪽으로 밀려 보인다 — 매번 다시 정렬해야 함. */
    lv_obj_align_to(lbl_speed, meter_speed, LV_ALIGN_CENTER, 0, -12);
}

static void animate_speed_to(int32_t target)
{
    static int32_t last_target = -1;
    static int32_t current = 0;
    if (target == last_target) return;
    last_target = target;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, meter_speed);
    lv_anim_set_exec_cb(&a, anim_speed_exec_cb);
    lv_anim_set_values(&a, current, target);
    lv_anim_set_time(&a, UI_GAUGE_ANIM_TIME_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
    current = target;
}

static void anim_soc_exec_cb(void *var, int32_t v)
{
    (void)var;
    lv_meter_set_indicator_end_value(meter_soc, soc_value_indic, v);
    /* lv_meter 인디케이터는 색을 바꾸는 공개 setter가 없어, 노출된(비-opaque) 구조체
     * 필드를 직접 갱신하고 무효화해서 다시 그리게 한다(공식 헤더에 문서화된 필드). */
    soc_value_indic->type_data.arc.color = (v <= UI_SOC_LOW_PCT) ? UI_COLOR_RED : UI_COLOR_CYAN;
    lv_obj_invalidate(meter_soc);
    char buf[16];
    snprintf(buf, sizeof(buf), "%u%%", (unsigned)v);
    lv_label_set_text(lbl_soc_pct, buf);
    /* speed와 동일한 이유로 매번 재정렬 필요 (자릿수 변화로 폭이 바뀜) */
    lv_obj_align_to(lbl_soc_pct, meter_soc, LV_ALIGN_CENTER, 0, 0);
}

static void animate_soc_to(int32_t target)
{
    static int32_t last_target = -1;
    static int32_t current = 0;
    if (target == last_target) return;
    last_target = target;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, meter_soc);
    lv_anim_set_exec_cb(&a, anim_soc_exec_cb);
    lv_anim_set_values(&a, current, target);
    lv_anim_set_time(&a, UI_GAUGE_ANIM_TIME_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
    current = target;
}

void ui_update(void)
{
    VehicleData_t d;
    vehicle_data_get(&d);

    char buf[32];

    /* Page 1 */
    animate_speed_to(d.speed);

    {
        static const char *gear_text[4] = { "P", "R", "N", "D" };
        lv_label_set_text(lbl_gear, d.drive_mode < 4 ? gear_text[d.drive_mode] : "--");
    }
    snprintf(buf, sizeof(buf), "%lu km", (unsigned long)d.odo_km);
    lv_label_set_text(lbl_odo, buf);

    /* Page 2 */
    animate_soc_to(d.soc);

    snprintf(buf, sizeof(buf), "%.1f V", d.pack_volt);
    lv_label_set_text(lbl_pack_volt, buf);

    snprintf(buf, sizeof(buf), "%u km", (unsigned)d.range_km);
    lv_label_set_text(lbl_range, buf);

    snprintf(buf, sizeof(buf), "%.1f kW\nRegen %.1f kW", d.power_kw, d.regen_kw);
    lv_label_set_text(lbl_power_regen, buf);

    /* Page 3 */
    if (d.dtc_code == 0) {
        lv_label_set_text(lbl_dtc_code, "DTC: OK");
        lv_obj_set_style_bg_color(banner_dtc, UI_COLOR_TEXT_SEC, 0);
        lv_obj_set_style_bg_opa(banner_dtc, LV_OPA_20, 0);
    } else {
        snprintf(buf, sizeof(buf), "DTC: 0x%04X", (unsigned)d.dtc_code);
        lv_label_set_text(lbl_dtc_code, buf);
        lv_obj_set_style_bg_color(banner_dtc, UI_COLOR_RED, 0);
        lv_obj_set_style_bg_opa(banner_dtc, LV_OPA_50, 0);
    }

    snprintf(buf, sizeof(buf), "%.1f V", d.pack_volt);
    lv_label_set_text(lbl_pack_volt2, buf);

    snprintf(buf, sizeof(buf), "%d C", (int)d.sys_temp_c);
    lv_label_set_text(lbl_sys_temp, buf);
    bool temp_warn = d.sys_temp_c >= UI_TEMP_WARN_C;
    lv_obj_set_style_border_color(card_sys_temp, temp_warn ? UI_COLOR_RED : UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_border_opa(card_sys_temp, temp_warn ? LV_OPA_COVER : LV_OPA_40, 0);
    lv_obj_set_style_text_color(lbl_sys_temp, temp_warn ? UI_COLOR_RED : UI_COLOR_TEXT_PRI, 0);

    /* Page 4 */
    lv_obj_set_style_bg_color(dot_ble, d.ble_connected ? UI_COLOR_GREEN : UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_ble_status, d.ble_connected ? "BLE Connected" : "BLE Disconnected");

    lv_obj_set_style_bg_color(dot_vehicle, d.dtc_code == 0 ? UI_COLOR_GREEN : UI_COLOR_RED, 0);
}
