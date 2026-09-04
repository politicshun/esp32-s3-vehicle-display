#include "ui_tile_ride.h"
#include "ui_style.h"
#include "ui_widget_cellbar.h"
#include "ui_widget_readout.h"
#include "ui_trip_state.h"
#include "ui_fonts_num.h"
#include "ui_fonts_label.h"
#include "vehicle_data.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>

/*
 * ui_tile_ride.c — Voltline Ride 탭 (phase 6, 계획 §5/§7-1).
 * 스펙: docs/Voltline 전기오토바이 클러스터 UXUI/Voltline Cluster.dc.html
 *       (isRide 블록, 약 82~134행) + Cluster Dev Spec.dc.html §Ride 행.
 *
 * grid 172 / 1fr / 172, rows 42 / 1fr.
 *   row1 col2: 기어 배지(스펙은 Eco/City/Sport 모드 필이지만, 2026-09-02 사용자
 *              확정으로 실제 P/R/N/D 읽기전용 배지로 대체 — 탭해도 안 바뀜).
 *   row2 col1: BATTERY 카드 (SOC% + 10셀 바 + RANGE)
 *   row2 col2: 속도 숫자 + REGEN<->POWER 41셀 플로우 스트립
 *   row2 col3: TRIP A / ODO 카드
 *
 * 글로우(스펙의 radial-gradient 속도 글로우, gen_glow_image.py 베이크 이미지 +
 * lv_img_set_zoom)는 실기기 검증이 필요해 이번 phase에서는 보류
 * (HARNESS-TODO: 실기기 확보 후 추가, 계획 §6 phase6 각주 참고).
 */

/* 2026-09-03: 내부 SRAM 예산 초과로 실기기 크래시(project_2026-09-02_voltline_5tile_ui_rewrite
 * 메모리 참고) — flow strip 해상도를 20->10(총 41->21 lv_obj_t)으로 낮춤. */
#define FLOW_HALF_CELLS 10
#define REGEN_MAX_KW 3.0f  /* Cluster Dev Spec.dc.html §7 "Motor power -3.0..+14.0 kW" */
#define POWER_MAX_KW 14.0f

static lv_obj_t *s_lbl_gear;

static lv_obj_t *s_lbl_soc_val;
static lv_obj_t *s_cellbar_soc;
static lv_obj_t *s_lbl_range_val;

static lv_obj_t *s_lbl_speed;

static lv_obj_t *s_lbl_regen_label;
static lv_obj_t *s_lbl_regen_val;
static lv_obj_t *s_lbl_power_val;
static lv_obj_t *s_lbl_power_label;
static lv_obj_t *s_flow_regen[FLOW_HALF_CELLS];
static lv_obj_t *s_flow_power[FLOW_HALF_CELLS];

static lv_obj_t *s_lbl_trip_val;
static lv_obj_t *s_lbl_odo_val;

static lv_color_t soc_energy_color(int soc)
{
    if (soc > UI_ENERGY_THRESH_FULL_PCT) return UI_ENERGY_FULL;
    if (soc > UI_ENERGY_THRESH_MID_PCT) return UI_ENERGY_MID;
    if (soc > UI_ENERGY_THRESH_LOW_PCT) return UI_ENERGY_LOW;
    return UI_ENERGY_EMPTY;
}

static lv_obj_t *build_side_card(lv_obj_t *parent, uint8_t col)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_add_style(card, &ui_style_panel, 0);
    lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

static lv_obj_t *build_caption(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_add_style(lbl, &ui_style_text_secondary, 0);
    lv_obj_set_style_text_font(lbl, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(lbl, 1, 0); /* 스펙 0.08em @13px */
    lv_label_set_text(lbl, text);
    return lbl;
}

static void build_battery_card(lv_obj_t *parent)
{
    lv_obj_t *card = build_side_card(parent, 0);

    build_caption(card, "BATTERY");
    s_lbl_soc_val = ui_widget_readout_create(card, &ui_font_num_60, UI_TEXT_PRIMARY, "%");

    lv_obj_t *bar_row = lv_obj_create(card);
    lv_obj_remove_style_all(bar_row);
    lv_obj_set_size(bar_row, lv_pct(100), 22);
    lv_obj_set_flex_flow(bar_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(bar_row, 2, 0);
    lv_obj_clear_flag(bar_row, LV_OBJ_FLAG_SCROLLABLE);

    s_cellbar_soc = ui_widget_cellbar_create(bar_row, UI_SOC_CELL_COUNT);
    lv_obj_set_flex_grow(s_cellbar_soc, 1);

    /* 우측 3x10 nub (스펙: 셸 밖 장식, 위젯에 포함 안 함) */
    lv_obj_t *nub = lv_obj_create(bar_row);
    lv_obj_remove_style_all(nub);
    lv_obj_set_size(nub, 3, 10);
    lv_obj_set_style_radius(nub, 2, 0);
    lv_obj_set_style_bg_color(nub, UI_LINE_DEFAULT, 0);
    lv_obj_set_style_bg_opa(nub, LV_OPA_COVER, 0);
    lv_obj_clear_flag(nub, LV_OBJ_FLAG_SCROLLABLE);

    build_caption(card, "RANGE");
    s_lbl_range_val = ui_widget_readout_create(card, &ui_font_num_46, UI_ACCENT, "km"); /* 스펙: RANGE 값=accent */
}

static void build_trip_card(lv_obj_t *parent)
{
    lv_obj_t *card = build_side_card(parent, 2);

    build_caption(card, "TRIP A");
    s_lbl_trip_val = ui_widget_readout_create(card, &ui_font_num_46, UI_TEXT_PRIMARY, "km");

    lv_obj_t *hr = lv_obj_create(card);
    lv_obj_remove_style_all(hr);
    lv_obj_set_size(hr, lv_pct(100), 1);
    lv_obj_set_style_bg_color(hr, UI_HAIRLINE, 0);
    lv_obj_set_style_bg_opa(hr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(hr, LV_OBJ_FLAG_SCROLLABLE);

    build_caption(card, "ODO");
    s_lbl_odo_val = ui_widget_readout_create(card, &ui_font_num_34, UI_TEXT_PRIMARY, "km");
}

static void build_flow_strip(lv_obj_t *parent)
{
    lv_obj_t *header_row = lv_obj_create(parent);
    lv_obj_remove_style_all(header_row);
    lv_obj_set_size(header_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(header_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *left = lv_obj_create(header_row);
    lv_obj_remove_style_all(left);
    lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left, 8, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);
    s_lbl_regen_label = lv_label_create(left);
    lv_obj_set_style_text_font(s_lbl_regen_label, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(s_lbl_regen_label, 1, 0);
    lv_obj_set_style_text_color(s_lbl_regen_label, UI_TEXT_TERTIARY, 0);
    lv_label_set_text(s_lbl_regen_label, "REGEN");
    s_lbl_regen_val = lv_label_create(left);
    lv_obj_set_style_text_font(s_lbl_regen_val, &ui_font_label_13, 0);
    lv_obj_set_style_text_color(s_lbl_regen_val, UI_ACCENT, 0);
    lv_label_set_text(s_lbl_regen_val, "");

    lv_obj_t *right = lv_obj_create(header_row);
    lv_obj_remove_style_all(right);
    lv_obj_set_size(right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right, 8, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);
    s_lbl_power_val = lv_label_create(right);
    lv_obj_set_style_text_font(s_lbl_power_val, &ui_font_label_13, 0);
    lv_obj_set_style_text_color(s_lbl_power_val, UI_SIGNAL_CAUTION, 0);
    lv_label_set_text(s_lbl_power_val, "");
    s_lbl_power_label = lv_label_create(right);
    lv_obj_set_style_text_font(s_lbl_power_label, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(s_lbl_power_label, 1, 0);
    lv_obj_set_style_text_color(s_lbl_power_label, UI_TEXT_TERTIARY, 0);
    lv_label_set_text(s_lbl_power_label, "POWER");

    lv_obj_t *strip = lv_obj_create(parent);
    lv_obj_remove_style_all(strip);
    lv_obj_set_size(strip, lv_pct(100), 18);
    lv_obj_set_flex_flow(strip, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(strip, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(strip, 2, 0);
    lv_obj_clear_flag(strip, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < FLOW_HALF_CELLS; i++) {
        lv_obj_t *cell = lv_obj_create(strip);
        lv_obj_remove_style_all(cell);
        lv_obj_set_flex_grow(cell, 2);
        lv_obj_set_height(cell, lv_pct(100));
        lv_obj_set_style_radius(cell, 1, 0);
        lv_obj_set_style_bg_color(cell, UI_TRACK_EMPTY, 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        s_flow_regen[i] = cell;
    }

    lv_obj_t *tick = lv_obj_create(strip);
    lv_obj_remove_style_all(tick);
    lv_obj_set_flex_grow(tick, 1);
    lv_obj_set_height(tick, lv_pct(100));
    lv_obj_set_style_bg_color(tick, UI_TEXT_PRIMARY, 0);
    lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);
    lv_obj_clear_flag(tick, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < FLOW_HALF_CELLS; i++) {
        lv_obj_t *cell = lv_obj_create(strip);
        lv_obj_remove_style_all(cell);
        lv_obj_set_flex_grow(cell, 2);
        lv_obj_set_height(cell, lv_pct(100));
        lv_obj_set_style_radius(cell, 1, 0);
        lv_obj_set_style_bg_color(cell, UI_TRACK_EMPTY, 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        s_flow_power[i] = cell;
    }
}

static void build_center_column(lv_obj_t *parent)
{
    lv_obj_t *col = lv_obj_create(parent);
    lv_obj_remove_style_all(col);
    lv_obj_set_grid_cell(col, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(col, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *speed_wrap = lv_obj_create(col);
    lv_obj_remove_style_all(speed_wrap);
    lv_obj_set_width(speed_wrap, lv_pct(100));
    lv_obj_set_flex_grow(speed_wrap, 1);
    lv_obj_clear_flag(speed_wrap, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *speed_row = lv_obj_create(speed_wrap);
    lv_obj_remove_style_all(speed_row);
    lv_obj_set_size(speed_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(speed_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(speed_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(speed_row, 10, 0);
    lv_obj_clear_flag(speed_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(speed_row);

    /* speedInt는 |speed| — drive_mode(P/R/N/D) 배지가 방향을 이미 보여주므로 숫자
     * 자체는 부호 없이 표시. 100 기준 250px<->190px 폰트 스왑은 ui_tile_ride_update()에서
     * 매 프레임 판정(phase 7, 계획 §5/§3). */
    s_lbl_speed = lv_label_create(speed_row);
    lv_obj_set_style_text_font(s_lbl_speed, &ui_font_num_250, 0);
    lv_obj_set_style_text_color(s_lbl_speed, UI_TEXT_PRIMARY, 0);
    lv_label_set_text(s_lbl_speed, "-");

    lv_obj_t *speed_unit = lv_label_create(speed_row);
    lv_obj_set_style_text_font(speed_unit, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(speed_unit, UI_TEXT_SECONDARY, 0);
    lv_label_set_text(speed_unit, "km/h");

    build_flow_strip(col);
}

lv_obj_t *ui_tile_ride_build(lv_obj_t *tile_parent)
{
    lv_obj_t *tile = lv_tileview_add_tile(tile_parent, 0, 0, LV_DIR_NONE);
    lv_obj_add_style(tile, &ui_style_bg, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

    static lv_coord_t col_dsc[] = {172, LV_GRID_FR(1), 172, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {42, LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(tile, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(tile, 4, 0);
    lv_obj_set_style_pad_column(tile, 16, 0);
    lv_obj_set_style_pad_row(tile, 8, 0);

    /* row1 col2: 기어 배지 (읽기전용, 2026-09-02 사용자 확정 — Eco/City/Sport 모드 필 대체) */
    lv_obj_t *gear_wrap = lv_obj_create(tile);
    lv_obj_remove_style_all(gear_wrap);
    lv_obj_set_grid_cell(gear_wrap, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_size(gear_wrap, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_clear_flag(gear_wrap, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *badge = lv_obj_create(gear_wrap);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, LV_SIZE_CONTENT, 30);
    lv_obj_set_style_pad_hor(badge, 22, 0);
    lv_obj_set_style_border_width(badge, 2, 0); /* 스펙 1.5px는 소수점 미지원이라 반올림 */
    lv_obj_set_style_border_color(badge, UI_LINE_ACCENT, 0);
    lv_obj_set_style_border_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(badge, UI_RADIUS_PILL, 0);
    lv_obj_set_flex_flow(badge, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(badge, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_gear = lv_label_create(badge);
    lv_obj_set_style_text_font(s_lbl_gear, &ui_font_label_15, 0);
    lv_obj_set_style_text_letter_space(s_lbl_gear, 2, 0); /* 스펙 0.16em @15px */
    lv_obj_set_style_text_color(s_lbl_gear, UI_ACCENT, 0);
    lv_label_set_text(s_lbl_gear, "-");

    build_battery_card(tile);
    build_center_column(tile);
    build_trip_card(tile);

    return tile;
}

/* 2026-09-04: full_refresh=1(main/lvgl.c)에서는 lv_label_set_text/lv_obj_set_style_*
 * 호출 자체가 내용이 그대로여도 매번 lv_obj_invalidate()를 부른다 — 이 탭은 CAN
 * 데이터와 무관하게 20ms마다 통째로 다시 계산·재기록되고 있었어서 handler_avg가
 * 항상 180ms(예산의 9배)에 머무는 근본 원인이었다(ui_widget_cellbar.c 상단 주석
 * 참고). 아래는 ui_chrome.c의 ble_prev/dtc_prev 가드와 같은 패턴 — 화면에 실제로
 * 찍히는 값이 바뀔 때만 LVGL 호출을 한다. */
void ui_tile_ride_update(void)
{
    VehicleData_t d;
    vehicle_data_get(&d);

    static int gear_prev = -1;
    int gear_now = d.drive_mode < 4 ? (int)d.drive_mode : 4;
    if (gear_now != gear_prev) {
        gear_prev = gear_now;
        static const char *gear_text[4] = {"P", "R", "N", "D"};
        lv_label_set_text(s_lbl_gear, gear_now < 4 ? gear_text[gear_now] : "--");
    }

    char buf[24];
    static unsigned soc_prev = UINT32_MAX;
    if ((unsigned)d.soc != soc_prev) {
        soc_prev = (unsigned)d.soc;
        snprintf(buf, sizeof(buf), "%u", (unsigned)d.soc);
        lv_label_set_text(s_lbl_soc_val, buf);
        lv_color_t soc_color = soc_energy_color(d.soc);
        lv_obj_set_style_text_color(s_lbl_soc_val, d.soc > UI_ENERGY_THRESH_MID_PCT ? UI_TEXT_PRIMARY
                                     : (d.soc > UI_BATTERY_LOW_PCT ? UI_SIGNAL_CAUTION : UI_SIGNAL_CRITICAL), 0);
        ui_widget_cellbar_set(s_cellbar_soc, (int)lroundf(d.soc / (100.0f / UI_SOC_CELL_COUNT)), soc_color);
    }

    static unsigned range_prev = UINT32_MAX;
    if ((unsigned)d.range_km != range_prev) {
        range_prev = (unsigned)d.range_km;
        snprintf(buf, sizeof(buf), "%u", (unsigned)d.range_km);
        lv_label_set_text(s_lbl_range_val, buf);
    }

    int speed_abs = d.speed < 0 ? -d.speed : d.speed;
    static int speed_abs_prev = INT32_MIN;
    static int speed_dtc_prev = -1;
    int speed_dtc_now = d.dtc_code != 0 ? 1 : 0;
    if (speed_abs != speed_abs_prev || speed_dtc_now != speed_dtc_prev) {
        speed_abs_prev = speed_abs;
        speed_dtc_prev = speed_dtc_now;
        snprintf(buf, sizeof(buf), "%d", speed_abs);
        lv_label_set_text(s_lbl_speed, buf);
        lv_obj_set_style_text_color(s_lbl_speed, speed_dtc_now ? UI_SIGNAL_CAUTION : UI_TEXT_PRIMARY, 0);
    }
    /* 스펙 §3: 100 기준 250px<->190px 폰트 스왑. 폰트 변경은 값이 임계값을 넘나들 때만
     * (매 프레임 무조건 set_style 금지 — ui_chrome.c의 BLE 램프와 같은 패턴). */
    static int speed_font_is_small = -1;
    int want_small = speed_abs >= 100 ? 1 : 0;
    if (want_small != speed_font_is_small) {
        speed_font_is_small = want_small;
        lv_obj_set_style_text_font(s_lbl_speed, want_small ? &ui_font_num_190 : &ui_font_num_250, 0);
    }

    int regen_lit = (int)lroundf(LV_CLAMP(0.0f, d.regen_kw / REGEN_MAX_KW, 1.0f) * FLOW_HALF_CELLS);
    int power_lit = (int)lroundf(LV_CLAMP(0.0f, d.power_kw / POWER_MAX_KW, 1.0f) * FLOW_HALF_CELLS);
    static int regen_lit_prev = INT32_MIN, power_lit_prev = INT32_MIN;
    if (regen_lit != regen_lit_prev || power_lit != power_lit_prev) {
        regen_lit_prev = regen_lit;
        power_lit_prev = power_lit;
        for (int i = 0; i < FLOW_HALF_CELLS; i++) {
            int from = FLOW_HALF_CELLS - i; /* 스펙: regen은 tick에서 왼쪽으로 채워짐 */
            lv_obj_set_style_bg_color(s_flow_regen[i], from <= regen_lit ? UI_ENERGY_REGEN : UI_TRACK_EMPTY, 0);
            lv_obj_set_style_bg_color(s_flow_power[i], (i + 1) <= power_lit ? UI_SIGNAL_CAUTION : UI_TRACK_EMPTY, 0);
        }
    }

    static int regen_dv_prev = INT32_MIN; /* -1은 "0.05kW 이하(빈 문자열)" 상태 표시용으로 씀 */
    int regen_active = d.regen_kw > 0.05f;
    int regen_dv = regen_active ? (int)lroundf(d.regen_kw * 10.0f) : -1;
    if (regen_dv != regen_dv_prev) {
        regen_dv_prev = regen_dv;
        if (regen_active) {
            snprintf(buf, sizeof(buf), "%.1f kW", (double)d.regen_kw);
            lv_label_set_text(s_lbl_regen_val, buf);
            lv_obj_set_style_text_color(s_lbl_regen_label, UI_ENERGY_REGEN, 0);
        } else {
            lv_label_set_text(s_lbl_regen_val, "");
            lv_obj_set_style_text_color(s_lbl_regen_label, UI_TEXT_TERTIARY, 0);
        }
    }
    static int power_dv_prev = INT32_MIN;
    int power_active = d.power_kw > 0.05f;
    int power_dv = power_active ? (int)lroundf(d.power_kw * 10.0f) : -1;
    if (power_dv != power_dv_prev) {
        power_dv_prev = power_dv;
        if (power_active) {
            snprintf(buf, sizeof(buf), "%.1f kW", (double)d.power_kw);
            lv_label_set_text(s_lbl_power_val, buf);
            lv_obj_set_style_text_color(s_lbl_power_label, UI_SIGNAL_CAUTION, 0);
        } else {
            lv_label_set_text(s_lbl_power_val, "");
            lv_obj_set_style_text_color(s_lbl_power_label, UI_TEXT_TERTIARY, 0);
        }
    }

    ui_trip_stats_t trip;
    ui_trip_state_get(&trip);
    static int trip_dv_prev = INT32_MIN;
    int trip_dv = (int)lroundf(trip.dist_km * 10.0f);
    if (trip_dv != trip_dv_prev) {
        trip_dv_prev = trip_dv;
        snprintf(buf, sizeof(buf), "%.1f", (double)trip.dist_km);
        lv_label_set_text(s_lbl_trip_val, buf);
    }

    static unsigned odo_prev = UINT32_MAX;
    if ((unsigned)d.odo_km != odo_prev) {
        odo_prev = (unsigned)d.odo_km;
        snprintf(buf, sizeof(buf), "%u", (unsigned)d.odo_km);
        lv_label_set_text(s_lbl_odo_val, buf);
    }
}
