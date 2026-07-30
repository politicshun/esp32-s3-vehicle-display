#include "ui.h"
#include "ui_style.h"
#include "vehicle_data.h"
#include <stdio.h>

/*
 * main/include/vehicle_data.h 의 VehicleData_t 에는 아래 5개 필드만 있다:
 *   speed(uint16_t), soc(uint8_t), pack_volt(float), dtc_code(uint16_t), ble_connected(bool)
 *
 * 레퍼런스 디자인(핵심 표시 항목 표)에 있던 Drive Mode/ODO/TRIP/Range/실시간 출력/
 * 팩 온도/시계/외기온도는 현재 VehicleData_t에 소스가 없다. CLAUDE.md 0번 원칙
 * ("확인할 수 없는 사양은 존재하지 않는 사양")에 따라 구조체에 없는 필드를 지어내지 않고,
 * 레이아웃만 남겨둔 채 "HARNESS-TODO" 정적 placeholder로 표시한다.
 * 보드 실구동 후 확장 필드가 확정되면 vehicle_data.h에 필드 추가 -> 아래 ui_update()의
 * 각 HARNESS-TODO 블록만 채우면 된다.
 */

static lv_obj_t *tileview;

/* Page 1 — 주행 필수 정보 */
static lv_obj_t *lbl_speed;
static lv_obj_t *lbl_gear_todo;   /* HARNESS-TODO: gear 필드 없음 */
static lv_obj_t *lbl_odo_todo;    /* HARNESS-TODO: odo 필드 없음 */
static lv_obj_t *lbl_trip_todo;   /* HARNESS-TODO: trip 필드 없음 */

/* Page 2 — 전력 및 배터리 */
static lv_obj_t *arc_soc;
static lv_obj_t *lbl_soc_pct;
static lv_obj_t *lbl_pack_volt;
static lv_obj_t *lbl_range_todo;   /* HARNESS-TODO: range 필드 없음 */
static lv_obj_t *lbl_output_todo;  /* HARNESS-TODO: 실시간 출력 필드 없음 */

/* Page 3 — 시스템 상태 및 진단 */
static lv_obj_t *banner_dtc;
static lv_obj_t *lbl_dtc_code;
static lv_obj_t *lbl_pack_volt2;     /* page3 전용 전압 라벨 (page2의 lbl_pack_volt와 별개 위젯) */
static lv_obj_t *lbl_pack_temp_todo; /* HARNESS-TODO: pack_temp 필드 없음 */

/* Page 4 — 기기 조작 및 연결성 */
static lv_obj_t *dot_ble;
static lv_obj_t *lbl_ble_status;
static lv_obj_t *dot_vehicle;

/* ---------------------------------------------------------------------
 * 공통 헬퍼
 * ------------------------------------------------------------------- */
static lv_obj_t *make_page(lv_obj_t *tv)
{
    lv_obj_t *page = lv_tileview_add_tile(tv, 0, 0, LV_DIR_HOR);
    lv_obj_add_style(page, &ui_style_bg, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
}

/* 아직 데이터 소스가 없는 항목용 — 라벨을 만들고 static "확인필요" 텍스트로 채운다.
 * ui_update()에서 값을 갱신하지 않는다(할 데이터가 없으므로). */
static lv_obj_t *make_todo_label(lv_obj_t *parent, const char *title)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_obj_add_style(lbl, &ui_style_label_small, 0);
    lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text_fmt(lbl, "%s --", title);
    return lbl;
}

/* ---------------------------------------------------------------------
 * Page 1 — 주행 필수 정보
 * ------------------------------------------------------------------- */
static void build_page_drive(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv);

    /* HARNESS-TODO: gear(N/D) 필드가 VehicleData_t에 없어 뱃지 대신 안내 라벨만 배치.
     * 필드 추가되면 make_gear_badge 형태(이전 리비전 참고)로 교체 가능. */
    lbl_gear_todo = make_todo_label(page, "GEAR");
    lv_obj_align(lbl_gear_todo, LV_ALIGN_TOP_LEFT, 16, 16);

    lbl_speed = lv_label_create(page);
    lv_obj_add_style(lbl_speed, &ui_style_label_big, 0);
    lv_label_set_text(lbl_speed, "0");
    lv_obj_align(lbl_speed, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *unit = lv_label_create(page);
    lv_obj_add_style(unit, &ui_style_label_small, 0);
    lv_label_set_text(unit, "km/h");
    lv_obj_align_to(unit, lbl_speed, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    lbl_odo_todo = make_todo_label(page, "ODO");
    lv_obj_align(lbl_odo_todo, LV_ALIGN_BOTTOM_LEFT, 16, -16);

    lbl_trip_todo = make_todo_label(page, "TRIP");
    lv_obj_align(lbl_trip_todo, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
}

/* ---------------------------------------------------------------------
 * Page 2 — 전력 및 배터리
 * ------------------------------------------------------------------- */
static void build_page_battery(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv);

    arc_soc = lv_arc_create(page);
    lv_obj_set_size(arc_soc, 220, 220);
    lv_arc_set_rotation(arc_soc, 135);
    lv_arc_set_bg_angles(arc_soc, 0, 270);
    lv_arc_set_range(arc_soc, 0, 100);
    lv_arc_set_value(arc_soc, 0);
    lv_obj_remove_style(arc_soc, NULL, LV_PART_KNOB); /* 표시 전용, 조작 불가 */
    lv_obj_clear_flag(arc_soc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc_soc, UI_COLOR_CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_soc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_soc, 14, LV_PART_MAIN);
    lv_obj_align(arc_soc, LV_ALIGN_TOP_MID, 0, 16);

    lbl_soc_pct = lv_label_create(page);
    lv_obj_add_style(lbl_soc_pct, &ui_style_label_big, 0);
    lv_label_set_text(lbl_soc_pct, "0%");
    lv_obj_align_to(lbl_soc_pct, arc_soc, LV_ALIGN_CENTER, 0, 0);

    lbl_pack_volt = lv_label_create(page);
    lv_obj_add_style(lbl_pack_volt, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_pack_volt, "-- V");
    lv_obj_align(lbl_pack_volt, LV_ALIGN_BOTTOM_MID, 0, -60);

    /* HARNESS-TODO: range/실시간 출력(kW) 필드 없음 — 레이아웃만 예약 */
    lbl_range_todo = make_todo_label(page, "Range");
    lv_obj_align(lbl_range_todo, LV_ALIGN_BOTTOM_LEFT, 16, -16);

    lbl_output_todo = make_todo_label(page, "Output");
    lv_obj_align(lbl_output_todo, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
}

/* ---------------------------------------------------------------------
 * Page 3 — 시스템 상태 및 진단
 * ------------------------------------------------------------------- */
static void build_page_diag(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv);

    banner_dtc = lv_obj_create(page);
    lv_obj_remove_style_all(banner_dtc);
    lv_obj_set_size(banner_dtc, 800 - 32, 56);
    lv_obj_set_style_radius(banner_dtc, 8, 0);
    lv_obj_set_style_bg_color(banner_dtc, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(banner_dtc, LV_OPA_20, 0);
    lv_obj_align(banner_dtc, LV_ALIGN_TOP_MID, 0, 16);

    lbl_dtc_code = lv_label_create(banner_dtc);
    lv_obj_add_style(lbl_dtc_code, &ui_style_label_mid, 0);
    /* dtc_code CAN ID(0x301)는 docs/design/can-signals.md 기준 실차 DBC 미대조
     * placeholder다 — UI는 raw 값만 그대로 보여준다(의미 해석 시도 안 함). */
    lv_label_set_text(lbl_dtc_code, "DTC: 정상");
    lv_obj_center(lbl_dtc_code);

    lv_obj_t *lbl_volt2 = lv_label_create(page);
    lv_obj_add_style(lbl_volt2, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_volt2, "Pack Voltage -- V");
    lv_obj_align(lbl_volt2, LV_ALIGN_LEFT_MID, 16, 0);
    /* 재사용을 위해 별도 static 대신 lbl_pack_volt를 그대로 갱신해도 되지만,
     * page 2와 페이지가 달라 별개 라벨로 둔다 -> ui_update()에서 이 포인터도 갱신 필요.
     * (아래 static 전역에 등록) */
    lbl_pack_volt2 = lbl_volt2;

    /* HARNESS-TODO: pack_temp 필드 없음 */
    lbl_pack_temp_todo = make_todo_label(page, "Pack Temp");
    lv_obj_align_to(lbl_pack_temp_todo, lbl_volt2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);
}

/* ---------------------------------------------------------------------
 * Page 4 — 기기 조작 및 연결성
 * ------------------------------------------------------------------- */
static lv_obj_t *make_status_dot(lv_obj_t *parent)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 16, 16);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    return dot;
}

static void build_page_connect(lv_obj_t *tv)
{
    lv_obj_t *page = make_page(tv);

    dot_ble = make_status_dot(page);
    lv_obj_align(dot_ble, LV_ALIGN_TOP_LEFT, 16, 24);

    lbl_ble_status = lv_label_create(page);
    lv_obj_add_style(lbl_ble_status, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_ble_status, "BLE 연결 안됨");
    lv_obj_align_to(lbl_ble_status, dot_ble, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

    dot_vehicle = make_status_dot(page);
    lv_obj_align_to(dot_vehicle, dot_ble, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 24);

    lv_obj_t *lbl_vehicle = lv_label_create(page);
    lv_obj_add_style(lbl_vehicle, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_vehicle, "차량 상태 정상");
    lv_obj_align_to(lbl_vehicle, dot_vehicle, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

    /* HARNESS-TODO: 시계/외기온도 — RTC/온도센서 소스 미확정, 소스 확정 전 stub 유지 */
    lv_obj_t *lbl_clock = make_todo_label(page, "시각");
    lv_obj_align(lbl_clock, LV_ALIGN_BOTTOM_LEFT, 16, -16);

    lv_obj_t *lbl_outtemp = make_todo_label(page, "외기");
    lv_obj_align(lbl_outtemp, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
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
        lv_obj_set_style_bg_color(dot, UI_COLOR_TEXT_SEC, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    }
    /* HARNESS-TODO: 페이지 전환 시 활성 dot 하이라이트 미구현
     * (tileview LV_EVENT_VALUE_CHANGED 콜백에서 처리 가능) */
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
}

void ui_update(void)
{
    VehicleData_t d;
    vehicle_data_get(&d);

    char buf[32];

    /* Page 1 */
    snprintf(buf, sizeof(buf), "%u", (unsigned)d.speed);
    lv_label_set_text(lbl_speed, buf);

    /* Page 2 */
    lv_arc_set_value(arc_soc, d.soc);
    lv_obj_set_style_arc_color(arc_soc,
        d.soc <= UI_SOC_LOW_PCT ? UI_COLOR_RED : UI_COLOR_CYAN,
        LV_PART_INDICATOR);
    snprintf(buf, sizeof(buf), "%u%%", (unsigned)d.soc);
    lv_label_set_text(lbl_soc_pct, buf);

    snprintf(buf, sizeof(buf), "%.1f V", d.pack_volt);
    lv_label_set_text(lbl_pack_volt, buf);

    /* Page 3 */
    if (d.dtc_code == 0) {
        lv_label_set_text(lbl_dtc_code, "DTC: 정상");
        lv_obj_set_style_bg_color(banner_dtc, UI_COLOR_TEXT_SEC, 0);
        lv_obj_set_style_bg_opa(banner_dtc, LV_OPA_20, 0);
    } else {
        snprintf(buf, sizeof(buf), "DTC: 0x%04X", (unsigned)d.dtc_code);
        lv_label_set_text(lbl_dtc_code, buf);
        lv_obj_set_style_bg_color(banner_dtc, UI_COLOR_RED, 0);
        lv_obj_set_style_bg_opa(banner_dtc, LV_OPA_50, 0);
    }

    snprintf(buf, sizeof(buf), "Pack Voltage %.1f V", d.pack_volt);
    lv_label_set_text(lbl_pack_volt2, buf);

    /* Page 4 */
    lv_obj_set_style_bg_color(dot_ble, d.ble_connected ? UI_COLOR_GREEN : UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_ble_status, d.ble_connected ? "BLE 연결됨" : "BLE 연결 안됨");

    lv_obj_set_style_bg_color(dot_vehicle, d.dtc_code == 0 ? UI_COLOR_GREEN : UI_COLOR_RED, 0);
}
