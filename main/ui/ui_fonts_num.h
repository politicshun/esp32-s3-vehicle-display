#pragma once
/*
 * ui_fonts_num.h — Voltline 숫자 전용 폰트(Barlow Semi Condensed 600 italic,
 * digits-only 서브셋 " -.,0123456789"). 계획 §3 폰트 파이프라인, phase 7(Ride가
 * 쓰는 사이즈부터). 각 크기는 main/ui/ui_font_num_<px>.c 1파일(ui_font_hero_80.c
 * 관례 그대로) — 원본 TTF는 변환 후 폐기, .fallback = NULL 고정.
 */
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t ui_font_num_250; /* 속도 (< 100 km/h) */
extern const lv_font_t ui_font_num_190; /* 속도 (>= 100 km/h) */
extern const lv_font_t ui_font_num_120; /* Charging SOC */
extern const lv_font_t ui_font_num_60;  /* Ride BATTERY % */
extern const lv_font_t ui_font_num_46;  /* RANGE / TRIP A distance / Trip 3x2 카드 / Faults 4칸 */
extern const lv_font_t ui_font_num_34;  /* ODO / Trip 하단 스트립 */

#ifdef __cplusplus
}
#endif
