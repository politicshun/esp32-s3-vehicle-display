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

/* 계기판 "히어로" 숫자(속도/파워/SOC/온도/전압/기어)용 커스텀 폰트.
 * LVGL 내장 몽세라는 48px가 상한이라, Rajdhani Bold(구글 폰트, SIL OFL 1.1)를
 * 80px로 lv_font_conv 변환해서 만듦 — 실제 화면에 쓰이는 문자만 포함
 * (" -.0123456789%VCPRND", scripts 폴더가 아니라 main/ui/ui_font_hero_80.c에
 * 생성 결과물만 커밋됨 — 원본 TTF는 라이선스상 리포에 안 넣고 폰트 자체를 구운 산출물만 둠).
 * 2026-07-30에 한글 서브셋 폰트를 --lv-fallback으로 몽세라에 연결했다가 라틴 텍스트까지
 * 안 보이는 원인 불명 버그가 있었던 전례(docs/design/ui-layout.md) 때문에, 이번엔
 * fallback 체인을 아예 안 씀(.fallback = NULL, 필요한 문자를 전부 이 폰트 하나에 포함). */
extern const lv_font_t ui_font_hero_80;

void ui_style_init(void);

extern lv_style_t ui_style_bg;
extern lv_style_t ui_style_label_hero;   /* 80px, 커스텀 폰트 — 계기판 히어로 숫자 전용 */
extern lv_style_t ui_style_label_big;    /* 48px 큰 숫자 */
extern lv_style_t ui_style_label_mid;    /* 24px */
extern lv_style_t ui_style_label_small;  /* 14px, secondary 색상 */

#ifdef __cplusplus
}
#endif
