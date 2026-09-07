#include "ui_tile_charging.h"
#include "ui_style.h"
#include "ui_widget_cellbar.h"
#include "ui_widget_readout.h"
#include "ui_fonts_num.h"
#include "ui_fonts_label.h"
#include "vehicle_data.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>

/*
 * ui_tile_charging.c — Voltline Charging 탭 (phase 8, 계획 §5/§7-4).
 * 스펙: Voltline Cluster.dc.html isCharge 블록(약 136~154행).
 *
 * 플러그 감지 CAN 신호가 없어(vehicle_data.h 확인 완료) 강제전환/잠금 로직은
 * 없음 — 다른 타일과 동일하게 도트/탭존으로 도달 가능한 일반 5번째 타일(계획 §7-4).
 *
 * 데이터 갭(HARNESS-TODO, 지어내지 않음): 충전 중 여부 자체를 모르므로 스펙의
 * "Charging" go-배지는 안 띄운다(중립 캡션 "BATTERY STATUS"로 대체). "완충 시 range"
 * 예측/펌웨어 설치 안내는 대응 신호가 없어 "-"/"—" 고정. 완충예상시간/충전전력은
 * 2026-09-04 cluster.dbc InvMsg3로 신호가 생겨서(통상 범위 플레이스홀더,
 * docs/design/can-signals.md 참고) 배선했다.
 */

static lv_obj_t *s_lbl_soc_val;
static lv_obj_t *s_cellbar_soc;
static lv_obj_t *s_lbl_charge_note; /* "<time> to 100% · <power> kW · range now" 한 오브젝트 */
static lv_obj_t *s_lbl_range_val;
static lv_obj_t *s_lbl_packv_val;
static lv_obj_t *s_lbl_packtemp_val;

static lv_color_t soc_energy_color(int soc)
{
    if (soc > UI_ENERGY_THRESH_FULL_PCT) return UI_ENERGY_FULL;
    if (soc > UI_ENERGY_THRESH_MID_PCT) return UI_ENERGY_MID;
    if (soc > UI_ENERGY_THRESH_LOW_PCT) return UI_ENERGY_LOW;
    return UI_ENERGY_EMPTY;
}

lv_obj_t *ui_tile_charging_build(lv_obj_t *tile_parent)
{
    lv_obj_t *tile = lv_tileview_add_tile(tile_parent, 3, 0, LV_DIR_NONE);
    lv_obj_add_style(tile, &ui_style_bg, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(tile, 4, 0);

    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), 260, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(tile, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(tile, 10, 0);

    /* 좌측: SOC 큰 숫자 + 셀바 + 캡션 */
    lv_obj_t *left = lv_obj_create(tile);
    lv_obj_add_style(left, &ui_style_panel, 0);
    lv_obj_set_grid_cell(left, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(left, 10, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *badge = lv_obj_create(left);
    lv_obj_remove_style_all(badge);
    lv_obj_set_size(badge, LV_SIZE_CONTENT, 24);
    lv_obj_set_style_pad_hor(badge, 10, 0);
    lv_obj_set_style_radius(badge, UI_RADIUS_PILL, 0);
    lv_obj_set_style_bg_color(badge, UI_TEXT_TERTIARY, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_20, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *badge_lbl = lv_label_create(badge);
    lv_obj_set_style_text_font(badge_lbl, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(badge_lbl, 1, 0);
    lv_obj_set_style_text_color(badge_lbl, UI_TEXT_TERTIARY, 0);
    lv_label_set_text(badge_lbl, "BATTERY STATUS"); /* 충전 중 여부 확인 불가 — 중립 캡션 */
    lv_obj_center(badge_lbl);

    s_lbl_soc_val = ui_widget_readout_create(left, &ui_font_num_120, UI_TEXT_PRIMARY, "%");

    lv_obj_t *bar_row = lv_obj_create(left);
    lv_obj_remove_style_all(bar_row);
    lv_obj_set_size(bar_row, lv_pct(100), 22);
    lv_obj_set_flex_flow(bar_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(bar_row, LV_OBJ_FLAG_SCROLLABLE);
    s_cellbar_soc = ui_widget_cellbar_create(bar_row, UI_SOC_CELL_COUNT);
    lv_obj_set_flex_grow(s_cellbar_soc, 1);

    /* RANGE는 d.range_km 직결(스펙의 "완충 시 range" 예측이 아니라 "현재 range").
     * time_to_full/charge_power는 2026-09-04 InvMsg3 배선(통상 범위 플레이스홀더).
     *
     * 2026-09-04(2차): 원래 이 자리를 5개 라벨 오브젝트(값 2개 + 구분 텍스트 2개 +
     * range)로 쪼갰었는데, 그게 실기기에서 LVGL 내부 힙(LV_MEM_SIZE=40KB, 여유
     * 1024B뿐 — 2026-09-03 SRAM 위기 때와 동일한 빠듯한 예산)을 넘겨 lv_mem_buf_get()
     * 안에서 조용히 멈추는(LV_USE_ASSERT_MALLOC=y가 OOM을 assert로 처리, 그 핸들러가
     * while(1) 스핀 — task watchdog이 5초마다 반복 트리거되는 것으로 확인) 증상을
     * 재현시켰다. 새 오브젝트를 추가하지 않고 원래 있던 2개짜리 구조(정적 안내문 +
     * range 강조값)를 그대로 재사용해서, time_to_full/charge_power는 안내문 문자열
     * 안에 같이 포맷팅한다(강조색은 range만 유지, 값이 늘었다고 위젯을 늘리지 않는다). */
    lv_obj_t *note_row = lv_obj_create(left);
    lv_obj_remove_style_all(note_row);
    lv_obj_set_size(note_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(note_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(note_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(note_row, 6, 0);
    lv_obj_clear_flag(note_row, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_charge_note = lv_label_create(note_row);
    lv_obj_add_style(s_lbl_charge_note, &ui_style_text_secondary, 0);
    lv_obj_set_style_text_font(s_lbl_charge_note, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_lbl_charge_note, "\xE2\x80\x94 to 100% \xC2\xB7 \xE2\x80\x94 kW \xC2\xB7 range now");

    s_lbl_range_val = lv_label_create(note_row);
    lv_obj_add_style(s_lbl_range_val, &ui_style_text_primary, 0);
    lv_obj_set_style_text_font(s_lbl_range_val, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_lbl_range_val, "\xE2\x80\x94");

    /* 우측: PACK V / PACK 온도 + 펌웨어 안내 */
    lv_obj_t *right = lv_obj_create(tile);
    lv_obj_add_style(right, &ui_style_panel, 0);
    lv_obj_set_grid_cell(right, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cap1 = lv_label_create(right);
    lv_obj_add_style(cap1, &ui_style_text_secondary, 0);
    lv_obj_set_style_text_font(cap1, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(cap1, 1, 0);
    lv_label_set_text(cap1, "PACK");
    s_lbl_packv_val = ui_widget_readout_create(right, &ui_font_num_46, UI_TEXT_PRIMARY, "V");

    lv_obj_t *cap2 = lv_label_create(right);
    lv_obj_add_style(cap2, &ui_style_text_secondary, 0);
    lv_obj_set_style_text_font(cap2, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(cap2, 1, 0);
    lv_label_set_text(cap2, "CELL TEMP");
    s_lbl_packtemp_val = ui_widget_readout_create(right, &ui_font_num_34, UI_TEXT_PRIMARY, "\xC2\xB0" "C");

    /* HARNESS-TODO: 확인필요 — 펌웨어 설치/OTA 진행상태 신호 없음 */
    lv_obj_t *fw_lbl = lv_label_create(right);
    lv_obj_add_style(fw_lbl, &ui_style_text_tertiary, 0);
    lv_obj_set_style_text_font(fw_lbl, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(fw_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(fw_lbl, lv_pct(100));
    lv_label_set_text(fw_lbl, "\xE2\x80\x94"); /* 펌웨어 설치 안내 실데이터 없음 */

    return tile;
}

/* 2026-09-04: full_refresh=1(main/lvgl.c)에서는 lv_label_set_text/lv_obj_set_style_*
 * 호출 자체가 내용이 그대로여도 매번 invalidate를 부른다 — 값이 실제로 바뀔 때만
 * 호출하도록 가드(ui_widget_cellbar.c 상단 주석 참고, ui_tile_ride.c와 동일 패턴). */
void ui_tile_charging_update(void)
{
    VehicleData_t d;
    vehicle_data_get(&d);

    char buf[16];
    static unsigned soc_prev = UINT32_MAX;
    if ((unsigned)d.soc != soc_prev) {
        soc_prev = (unsigned)d.soc;
        snprintf(buf, sizeof(buf), "%u", (unsigned)d.soc);
        lv_label_set_text(s_lbl_soc_val, buf);
        lv_obj_set_style_text_color(s_lbl_soc_val, d.soc > UI_ENERGY_THRESH_MID_PCT ? UI_TEXT_PRIMARY
                                     : (d.soc > UI_BATTERY_LOW_PCT ? UI_SIGNAL_CAUTION : UI_SIGNAL_CRITICAL), 0);
        ui_widget_cellbar_set(s_cellbar_soc, (int)lroundf(d.soc / (100.0f / UI_SOC_CELL_COUNT)), soc_energy_color(d.soc));
    }

    static unsigned range_prev = UINT32_MAX;
    if ((unsigned)d.range_km != range_prev) {
        range_prev = (unsigned)d.range_km;
        snprintf(buf, sizeof(buf), "%u km", (unsigned)d.range_km);
        lv_label_set_text(s_lbl_range_val, buf);
    }

    /* time_to_full/charge_power를 별도 오브젝트로 안 쪼개고 안내문 하나에 같이
     * 포맷팅한다(위 build 함수 주석 참고 — LVGL 내부 힙 예산 때문에 위젯을 안 늘림). */
    static unsigned time_to_full_prev = UINT32_MAX;
    static int charge_power_dv_prev = INT32_MIN;
    unsigned time_to_full_now = (unsigned)d.time_to_full_min;
    int charge_power_dv = (int)lroundf(d.charge_power_kw * 10.0f);
    if (time_to_full_now != time_to_full_prev || charge_power_dv != charge_power_dv_prev) {
        time_to_full_prev = time_to_full_now;
        charge_power_dv_prev = charge_power_dv;
        char note_buf[48];
        snprintf(note_buf, sizeof(note_buf), "%u min to 100%% \xC2\xB7 %.1f kW \xC2\xB7 range now",
                 time_to_full_now, (double)d.charge_power_kw);
        lv_label_set_text(s_lbl_charge_note, note_buf);
    }

    static int packv_dv_prev = INT32_MIN;
    int packv_dv = (int)lroundf(d.pack_volt * 10.0f);
    if (packv_dv != packv_dv_prev) {
        packv_dv_prev = packv_dv;
        snprintf(buf, sizeof(buf), "%.1f", (double)d.pack_volt);
        lv_label_set_text(s_lbl_packv_val, buf);
    }

    static int packtemp_prev = INT32_MIN;
    int packtemp_now = (int)d.sys_temp_c;
    if (packtemp_now != packtemp_prev) {
        packtemp_prev = packtemp_now;
        snprintf(buf, sizeof(buf), "%d", packtemp_now);
        lv_label_set_text(s_lbl_packtemp_val, buf);
    }
}
