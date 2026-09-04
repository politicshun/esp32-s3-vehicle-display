#pragma once
/*
 * ui_widget_readout.h — "숫자값 + 작은 단위" 한 줄 위젯. Ride 탭(phase 6)에서
 * 처음 만들어졌고 Trip/Faults/Charging(phase 8)도 동일 패턴을 쓰므로 공용으로 승격.
 */
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* num_font로 값을, 있으면 unit_text를 작은 회색 글씨로 옆에 붙인다(baseline은
 * flex align-items:end로 근사). 반환값은 값 라벨 — 호출자가 lv_label_set_text로
 * 갱신한다. 초기 텍스트는 "-"(숫자 전용 폰트 서브셋엔 em dash가 없음). */
lv_obj_t *ui_widget_readout_create(lv_obj_t *parent, const lv_font_t *num_font,
                                    lv_color_t color, const char *unit_text);

#ifdef __cplusplus
}
#endif
