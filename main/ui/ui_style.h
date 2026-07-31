#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 레퍼런스 클러스터 디자인(사용자 제공 이미지)에서 뽑은 팔레트.
 * docs/design/ui-layout.md 의 토큰 표와 1:1 대응. */
#define UI_COLOR_BG          lv_color_hex(0x0A0E14)
#define UI_COLOR_BG_GRAD     lv_color_hex(0x141C2E) /* 배경 그라데이션 하단 톤 — BG보다 살짝 밝은 네이비 */
#define UI_COLOR_CYAN        lv_color_hex(0x2FD8E8)
#define UI_COLOR_CYAN_DEEP   lv_color_hex(0x1560A0) /* 게이지 그라데이션 아크의 저값 쪽 색 */
#define UI_COLOR_GREEN       lv_color_hex(0x3CE87A)
#define UI_COLOR_YELLOW      lv_color_hex(0xE8C93C)
#define UI_COLOR_RED         lv_color_hex(0xE84C3C)
#define UI_COLOR_TEXT_PRI    lv_color_hex(0xFFFFFF)
#define UI_COLOR_TEXT_SEC    lv_color_hex(0x8A97A8)

#define UI_SOC_LOW_PCT   20
#define UI_TEMP_WARN_C   60 /* placeholder 임계값 — 실차 BMS 온도 경고 기준 미확정 */

void ui_style_init(void);

extern lv_style_t ui_style_bg;
extern lv_style_t ui_style_label_big;    /* 48px 큰 숫자 */
extern lv_style_t ui_style_label_mid;    /* 24px */
extern lv_style_t ui_style_label_small;  /* 14px, secondary 색상 */

#ifdef __cplusplus
}
#endif
