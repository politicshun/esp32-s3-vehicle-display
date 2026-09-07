#include "ui_widget_cellbar.h"
#include "ui_style.h"
#include <stdbool.h>

/* 2026-09-04: full_refresh=1인 이 프로젝트에서는 lv_obj_set_style_xxx/lv_label_set_text
 * 호출 자체가 (내용이 안 바뀌어도) 매번 lv_obj_invalidate()를 부른다 — LVGL 소스
 * 확인 완료(managed_components/.../lv_label.c:90, lv_obj_style.c의 lv_obj_refresh_style).
 * 그래서 20ms마다 값이 그대로여도 화면 전체가 다시 그려져 handler_avg가 항상
 * 예산의 9배(180ms)에 머물렀던 것 — 매 위젯 갱신 호출부를 "값이 실제로 바뀔 때만"
 * 실행하도록 가드해야 한다(project_2026-09-02_voltline_5tile_ui_rewrite 메모리,
 * 2026-09-04 세션). 이 위젯은 SOC 셀바(Ride/Charging 2곳 재사용)라 콜사이트별
 * static 변수 대신 바 오브젝트의 user_data에 이전 상태를 들고 다닌다. */
/* 2026-09-04(2차, KVASER 실동 부하 실측 후): 위 가드만으로는 부족했다 — 값이 바뀌면
 * 이 for문이 여전히 셀 전부(최대 count개)를 무조건 다시 set_style해서, 매 CAN 틱마다
 * lv_obj_invalidate()가 셀 개수만큼 쌓인다. LVGL의 invalidate 버퍼는 32칸
 * 고정(LV_INV_BUF_SIZE, managed_components/.../lv_hal_disp.h)이라 Ride 탭 플로우
 * 스트립(20셀)+이 셀바(최대 5칸)+라벨들이 한 틱에 같이 바뀌면 32칸을 넘겨 LVGL이
 * "자리 없으면 화면 전체를 넣는다"(_lv_inv_area(), lv_refr.c)로 조용히 풀리프레시급
 * 손실로 빠진다 — full_refresh=0으로 바꿨는데도 handler_avg가 안 줄어든 근본 원인.
 * 그래서 셀 단위로도 "실제로 켜짐/꺼짐이 바뀐 셀만" invalidate하도록 한 단계 더
 * 좁힌다. */
typedef struct {
    int lit_count;
    lv_color_t lit_color;
    int cell_count;
    bool *cell_lit; /* 각 셀이 마지막으로 실제 그려진 상태에서 켜져 있었는지 */
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
    st->cell_count = cell_count;
    st->cell_lit = lv_mem_alloc(sizeof(bool) * (size_t)cell_count);
    for (int i = 0; i < cell_count; i++) st->cell_lit[i] = false; /* build 시 전부 UI_TRACK_EMPTY로 그려짐 */
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
    bool color_changed = st->lit_color.full != lit_color.full;
    st->lit_count = lit_count;
    st->lit_color = lit_color;

    for (uint32_t i = 0; i < count; i++) {
        bool want_lit = (int)i < lit_count;
        /* on/off 전환이 없고, 계속 켜진 채로 색만 그대로면 건드릴 이유가 없다.
         * 계속 꺼진 셀은 항상 UI_TRACK_EMPTY 고정이라 색 변화와 무관하게 스킵. */
        if (want_lit == st->cell_lit[i] && !(want_lit && color_changed)) continue;
        st->cell_lit[i] = want_lit;
        lv_obj_t *cell = lv_obj_get_child(bar, (int32_t)i);
        lv_obj_set_style_bg_color(cell, want_lit ? lit_color : UI_TRACK_EMPTY, 0);
    }
}
