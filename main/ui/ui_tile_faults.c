#include "ui_tile_faults.h"
#include "ui_style.h"
#include "ui_widget_readout.h"
#include "ui_fonts_num.h"
#include "ui_fonts_label.h"
#include "vehicle_data.h"
#include <stdio.h>
#include <stdint.h>

/*
 * ui_tile_faults.c — Voltline Faults 탭 (phase 8, 계획 §5/§7-6).
 * 스펙: Voltline Cluster.dc.html isDtc 블록(약 206~235행) + Cluster Dev
 *       Spec.dc.html §Faults/DTC.
 *
 * dtc_code는 단일 enum(카운트/비트마스크 아님, vehicle_data.h)이고 cluster.dbc에
 * VAL_ 매핑 테이블이 없다 — 그래서 스펙 데모의 "Motor over temperature" 같은
 * 구체적 제목/설명은 지어내지 않는다(CLAUDE.md 0번 원칙). 배너는 활성 여부와
 * raw hex 코드만 보여준다. AlertBanner는 스펙상 여러 개(다중 fault)지만 우리는
 * "최대 1개"만 취급하므로 배너도 최대 1개.
 *
 * 4칸 온도/전압 그리드: Motor/Controller/Cell delta는 대응 CAN 신호가 아예 없어
 * (계획 §2 데이터 갭) 영구 "-"(HARNESS-TODO, 절대 갱신 안 함). Pack만 sys_temp_c
 * 직결 — git history상 이 필드가 일관되게 "배터리/팩 온도"로 쓰였음을 재확인
 * (project 메모리 2026-09-02).
 */

static lv_obj_t *s_lbl_title;
static lv_obj_t *s_banner;
static lv_obj_t *s_lbl_banner_title;
static lv_obj_t *s_lbl_banner_code;

static lv_obj_t *s_lbl_pack_val;

/* card 오브젝트(그리드 배치용)를 반환하고 value 라벨은 *val_out에 담는다. */
static lv_obj_t *build_stat_card(lv_obj_t *parent, const char *caption, const char *unit,
                                  lv_obj_t **val_out)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_add_style(card, &ui_style_panel, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cap = lv_label_create(card);
    lv_obj_add_style(cap, &ui_style_text_secondary, 0);
    lv_obj_set_style_text_font(cap, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(cap, 1, 0);
    lv_label_set_text(cap, caption);

    /* 4칸 전부 num_46 재사용(phase 7에서 이미 생성됨) — 값이 없는 칸도 같은 폰트로
     * "-"를 고정 출력해 시각적 일관성을 유지한다. */
    *val_out = ui_widget_readout_create(card, &ui_font_num_46, UI_TEXT_PRIMARY, unit);
    return card;
}

lv_obj_t *ui_tile_faults_build(lv_obj_t *tile_parent)
{
    lv_obj_t *tile = lv_tileview_add_tile(tile_parent, 1, 0, LV_DIR_NONE);
    lv_obj_add_style(tile, &ui_style_bg, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(tile, 4, 0);
    lv_obj_set_style_pad_row(tile, 10, 0);

    /* 타이틀 행 */
    lv_obj_t *title_row = lv_obj_create(tile);
    lv_obj_remove_style_all(title_row);
    lv_obj_set_size(title_row, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_title = lv_label_create(title_row);
    lv_obj_set_style_text_font(s_lbl_title, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(s_lbl_title, 2, 0); /* 스펙 0.16em @13px */
    lv_obj_set_style_text_color(s_lbl_title, UI_TEXT_SECONDARY, 0);
    lv_label_set_text(s_lbl_title, "FAULTS - 0 ACTIVE");

    /* HARNESS-TODO: 확인필요 — VIN/FW 버전 저장 소스 없음(grep 결과 없음).
     * 스펙은 이 줄을 var(--font-mono)로 그린다. */
    lv_obj_t *lbl_meta = lv_label_create(title_row);
    lv_obj_set_style_text_font(lbl_meta, &ui_font_mono_12, 0);
    lv_obj_set_style_text_color(lbl_meta, UI_TEXT_TERTIARY, 0);
    lv_label_set_text(lbl_meta, "VIN \xE2\x80\x94" " \xC2\xB7 " "FW \xE2\x80\x94"); /* "VIN — · FW —" */

    /* DTC 배너 — 기본 숨김, dtc_code!=0일 때만 표시(update에서 토글) */
    s_banner = lv_obj_create(tile);
    lv_obj_remove_style_all(s_banner);
    lv_obj_set_size(s_banner, lv_pct(100), 58);
    lv_obj_set_style_bg_color(s_banner, UI_SIGNAL_CRITICAL, 0);
    lv_obj_set_style_bg_opa(s_banner, LV_OPA_20, 0);
    lv_obj_set_style_border_color(s_banner, UI_SIGNAL_CRITICAL, 0);
    lv_obj_set_style_border_width(s_banner, 1, 0);
    lv_obj_set_style_border_opa(s_banner, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_banner, UI_RADIUS_TILE, 0);
    lv_obj_set_style_pad_hor(s_banner, 14, 0);
    lv_obj_set_flex_flow(s_banner, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_banner, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_banner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_banner, LV_OBJ_FLAG_HIDDEN);

    s_lbl_banner_title = lv_label_create(s_banner);
    lv_obj_set_style_text_font(s_lbl_banner_title, &ui_font_label_13, 0);
    lv_obj_set_style_text_letter_space(s_lbl_banner_title, 1, 0);
    lv_obj_set_style_text_color(s_lbl_banner_title, UI_SIGNAL_CRITICAL, 0);
    lv_label_set_text(s_lbl_banner_title, "DTC ACTIVE");

    s_lbl_banner_code = lv_label_create(s_banner);
    lv_obj_set_style_text_font(s_lbl_banner_code, &ui_font_mono_12, 0);
    lv_obj_set_style_text_color(s_lbl_banner_code, UI_TEXT_PRIMARY, 0);
    lv_label_set_text(s_lbl_banner_code, "");

    /* 4칸 그리드: Motor / Controller / Pack / Cell delta */
    lv_obj_t *grid = lv_obj_create(tile);
    lv_obj_remove_style_all(grid);
    lv_obj_set_size(grid, lv_pct(100), 126);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
    lv_obj_set_style_pad_column(grid, 10, 0);

    lv_obj_t *c, *v;
    c = build_stat_card(grid, "MOTOR", "\xC2\xB0" "C", &v); /* °C — 소스 없음, 영구 "-" */
    lv_obj_set_grid_cell(c, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    c = build_stat_card(grid, "CONTROLLER", "\xC2\xB0" "C", &v); /* 소스 없음, 영구 "-" */
    lv_obj_set_grid_cell(c, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    c = build_stat_card(grid, "PACK", "\xC2\xB0" "C", &s_lbl_pack_val);
    lv_obj_set_grid_cell(c, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    c = build_stat_card(grid, "CELL DELTA", "V", &v); /* 소스 없음, 영구 "-" */
    lv_obj_set_grid_cell(c, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    return tile;
}

void ui_tile_faults_update(void)
{
    VehicleData_t d;
    vehicle_data_get(&d);

    static int dtc_prev = -1;
    int dtc_now = (d.dtc_code != 0) ? 1 : 0;
    if (dtc_now != dtc_prev) {
        dtc_prev = dtc_now;
        lv_label_set_text(s_lbl_title, dtc_now ? "FAULTS - 1 ACTIVE" : "FAULTS - 0 ACTIVE");
        lv_obj_set_style_text_color(s_lbl_title, dtc_now ? UI_SIGNAL_CRITICAL : UI_TEXT_SECONDARY, 0);
        if (dtc_now) {
            lv_obj_clear_flag(s_banner, LV_OBJ_FLAG_HIDDEN);
            char buf[16];
            snprintf(buf, sizeof(buf), "code 0x%02X", (unsigned)d.dtc_code);
            lv_label_set_text(s_lbl_banner_code, buf);
        } else {
            lv_obj_add_flag(s_banner, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 2026-09-04: full_refresh=1(main/lvgl.c)에서 lv_label_set_text는 값이 그대로여도
     * 매번 invalidate를 부른다 — 값이 실제로 바뀔 때만 호출(ui_widget_cellbar.c
     * 상단 주석 참고). */
    static int pack_temp_prev = INT32_MIN;
    int pack_temp_now = (int)d.sys_temp_c;
    if (pack_temp_now != pack_temp_prev) {
        pack_temp_prev = pack_temp_now;
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", pack_temp_now);
        lv_label_set_text(s_lbl_pack_val, buf);
    }
}
