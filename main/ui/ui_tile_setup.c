#include "ui_tile_setup.h"
#include "ui_style.h"
#include "ui_fonts_label.h"
#include "vehicle_data.h"
#include "cluster_settings.h"
#include <stdio.h>

/*
 * ui_tile_setup.c — Voltline Setup 탭 (phase 8, 계획 §5).
 * 스펙: Voltline Cluster.dc.html isSetup 블록(약 237~269행) — 2x2 카드
 *       (Display/Vehicle/Phone·BLE/System).
 *
 * 2026-09-04: 5개 값(밝기/회생레벨/자동상향등/자동주야간/단위)을 `cluster_settings.h`
 * 경유로 `ClusterSettings`(0x500, CLUSTER->INVERTER, docs/hardware/cluster.dbc,
 * 통상 범위 플레이스홀더)에 실어 CAN으로 내보내는 배선까지는 됐다. **그래도 여전히
 * 로컬 UI 데모다** — 밝기는 백라이트 PWM 드라이버가 없고(pin_config.h에 백라이트 핀
 * 자체가 없음), 설정값은 NVS에 저장 안 해서 재부팅하면 초기값으로 돌아가고, 단위
 * 전환도 Ride 탭에 아직 전파되지 않는다(그쪽은 km 하드코딩). 인버터가 이 CAN
 * 메시지를 받아서 뭘 하는지도 인버터측 구현 영역이라 여기선 모른다 — "화면 조작 ->
 * CAN으로 나감"까지만 확인됐고, "그게 실제 차량에 반영됨"은 아직 아니다.
 *
 * 유일하게 실데이터인 것: Phone(BLE) 카드의 연결 배지(d.ble_connected 직결).
 * "Rider's iPhone"/페어링코드 "418 902"(스펙 데모값)는 지어내지 않는다 —
 * main/ble.c는 상대(폰) GAP 이름을 저장하지 않고(project 메모리 확인됨),
 * Just Works 페어링이라 PIN 코드 개념 자체가 없다(사실 그대로 표기).
 */

static lv_obj_t *s_ble_badge_lbl;
static lv_obj_t *s_ble_badge_box;

static lv_obj_t *build_card(lv_obj_t *parent, const char *caption)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_add_style(card, &ui_style_panel, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cap = lv_label_create(card);
    lv_obj_add_style(cap, &ui_style_text_secondary, 0);
    lv_obj_set_style_text_font(cap, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(cap, 1, 0);
    lv_label_set_text(cap, caption);

    return card;
}

/* 일반 본문 라벨(체크박스/슬라이더 옆 설명 등) — 대문자/자간 캡션이 아니라 몽세라 그대로. */
static lv_obj_t *build_labeled_row(lv_obj_t *parent, const char *text)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(row);
    lv_obj_add_style(lbl, &ui_style_text_primary, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_label_set_text(lbl, text);

    return row;
}

/* 슬라이더 값 라벨을 "라벨 N%" 형태로 실시간 갱신 + cluster_settings에 반영해서
 * ClusterSettings(0x500)로 나가게 한다(여전히 로컬 UI 데모, 위 파일 상단 주석 참고). */
static void slider_pct_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *val_lbl = (lv_obj_t *)lv_event_get_user_data(e);
    int val = (int)lv_slider_get_value(slider);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", val);
    lv_label_set_text(val_lbl, buf);

    ClusterSettings_t cs;
    cluster_settings_get(&cs);
    cs.brightness_pct = (uint8_t)val;
    cluster_settings_set(&cs);
}

static void slider_level_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *val_lbl = (lv_obj_t *)lv_event_get_user_data(e);
    int val = (int)lv_slider_get_value(slider);
    char buf[16];
    snprintf(buf, sizeof(buf), "LEVEL %d", val);
    lv_label_set_text(val_lbl, buf);

    ClusterSettings_t cs;
    cluster_settings_get(&cs);
    cs.regen_level = (uint8_t)val;
    cluster_settings_set(&cs);
}

/* 단위 버튼매트릭스/스위치 3개는 라벨 갱신이 따로 없어 cluster_settings 반영만 한다. */
static void units_btnmatrix_cb(lv_event_t *e)
{
    lv_obj_t *btnm = lv_event_get_target(e);
    uint16_t checked = lv_btnmatrix_get_selected_btn(btnm);
    ClusterSettings_t cs;
    cluster_settings_get(&cs);
    cs.units_mph = (checked == 1); /* index 0=KM/H·KM, 1=MPH·MI(build_display_card 참고) */
    cluster_settings_set(&cs);
}

static void headlight_switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    ClusterSettings_t cs;
    cluster_settings_get(&cs);
    cs.auto_headlight = lv_obj_has_state(sw, LV_STATE_CHECKED);
    cluster_settings_set(&cs);
}

static void daynight_switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    ClusterSettings_t cs;
    cluster_settings_get(&cs);
    cs.auto_day_night = lv_obj_has_state(sw, LV_STATE_CHECKED);
    cluster_settings_set(&cs);
}

static lv_obj_t *build_display_card(lv_obj_t *parent)
{
    lv_obj_t *card = build_card(parent, "DISPLAY");

    static const char *unit_map[] = {"KM/H \xC2\xB7 KM", "MPH \xC2\xB7 MI", ""};
    lv_obj_t *units = lv_btnmatrix_create(card);
    lv_obj_set_size(units, lv_pct(100), 36);
    lv_btnmatrix_set_map(units, unit_map);
    lv_btnmatrix_set_one_checked(units, true);
    lv_btnmatrix_set_btn_ctrl(units, 0, LV_BTNMATRIX_CTRL_CHECKED);
    lv_obj_add_event_cb(units, units_btnmatrix_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *br_row = build_labeled_row(card, "Brightness");
    lv_obj_t *br_val = lv_label_create(br_row);
    lv_obj_add_style(br_val, &ui_style_text_secondary, 0);
    lv_obj_set_style_text_font(br_val, &lv_font_montserrat_14, 0);
    lv_label_set_text(br_val, "100%");

    lv_obj_t *br_slider = lv_slider_create(card);
    lv_obj_set_size(br_slider, lv_pct(100), 16);
    lv_slider_set_range(br_slider, 10, 100);
    lv_slider_set_value(br_slider, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(br_slider, UI_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(br_slider, UI_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(br_slider, slider_pct_cb, LV_EVENT_VALUE_CHANGED, br_val);
    return card;
}

static lv_obj_t *build_vehicle_card(lv_obj_t *parent)
{
    lv_obj_t *card = build_card(parent, "VEHICLE");

    lv_obj_t *hl_row = build_labeled_row(card, "Auto headlight");
    lv_obj_t *hl_sw = lv_switch_create(hl_row);
    lv_obj_set_style_bg_color(hl_sw, UI_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(hl_sw, headlight_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *rg_row = build_labeled_row(card, "Regen level");
    lv_obj_t *rg_val = lv_label_create(rg_row);
    lv_obj_add_style(rg_val, &ui_style_text_secondary, 0);
    lv_obj_set_style_text_font(rg_val, &lv_font_montserrat_14, 0);
    lv_label_set_text(rg_val, "LEVEL 2");

    lv_obj_t *rg_slider = lv_slider_create(card);
    lv_obj_set_size(rg_slider, lv_pct(100), 16);
    lv_slider_set_range(rg_slider, 0, 3);
    lv_slider_set_value(rg_slider, 2, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(rg_slider, UI_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(rg_slider, UI_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(rg_slider, slider_level_cb, LV_EVENT_VALUE_CHANGED, rg_val);
    return card;
}

static lv_obj_t *build_ble_card(lv_obj_t *parent)
{
    /* 캡션+배지를 한 행에 같이 배치해야 해서 build_card()를 쓰지 않고 직접 만든다. */
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_add_style(card, &ui_style_panel, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *header = build_labeled_row(card, "PHONE (BLE)");
    lv_obj_t *cap = lv_obj_get_child(header, 0);
    lv_obj_add_style(cap, &ui_style_text_secondary, 0);
    lv_obj_set_style_text_color(cap, UI_TEXT_SECONDARY, 0);
    /* 이 행만 다른 카드들의 캡션과 같은 대문자/자간 스타일로 재지정
     * (build_labeled_row 기본값은 일반 본문용) */
    lv_obj_set_style_text_font(cap, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(cap, 1, 0);

    s_ble_badge_box = lv_obj_create(header);
    lv_obj_remove_style_all(s_ble_badge_box);
    lv_obj_set_size(s_ble_badge_box, LV_SIZE_CONTENT, 24);
    lv_obj_set_style_pad_hor(s_ble_badge_box, 10, 0);
    lv_obj_set_style_radius(s_ble_badge_box, UI_RADIUS_PILL, 0);
    lv_obj_set_style_bg_color(s_ble_badge_box, UI_TEXT_TERTIARY, 0);
    lv_obj_set_style_bg_opa(s_ble_badge_box, LV_OPA_20, 0);
    lv_obj_clear_flag(s_ble_badge_box, LV_OBJ_FLAG_SCROLLABLE);
    s_ble_badge_lbl = lv_label_create(s_ble_badge_box);
    lv_obj_set_style_text_font(s_ble_badge_lbl, &ui_font_label_13, 0);
    lv_obj_set_style_text_color(s_ble_badge_lbl, UI_TEXT_TERTIARY, 0);
    lv_label_set_text(s_ble_badge_lbl, "\xE2\x80\x94");
    lv_obj_center(s_ble_badge_lbl);

    /* HARNESS-TODO: 확인필요 — main/ble.c는 상대(폰)의 GAP 기기명을 저장하지 않는다
     * (자기 자신의 광고명 "ESP32S3-Cluster"만 있음). 지어내지 않고 "—" 고정. */
    lv_obj_t *dev_lbl = lv_label_create(card);
    lv_obj_add_style(dev_lbl, &ui_style_text_primary, 0);
    lv_obj_set_style_text_font(dev_lbl, &lv_font_montserrat_14, 0);
    lv_label_set_text(dev_lbl, "device \xE2\x80\x94");

    lv_obj_t *pair_lbl = lv_label_create(card);
    lv_obj_add_style(pair_lbl, &ui_style_text_tertiary, 0);
    lv_obj_set_style_text_font(pair_lbl, &lv_font_montserrat_14, 0);
    lv_label_set_text(pair_lbl, "Just Works pairing (no PIN)"); /* 사실 그대로 — 지어낸 코드 아님 */
    return card;
}

static lv_obj_t *build_system_card(lv_obj_t *parent)
{
    lv_obj_t *card = build_card(parent, "SYSTEM");

    lv_obj_t *dn_row = build_labeled_row(card, "Auto day/night");
    lv_obj_t *dn_sw = lv_switch_create(dn_row);
    lv_obj_set_style_bg_color(dn_sw, UI_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(dn_sw, daynight_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* HARNESS-TODO: 확인필요 — FW 버전/시리얼 저장 소스 없음(grep 결과 없음).
     * "ESP32-S3"만 실제 타깃 칩(빌드 타깃) 그대로. */
    lv_obj_t *sys_lbl = lv_label_create(card);
    lv_obj_add_style(sys_lbl, &ui_style_text_tertiary, 0);
    lv_obj_set_style_text_font(sys_lbl, &ui_font_mono_12, 0); /* 스펙: var(--font-mono) */
    lv_label_set_text(sys_lbl, "FW \xE2\x80\x94" " \xC2\xB7 " "S/N \xE2\x80\x94" " \xC2\xB7 ESP32-S3");
    return card;
}

lv_obj_t *ui_tile_setup_build(lv_obj_t *tile_parent)
{
    lv_obj_t *tile = lv_tileview_add_tile(tile_parent, 2, 0, LV_DIR_NONE);
    lv_obj_add_style(tile, &ui_style_bg, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tile, 4, 0);

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(tile, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(tile, 10, 0);
    lv_obj_set_style_pad_row(tile, 10, 0);

    lv_obj_t *card;
    card = build_display_card(tile);
    lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    card = build_vehicle_card(tile);
    lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    card = build_ble_card(tile);
    lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    card = build_system_card(tile);
    lv_obj_set_grid_cell(card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    return tile;
}

void ui_tile_setup_update(void)
{
    VehicleData_t d;
    vehicle_data_get(&d);

    static int ble_prev = -1;
    int ble_now = d.ble_connected ? 1 : 0;
    if (ble_now != ble_prev) {
        ble_prev = ble_now;
        lv_color_t tone = d.ble_connected ? UI_SIGNAL_GO : UI_TEXT_TERTIARY;
        lv_obj_set_style_bg_color(s_ble_badge_box, tone, 0);
        lv_obj_set_style_text_color(s_ble_badge_lbl, tone, 0);
        lv_label_set_text(s_ble_badge_lbl, d.ble_connected ? "CONNECTED" : "NOT CONNECTED");
    }
}
