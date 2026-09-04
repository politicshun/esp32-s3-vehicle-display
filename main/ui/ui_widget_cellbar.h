#pragma once
/*
 * ui_widget_cellbar.h — SOC N-cell 바 위젯 (스펙 Cluster Dev Spec.dc.html
 * §Implementation notes "Battery cells": 22px 셸, 1px line-default 테두리,
 * radius 3, padding 3, cell gap 3). Ride/Charging 탭 공용(phase 6 계획 §5).
 * 우측 3x10 nub는 스펙상 셸 밖 장식이라 이 위젯에 포함하지 않고 호출자가
 * 별도로 붙인다.
 */
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 2026-09-03: 내부 SRAM 예산 초과로 실기기 크래시(project_2026-09-02_voltline_5tile_ui_rewrite
 * 메모리 참고) — SOC 셀바 해상도를 10->5로 낮춰 Ride/Charging 합쳐 lv_obj_t 10개 절감.
 * 셀당 20%(스펙 10%보다 거칠지만 시각적으로는 여전히 단차 구분 가능). */
#define UI_SOC_CELL_COUNT 5

lv_obj_t *ui_widget_cellbar_create(lv_obj_t *parent, int cell_count);

/* lit_count(0..cell_count로 clamp)개의 셀을 lit_color로, 나머지는
 * UI_TRACK_EMPTY로 칠한다. */
void ui_widget_cellbar_set(lv_obj_t *bar, int lit_count, lv_color_t lit_color);

#ifdef __cplusplus
}
#endif
