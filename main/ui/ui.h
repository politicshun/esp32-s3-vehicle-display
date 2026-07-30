#pragma once
/*
 * ui.h — 디지털 클러스터 4페이지 UI 공개 API
 *
 * lvgl_ui_task() (main/lvgl.c)의 단일 태스크 안에서만 호출된다.
 * 이 프로젝트는 lvgl.c가 lv_init()부터 lv_timer_handler()까지 전부
 * 한 태스크(Core 1)에서 돌리는 구조라 별도 LVGL 뮤텍스 락이 없다 —
 * 따라서 ui_init()/ui_update()도 락 없이 그대로 호출하면 된다.
 *
 * 사용법 (main/lvgl.c):
 *
 *     lvgl_ui_build() 안에서:            ui_init(lv_scr_act());
 *     lvgl_ui_task()의 while(1) 루프 안: ui_update();
 */

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(lv_obj_t *parent);
void ui_update(void);

#ifdef __cplusplus
}
#endif
