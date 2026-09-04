#include "ui_chrome.h"
#include "ui_style.h"
#include "ui_icons.h"
#include "vehicle_data.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>

/* 텔테일 5개는 스펙 순서 고정: 턴L, 상향등, 브레이크/ABS, EV경고(MIL), 턴R.
 * 오프 상태 = 18%(약 46/255) opa 고정, 절대 hidden 하지 않는다(스펙 §Iconography/§5).
 * 실데이터 배선(턴/상향등/브레이크는 CAN 신호 자체가 없어 영구 오프)은 phase 5. */
#define TELLTALE_COUNT 5
#define UI_OPA_OFF_18PCT 46

typedef struct {
    const lv_img_dsc_t *icon;
    lv_color_t tint;
} telltale_def_t;

static lv_obj_t *s_lbl_clock;
static lv_obj_t *s_lbl_ambient;
static lv_obj_t *s_img_ble;
static lv_obj_t *s_telltales[TELLTALE_COUNT];

static lv_obj_t *s_lbl_pack_volt;
static lv_obj_t *s_lbl_motor_temp;
static lv_obj_t *s_lbl_ctrl_temp;
static lv_obj_t *s_center_dtc_pill;
static lv_obj_t *s_dots[UI_CHROME_TILE_COUNT];
static lv_obj_t *s_lbl_status;

static lv_obj_t *make_divider(lv_obj_t *parent, int h)
{
    lv_obj_t *d = lv_obj_create(parent);
    lv_obj_remove_style_all(d);
    lv_obj_set_size(d, 1, h);
    lv_obj_set_style_bg_color(d, UI_HAIRLINE, 0);
    lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
    lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);
    return d;
}

lv_obj_t *ui_chrome_build_header(lv_obj_t *parent)
{
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_remove_style_all(header);
    lv_obj_add_style(header, &ui_style_bg, 0);
    lv_obj_set_size(header, lv_pct(100), 44);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header, UI_HAIRLINE, 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(header, 20, 0);
    lv_obj_set_style_pad_hor(header, 6, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    /* HARNESS-TODO: 확인필요 — main/init.c에 RTC/SNTP 시간 소스가 없음(2026-09-02 확인,
     * grep 결과 없음). 실시계 소스가 생기기 전까지 상시 "—" 표기(스펙 §6 stale 규칙 재사용,
     * 지어낸 시각을 보여주지 않는다). */
    s_lbl_clock = lv_label_create(header);
    lv_obj_add_style(s_lbl_clock, &ui_style_text_primary, 0);
    lv_obj_set_style_text_font(s_lbl_clock, &lv_font_montserrat_24, 0);
    lv_label_set_text(s_lbl_clock, "\xE2\x80\x94");

    make_divider(header, 20);

    /* HARNESS-TODO: 확인필요 — 외기온 CAN 신호 없음(vehicle_data.h에 대응 필드 없음) */
    s_lbl_ambient = lv_label_create(header);
    lv_obj_add_style(s_lbl_ambient, &ui_style_text_secondary, 0);
    lv_obj_set_style_text_font(s_lbl_ambient, &lv_font_montserrat_24, 0);
    lv_label_set_text(s_lbl_ambient, "\xE2\x80\x94 \xC2\xB0" "C"); /* "— °C" */

    make_divider(header, 20);

    lv_obj_t *ble_box = lv_obj_create(header);
    lv_obj_remove_style_all(ble_box);
    lv_obj_set_size(ble_box, 32, 32);
    lv_obj_clear_flag(ble_box, LV_OBJ_FLAG_SCROLLABLE);
    s_img_ble = lv_img_create(ble_box);
    lv_obj_center(s_img_ble);
    ui_icon_set(s_img_ble, &ui_icon_bluetooth_22, UI_TEXT_TERTIARY, LV_OPA_COVER);

    /* flex-spacer: flex-grow=1 짜리 빈 오브젝트로 좌측 클러스터와 텔테일 그룹을 벌린다 */
    lv_obj_t *spacer = lv_obj_create(header);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_size(spacer, 1, 1);
    lv_obj_clear_flag(spacer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *telltale_group = lv_obj_create(header);
    lv_obj_remove_style_all(telltale_group);
    lv_obj_set_size(telltale_group, LV_SIZE_CONTENT, 34);
    lv_obj_set_flex_flow(telltale_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(telltale_group, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(telltale_group, 26, 0);
    lv_obj_set_style_pad_right(telltale_group, 24, 0);
    lv_obj_clear_flag(telltale_group, LV_OBJ_FLAG_SCROLLABLE);

    const telltale_def_t telltales[TELLTALE_COUNT] = {
        {&ui_icon_turn_l_26, UI_SIGNAL_GO},
        {&ui_icon_highbeam_26, UI_SIGNAL_BEAM},
        {&ui_icon_brake_26, UI_SIGNAL_CAUTION},
        {&ui_icon_ev_warning_26, UI_SIGNAL_CRITICAL},
        {&ui_icon_turn_r_26, UI_SIGNAL_GO},
    };

    for (int i = 0; i < TELLTALE_COUNT; i++) {
        lv_obj_t *box = lv_obj_create(telltale_group);
        lv_obj_remove_style_all(box);
        lv_obj_set_size(box, 34, 34);
        lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *glyph = lv_img_create(box);
        lv_obj_center(glyph);
        ui_icon_set(glyph, telltales[i].icon, telltales[i].tint, UI_OPA_OFF_18PCT);
        s_telltales[i] = glyph;
    }

    return header;
}

static lv_obj_t *make_footer_stat(lv_obj_t *parent, const lv_img_dsc_t *icon,
                                   const char *label_text, lv_obj_t **value_lbl_out)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 6, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon_obj = lv_img_create(row);
    ui_icon_set(icon_obj, icon, UI_TEXT_SECONDARY, LV_OPA_COVER);

    lv_obj_t *stack = lv_obj_create(row);
    lv_obj_remove_style_all(stack);
    lv_obj_set_size(stack, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(stack, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(stack, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(stack, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(stack);
    lv_obj_add_style(lbl, &ui_style_text_tertiary, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl, label_text);

    lv_obj_t *val = lv_label_create(stack);
    lv_obj_add_style(val, &ui_style_text_primary, 0);
    lv_obj_set_style_text_font(val, &lv_font_montserrat_14, 0);
    lv_label_set_text(val, "\xE2\x80\x94"); /* em dash "—" — stale/no-signal 공통 표기 (스펙 §6) */
    if (value_lbl_out) *value_lbl_out = val;

    return row;
}

static void dot_click_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    /* dot 자체는 user_data를 인덱스로 쓰고 있으므로, 콜백은 도트의 부모(dots row)에
     * 저장해두고 여기서 꺼내 쓴다. */
    lv_obj_t *dots_row = lv_obj_get_parent(lv_event_get_target(e));
    ui_chrome_goto_tile_cb_t goto_cb = (ui_chrome_goto_tile_cb_t)lv_obj_get_user_data(dots_row);
    if (goto_cb) goto_cb(idx);
}

lv_obj_t *ui_chrome_build_footer(lv_obj_t *parent, ui_chrome_goto_tile_cb_t goto_cb)
{
    lv_obj_t *footer = lv_obj_create(parent);
    lv_obj_remove_style_all(footer);
    lv_obj_add_style(footer, &ui_style_bg, 0);
    lv_obj_set_size(footer, lv_pct(100), 44);
    lv_obj_set_style_border_side(footer, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(footer, UI_HAIRLINE, 0);
    lv_obj_set_style_border_width(footer, 1, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    static lv_coord_t col_dsc[] = {LV_GRID_CONTENT, LV_GRID_FR(1), LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(footer, col_dsc, row_dsc);
    lv_obj_set_style_pad_hor(footer, 6, 0);
    lv_obj_set_style_pad_column(footer, 14, 0);

    /* 좌측: 팩전압 | 구분선 | 모터온도 | 구분선 | 컨트롤러온도 */
    lv_obj_t *left = lv_obj_create(footer);
    lv_obj_remove_style_all(left);
    lv_obj_set_grid_cell(left, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left, 14, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    make_footer_stat(left, &ui_icon_battery_22, "PACK V", &s_lbl_pack_volt);
    make_divider(left, 24);
    /* HARNESS-TODO: 확인필요 — 모터/컨트롤러 개별 온도 CAN 신호 없음. vehicle_data.h의
     * sys_temp_c는 과거 UI(git history)에서 일관되게 "배터리 온도"로 써왔던 값이라
     * 여기(모터/컨트롤러 라벨)엔 넣지 않는다 — 잘못된 라벨에 값을 넣는 게 안 넣는 것보다
     * 더 나쁜 오도다. sys_temp_c는 Faults 탭의 "팩온도" 칸에서만 쓴다(phase 8). */
    make_footer_stat(left, &ui_icon_temp_22, "MOTOR", &s_lbl_motor_temp);
    make_divider(left, 24);
    make_footer_stat(left, &ui_icon_controller_22, "CTRL", &s_lbl_ctrl_temp);

    /* 중앙: 조건부 DTC칩 — 기본 숨김, dtc_code!=0일 때만 표시 (phase 5에서 배선) */
    s_center_dtc_pill = lv_obj_create(footer);
    lv_obj_remove_style_all(s_center_dtc_pill);
    lv_obj_set_grid_cell(s_center_dtc_pill, LV_GRID_ALIGN_CENTER, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_size(s_center_dtc_pill, LV_SIZE_CONTENT, 28);
    lv_obj_set_style_bg_color(s_center_dtc_pill, UI_SIGNAL_CRITICAL, 0);
    lv_obj_set_style_bg_opa(s_center_dtc_pill, LV_OPA_20, 0);
    lv_obj_set_style_radius(s_center_dtc_pill, UI_RADIUS_PILL, 0);
    lv_obj_set_style_pad_hor(s_center_dtc_pill, 12, 0);
    lv_obj_clear_flag(s_center_dtc_pill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_center_dtc_pill, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *dtc_lbl = lv_label_create(s_center_dtc_pill);
    lv_obj_set_style_text_color(dtc_lbl, UI_SIGNAL_CRITICAL, 0);
    lv_obj_set_style_text_font(dtc_lbl, &lv_font_montserrat_14, 0);
    lv_label_set_text(dtc_lbl, "DTC 1");
    lv_obj_center(dtc_lbl);

    /* 우측: 페이지 도트 | 구분선 | 상태단어 */
    lv_obj_t *right = lv_obj_create(footer);
    lv_obj_remove_style_all(right);
    lv_obj_set_grid_cell(right, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_size(right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right, 14, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *dots_row = lv_obj_create(right);
    lv_obj_remove_style_all(dots_row);
    lv_obj_set_size(dots_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(dots_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dots_row, 8, 0);
    lv_obj_clear_flag(dots_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(dots_row, (void *)goto_cb);

    for (int i = 0; i < UI_CHROME_TILE_COUNT; i++) {
        lv_obj_t *dot = lv_obj_create(dots_row);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, i == 0 ? UI_ACCENT : UI_LINE_DEFAULT, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(dot, dot_click_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s_dots[i] = dot;
    }

    make_divider(right, 24);

    s_lbl_status = lv_label_create(right);
    lv_obj_set_style_text_color(s_lbl_status, UI_SIGNAL_GO, 0);
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_lbl_status, "READY");

    return footer;
}

void ui_chrome_set_active_tile(int index)
{
    for (int i = 0; i < UI_CHROME_TILE_COUNT; i++) {
        if (!s_dots[i]) continue;
        lv_obj_set_style_bg_color(s_dots[i], i == index ? UI_ACCENT : UI_LINE_DEFAULT, 0);
    }
}

void ui_chrome_update(void)
{
    VehicleData_t d;
    vehicle_data_get(&d);

    /* BLE 램프: 값이 바뀔 때만 재도색 (매 프레임 무조건 set_style 하는 예전 버그 재발 방지,
     * main/ui/ui.c git history 참고) */
    static int ble_prev = -1;
    int ble_now = d.ble_connected ? 1 : 0;
    if (ble_now != ble_prev) {
        ble_prev = ble_now;
        lv_obj_set_style_img_recolor(s_img_ble, d.ble_connected ? UI_ACCENT : UI_TEXT_TERTIARY, 0);
    }

    /* 팩 전압 — 실데이터 직결. 0.1V 단위로 반올림한 정수로 비교해서 표시값이
     * 실제로 바뀔 때만 lv_label_set_text를 부른다(2026-09-04, full_refresh=1
     * 렌더 지연 조사 — ui_widget_cellbar.c 상단 주석 참고). */
    static int pack_volt_dv_prev = INT32_MIN;
    int pack_volt_dv = (int)lroundf(d.pack_volt * 10.0f);
    if (pack_volt_dv != pack_volt_dv_prev) {
        pack_volt_dv_prev = pack_volt_dv;
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f V", (double)d.pack_volt);
        lv_label_set_text(s_lbl_pack_volt, buf);
    }

    /* dtc_code는 단일 enum(카운트/비트마스크 아님, vehicle_data.h 주석 참고) — 계획 §2/§7-6에
     * 따라 "최대 1개 fault"로만 취급. DTC 배지(pill) 자체는 dtc_code 유무만 반영한다. */
    static int dtc_prev = -1;
    int dtc_now = (d.dtc_code != 0) ? 1 : 0;
    if (dtc_now != dtc_prev) {
        dtc_prev = dtc_now;
        if (dtc_now) {
            lv_obj_clear_flag(s_center_dtc_pill, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_center_dtc_pill, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 상태단어: CAN 링크 신선도가 DTC보다 우선한다. 링크가 끊기면 화면의 모든 값(dtc_code
     * 포함)이 마지막 수신값에서 멈춰 있으므로 "READY"조차 신뢰할 수 없다 — 그 상태에서
     * 초록불을 띄우면 오히려 위험하다(이전 단일페이지 ui.c, git history 5c0731e 이전,
     * 동일 판단 재사용). 값이 바뀔 때만 갱신. */
    static int status_prev = -1;
    int status_now = d.can_rx_stale ? 2 : dtc_now;
    if (status_now != status_prev) {
        status_prev = status_now;
        if (status_now == 2) {
            lv_obj_set_style_text_color(s_lbl_status, UI_SIGNAL_CRITICAL, 0);
            lv_label_set_text(s_lbl_status, "NO SIGNAL");
        } else if (status_now == 1) {
            lv_obj_set_style_text_color(s_lbl_status, UI_SIGNAL_CAUTION, 0);
            lv_label_set_text(s_lbl_status, "LIMITED");
        } else {
            lv_obj_set_style_text_color(s_lbl_status, UI_SIGNAL_GO, 0);
            lv_label_set_text(s_lbl_status, "READY");
        }
    }

    /* 시계/외기온/모터온도/컨트롤러온도: HARNESS-TODO — 실소스 없어 상시 "—" 고정,
     * ui_chrome_build_header/footer()에서 이미 설정해둔 값 그대로 둔다(여기서 매 프레임
     * 다시 쓸 이유 없음). */
}
