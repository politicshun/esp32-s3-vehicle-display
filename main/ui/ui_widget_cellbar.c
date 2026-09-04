#include "ui_widget_cellbar.h"
#include "ui_style.h"

/* 2026-09-04: full_refresh=1인 이 프로젝트에서는 lv_obj_set_style_xxx/lv_label_set_text
 * 호출 자체가 (내용이 안 바뀌어도) 매번 lv_obj_invalidate()를 부른다 — LVGL 소스
 * 확인 완료(managed_components/.../lv_label.c:90, lv_obj_style.c의 lv_obj_refresh_style).
 * 그래서 20ms마다 값이 그대로여도 화면 전체가 다시 그려져 handler_avg가 항상
 * 예산의 9배(180ms)에 머물렀던 것 — 매 위젯 갱신 호출부를 "값이 실제로 바뀔 때만"
 * 실행하도록 가드해야 한다(project_2026-09-02_voltline_5tile_ui_rewrite 메모리,
 * 2026-09-04 세션). 이 위젯은 SOC 셀바(Ride/Charging 2곳 재사용)라 콜사이트별
 * static 변수 대신 바 오브젝트의 user_data에 이전 상태를 들고 다닌다. */
typedef struct {
    int lit_count;
    lv_color_t lit_color;
} cellbar_state_t;

lv_obj_t *ui_widget_cellbar_create(lv_obj_t *parent, int cell_count)
{
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, lv_pct(100), 22);
    lv_obj_set_style_border_color(bar, UI_LINE_DEFAULT, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, 3, 0);
    lv_obj_set_style_pad_all(bar, 3, 0);
    lv_obj_set_style_pad_column(bar, 3, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < cell_count; i++) {
        lv_obj_t *cell = lv_obj_create(bar);
        lv_obj_remove_style_all(cell);
        lv_obj_set_flex_grow(cell, 1);
        lv_obj_set_height(cell, lv_pct(100));
        lv_obj_set_style_radius(cell, 1, 0);
        lv_obj_set_style_bg_color(cell, UI_TRACK_EMPTY, 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
    }

    cellbar_state_t *st = lv_mem_alloc(sizeof(cellbar_state_t));
    st->lit_count = -1; /* 첫 ui_widget_cellbar_set() 호출은 항상 실그리기 */
    st->lit_color = UI_TRACK_EMPTY;
    lv_obj_set_user_data(bar, st);

    return bar;
}

void ui_widget_cellbar_set(lv_obj_t *bar, int lit_count, lv_color_t lit_color)
{
    uint32_t count = lv_obj_get_child_cnt(bar);
    if (lit_count < 0) lit_count = 0;
    if (lit_count > (int)count) lit_count = (int)count;

    cellbar_state_t *st = (cellbar_state_t *)lv_obj_get_user_data(bar);
    if (st->lit_count == lit_count && st->lit_color.full == lit_color.full) return;
    st->lit_count = lit_count;
    st->lit_color = lit_color;

    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *cell = lv_obj_get_child(bar, (int32_t)i);
        lv_obj_set_style_bg_color(cell, (int)i < lit_count ? lit_color : UI_TRACK_EMPTY, 0);
    }
}
