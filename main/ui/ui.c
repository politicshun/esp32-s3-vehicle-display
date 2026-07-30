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

/* 속도 아크 게이지 표시 범위 — 실차 최고속도 스펙이 아니라 UI 표시 스케일 값.
 * 클러스터 UI에서 흔히 쓰는 라운드 값(200km/h)으로 설정, 실차 스펙 확정되면 조정 가능. */
#define SPEED_GAUGE_MAX_KMH 200

static lv_obj_t *tileview;
static lv_obj_t *pages[4];
static lv_obj_t *page_dots[4];

/* Page 1 — 주행 필수 정보 */
static lv_obj_t *arc_speed;
static lv_obj_t *lbl_speed;
static lv_obj_t *lbl_gear_todo;   /* HARNESS-TODO: gear 필드 없음 — 배지 모양만 레퍼런스와 맞춤 */
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
static lv_obj_t *make_page(lv_obj_t *tv, uint8_t col_id)
{
    lv_obj_t *page = lv_tileview_add_tile(tv, col_id, 0, LV_DIR_HOR);
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
    lv_obj_t *page = make_page(tv, 0);
    pages[0] = page;

    /* 레퍼런스 디자인의 원형 게이지 스피도미터 — speed는 live 데이터, 아크 자체는 표시 전용 */
    arc_speed = lv_arc_create(page);
    lv_obj_set_size(arc_speed, 300, 300);
    lv_arc_set_rotation(arc_speed, 135);
    lv_arc_set_bg_angles(arc_speed, 0, 270);
    lv_arc_set_range(arc_speed, 0, SPEED_GAUGE_MAX_KMH);
    lv_arc_set_value(arc_speed, 0);
    lv_obj_remove_style(arc_speed, NULL, LV_PART_KNOB); /* 표시 전용, 조작 불가 */
    lv_obj_clear_flag(arc_speed, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(arc_speed, UI_COLOR_CYAN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_speed, 14, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_speed, 14, LV_PART_MAIN);
    lv_obj_align(arc_speed, LV_ALIGN_CENTER, 0, 0);

    lbl_speed = lv_label_create(page);
    lv_obj_add_style(lbl_speed, &ui_style_label_big, 0);
    lv_label_set_text(lbl_speed, "0");
    lv_obj_align_to(lbl_speed, arc_speed, LV_ALIGN_CENTER, 0, -12);

    lv_obj_t *unit = lv_label_create(page);
    lv_obj_add_style(unit, &ui_style_label_small, 0);
    lv_label_set_text(unit, "km/h");
    lv_obj_align_to(unit, lbl_speed, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    /* HARNESS-TODO: gear(N/D) 필드가 VehicleData_t에 없어 값은 지어내지 않고,
     * 레퍼런스 디자인의 원형 배지 모양만 미리 잡아둔다 (필드 추가되면 텍스트만 교체). */
    lv_obj_t *gear_badge = lv_obj_create(page);
    lv_obj_remove_style_all(gear_badge);
    lv_obj_set_size(gear_badge, 56, 56);
    lv_obj_set_style_radius(gear_badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(gear_badge, 2, 0);
    lv_obj_set_style_border_color(gear_badge, UI_COLOR_TEXT_SEC, 0);
    lv_obj_set_style_bg_opa(gear_badge, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(gear_badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(gear_badge, LV_ALIGN_TOP_LEFT, 16, 16);

    lbl_gear_todo = lv_label_create(gear_badge);
    lv_obj_add_style(lbl_gear_todo, &ui_style_label_mid, 0);
    lv_obj_set_style_text_color(lbl_gear_todo, UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_gear_todo, "--");
    lv_obj_center(lbl_gear_todo);

    /* "핵심 표시 항목" 표의 "주행 모드(Drive Mode)" — 배지 아래 캡션으로 항목명만 표시 */
    lv_obj_t *lbl_gear_caption = lv_label_create(page);
    lv_obj_add_style(lbl_gear_caption, &ui_style_label_small, 0);
    lv_label_set_text(lbl_gear_caption, "MODE");
    lv_obj_align_to(lbl_gear_caption, gear_badge, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

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
    lv_obj_t *page = make_page(tv, 1);
    pages[1] = page;

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

    /* "핵심 표시 항목" 표의 "실시간 출력 / 회생제동" 항목 */
    lbl_output_todo = make_todo_label(page, "Power/Regen");
    lv_obj_align(lbl_output_todo, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
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
    lv_obj_set_size(banner_dtc, 800 - 32, 56);
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

    lv_obj_t *lbl_volt2 = lv_label_create(page);
    lv_obj_add_style(lbl_volt2, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_volt2, "Pack Voltage -- V");
    lv_obj_align(lbl_volt2, LV_ALIGN_LEFT_MID, 16, 0);
    /* 재사용을 위해 별도 static 대신 lbl_pack_volt를 그대로 갱신해도 되지만,
     * page 2와 페이지가 달라 별개 라벨로 둔다 -> ui_update()에서 이 포인터도 갱신 필요.
     * (아래 static 전역에 등록) */
    lbl_pack_volt2 = lbl_volt2;

    /* HARNESS-TODO: 온도 센서 필드 없음 — "핵심 표시 항목" 표의 "시스템 온도 경고" 항목 */
    lbl_pack_temp_todo = make_todo_label(page, "Sys Temp");
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
    lv_obj_t *page = make_page(tv, 3);
    pages[3] = page;

    dot_ble = make_status_dot(page);
    lv_obj_align(dot_ble, LV_ALIGN_TOP_LEFT, 16, 24);

    lbl_ble_status = lv_label_create(page);
    lv_obj_add_style(lbl_ble_status, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_ble_status, "BLE Disconnected");
    lv_obj_align_to(lbl_ble_status, dot_ble, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

    dot_vehicle = make_status_dot(page);
    lv_obj_align_to(dot_vehicle, dot_ble, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 24);

    lv_obj_t *lbl_vehicle = lv_label_create(page);
    lv_obj_add_style(lbl_vehicle, &ui_style_label_mid, 0);
    lv_label_set_text(lbl_vehicle, "Vehicle Status: OK");
    lv_obj_align_to(lbl_vehicle, dot_vehicle, LV_ALIGN_OUT_RIGHT_MID, 12, 0);

    /* HARNESS-TODO: 시계/외기온도 — RTC/온도센서 소스 미확정, 소스 확정 전 stub 유지 */
    lv_obj_t *lbl_clock = make_todo_label(page, "Time");
    lv_obj_align(lbl_clock, LV_ALIGN_BOTTOM_LEFT, 16, -16);

    lv_obj_t *lbl_outtemp = make_todo_label(page, "Out Temp");
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

/* speed/soc 아크+숫자를 한 애니메이션으로 같이 움직여서(exec_cb 안에서 둘 다 갱신)
 * 실차 클러스터처럼 값이 순간 이동하지 않고 부드럽게 스윕되도록 한다. */
#define UI_GAUGE_ANIM_TIME_MS 250

static void anim_speed_exec_cb(void *var, int32_t v)
{
    (void)var;
    lv_arc_set_value(arc_speed, (int16_t)v);
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", (unsigned)v);
    lv_label_set_text(lbl_speed, buf);
}

static void animate_speed_to(int32_t target)
{
    static int32_t last_target = -1;
    if (target == last_target) return;
    last_target = target;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc_speed);
    lv_anim_set_exec_cb(&a, anim_speed_exec_cb);
    lv_anim_set_values(&a, lv_arc_get_value(arc_speed), target);
    lv_anim_set_time(&a, UI_GAUGE_ANIM_TIME_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void anim_soc_exec_cb(void *var, int32_t v)
{
    (void)var;
    lv_arc_set_value(arc_soc, (int16_t)v);
    lv_obj_set_style_arc_color(arc_soc,
        v <= UI_SOC_LOW_PCT ? UI_COLOR_RED : UI_COLOR_CYAN,
        LV_PART_INDICATOR);
    char buf[16];
    snprintf(buf, sizeof(buf), "%u%%", (unsigned)v);
    lv_label_set_text(lbl_soc_pct, buf);
}

static void animate_soc_to(int32_t target)
{
    static int32_t last_target = -1;
    if (target == last_target) return;
    last_target = target;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc_soc);
    lv_anim_set_exec_cb(&a, anim_soc_exec_cb);
    lv_anim_set_values(&a, lv_arc_get_value(arc_soc), target);
    lv_anim_set_time(&a, UI_GAUGE_ANIM_TIME_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void ui_update(void)
{
    VehicleData_t d;
    vehicle_data_get(&d);

    char buf[32];

    /* Page 1 */
    animate_speed_to(d.speed);

    /* Page 2 */
    animate_soc_to(d.soc);

    snprintf(buf, sizeof(buf), "%.1f V", d.pack_volt);
    lv_label_set_text(lbl_pack_volt, buf);

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

    snprintf(buf, sizeof(buf), "Pack Voltage %.1f V", d.pack_volt);
    lv_label_set_text(lbl_pack_volt2, buf);

    /* Page 4 */
    lv_obj_set_style_bg_color(dot_ble, d.ble_connected ? UI_COLOR_GREEN : UI_COLOR_TEXT_SEC, 0);
    lv_label_set_text(lbl_ble_status, d.ble_connected ? "BLE Connected" : "BLE Disconnected");

    lv_obj_set_style_bg_color(dot_vehicle, d.dtc_code == 0 ? UI_COLOR_GREEN : UI_COLOR_RED, 0);
}
