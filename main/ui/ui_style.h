#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Voltline 디자인 토큰 (팀장 제공, night 테마 — 이 프로젝트의 유일 테마, day 테마는
 * 이번 범위 밖).
 * 소스: docs/Voltline 전기오토바이 클러스터 UXUI/Cluster Dev Spec.dc.html §2 Colour
 *       docs/Voltline 전기오토바이 클러스터 UXUI/_ds/.../tokens/colors.css
 * 이전 loud.kr 레퍼런스 팔레트(UI_COLOR_BG/CYAN/...)는 전면 교체 — 병행 유지 안 함
 * (2026-09-02, Voltline 5탭 리디자인). */

#define UI_BG              lv_color_hex(0x06090C)
#define UI_PANEL           lv_color_hex(0x0E1419)
#define UI_HAIRLINE        lv_color_hex(0x1B242C)
#define UI_LINE_DEFAULT    lv_color_hex(0x26323C)
#define UI_TEXT_PRIMARY    lv_color_hex(0xEEF3F5)
#define UI_TEXT_SECONDARY  lv_color_hex(0x7F9099)
#define UI_TEXT_TERTIARY   lv_color_hex(0x586B76)
#define UI_ACCENT          lv_color_hex(0x00E5D0)
#define UI_LINE_ACCENT     lv_color_hex(0x00857A) /* current-600, colors.css --line-accent — 뱃지/필 테두리 전용 */
#define UI_TRACK_EMPTY     lv_color_hex(0x1B242C)

/* 신호색 — 자동차 텔테일 관례(ECE/ISO). 장식용 재사용/재틴트 금지, 램프·배지·바 외
 * 큰 면적에 칠하지 않는다 (스펙 §2). */
#define UI_SIGNAL_GO       lv_color_hex(0x3ED26B) /* 방향지시등, READY */
#define UI_SIGNAL_BEAM     lv_color_hex(0x3B82F6) /* 상향등 전용 — 다른 용도 금지 */
#define UI_SIGNAL_CAUTION  lv_color_hex(0xFFB020) /* 브레이크, TEMP CHECK, LIMITED */
#define UI_SIGNAL_CRITICAL lv_color_hex(0xFF4438) /* EV 경고등, DTC, critical 배너 */

/* 배터리 셀 에너지 램프 (SOC 퍼센트 구간별 색, 스펙 §2 energy ramp) */
#define UI_ENERGY_FULL     lv_color_hex(0x3ED26B) /* >55% */
#define UI_ENERGY_MID      lv_color_hex(0xC8E44A) /* >30% */
#define UI_ENERGY_LOW      lv_color_hex(0xFFB020) /* >12% */
#define UI_ENERGY_EMPTY    lv_color_hex(0xFF4438) /* <=12% */
#define UI_ENERGY_REGEN    UI_ACCENT

#define UI_ENERGY_THRESH_FULL_PCT 55
#define UI_ENERGY_THRESH_MID_PCT  30
#define UI_ENERGY_THRESH_LOW_PCT  12

/* 반경 (스펙 §shape.css) */
#define UI_RADIUS_CHIP     6
#define UI_RADIUS_CONTROL  10
#define UI_RADIUS_TILE     14
#define UI_RADIUS_PANEL    20
#define UI_RADIUS_SCREEN   28
#define UI_RADIUS_PILL     LV_RADIUS_CIRCLE

/* 모션 (스펙 §motion.css / §8 Motion budget). turn blink/caution pulse는 반드시
 * 하드스텝(이징 없음)이고, "Lite" 모션 모드가 생기더라도 절대 끄지 않는다 —
 * 법정 점멸 주기라 항상 켜져 있어야 한다. */
#define UI_ANIM_INSTANT_MS       80
#define UI_ANIM_FAST_MS          140
#define UI_ANIM_BASE_MS          220
#define UI_ANIM_TILE_SLIDE_MS    350
#define UI_ANIM_POWERON_SWEEP_MS 900
#define UI_ANIM_TURN_BLINK_MS    800
#define UI_ANIM_CAUTION_PULSE_MS 1200

/* 실차 경고 임계값 — 스펙 §7 Warnings/thresholds/DTCs 표에서 그대로 옮김(지어낸 값 아님). */
#define UI_MOTOR_TEMP_WARN_C   95  /* motor > 95C: TEMP CHECK 배너 + E-412 */
#define UI_BATTERY_LOW_PCT     12  /* SOC < 12%: 배터리 로우 경고, 셀 전부 red */

void ui_style_init(void);

/* 화면 배경 — 스펙은 "Flat. 그라데이션/텍스처 없음"이라 단색만 채운다. */
extern lv_style_t ui_style_bg;

/* 카드/패널 공통 스타일: UI_PANEL 채움 + 1px UI_HAIRLINE 테두리 + radius14.
 * 스펙은 드롭섀도 대신 inset 1px 하이라이트를 쓰지만, 애니메이션 중인 위젯에
 * shadow_width를 걸면 Core1 워치독 타임아웃+발열이 실기기에서 재현된 전례가 있어
 * (docs/design/ui-layout.md) 정적 요소에도 당분간 shadow 자체를 안 쓴다. */
extern lv_style_t ui_style_panel;

/* 텍스트 색상 전용 스타일(폰트는 각 위젯에서 개별 지정 — Voltline 숫자/라벨/모노
 * 폰트는 아직 미변환 상태라 당분간 내장 montserrat로 대체). */
extern lv_style_t ui_style_text_primary;
extern lv_style_t ui_style_text_secondary;
extern lv_style_t ui_style_text_tertiary;

#ifdef __cplusplus
}
#endif
