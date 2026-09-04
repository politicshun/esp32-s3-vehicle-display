#pragma once
/*
 * ui_fonts_label.h — Voltline 라벨/모노 폰트 (계획 §3 phase 9).
 * ui_font_label_*: Archivo 600(SemiBold, variable font을 fontTools instancer로
 * wght=600 고정 추출) — 대문자 변환은 코드에서 이미 대문자로 쓰고, 자간(0.08em)은
 * lv_style_set_text_letter_space로 런타임 적용(폰트에 안 구움).
 * ui_font_mono_*: JetBrains Mono 500(Medium, 동일하게 wght=500 고정 추출).
 * 두 폰트 다 ASCII printable 전체 + °·— 서브셋(캡션/코드/펌웨어 문자열 커버).
 */
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t ui_font_label_13; /* 카드 캡션(BATTERY, RANGE, ...) */
extern const lv_font_t ui_font_label_15; /* 기어 배지 등 강조 라벨 */
extern const lv_font_t ui_font_mono_12;  /* DTC 코드 / FW·시리얼 / elapsed */

#ifdef __cplusplus
}
#endif
