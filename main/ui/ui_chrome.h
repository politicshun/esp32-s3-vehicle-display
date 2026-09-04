#pragma once
/*
 * ui_chrome.h — Voltline 클러스터의 상시 헤더(44px)/푸터(44px)
 *
 * 스펙(Cluster Dev Spec.dc.html §1 Band structure): 이 두 밴드는 모든 타일에서
 * 절대 언마운트되지 않는다 — ui.c가 화면 루트 flex-column의 첫/마지막 자식으로
 * 붙이고, 가운데 lv_tileview만 타일마다 내용이 바뀐다.
 */
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_CHROME_TILE_COUNT 4 /* Ride, Faults, Setup, Charging (순서 고정) —
                                 * 2026-09-03: Trip 탭(구 5탭 중 위젯 밀도 최다)을 내부
                                 * SRAM 예산 확보를 위해 제거. Trip A/ODO 정보는 Ride
                                 * 탭 카드에 이미 있어 정보 손실 없음(ui_trip_state는
                                 * 계속 사용, 탭만 없앰). */

typedef void (*ui_chrome_goto_tile_cb_t)(int index);

/* 44px 상시 헤더: 시계 / 외기온 / BLE 램프 / 텔테일 5개(턴L, 상향등, 브레이크,
 * EV경고, 턴R). parent 폭 100%를 그대로 채운다. */
lv_obj_t *ui_chrome_build_header(lv_obj_t *parent);

/* 44px 상시 푸터: 팩전압/모터온도/컨트롤러온도 + 중앙 DTC칩(조건부) + 페이지 도트
 * + 상태단어. goto_cb는 도트/DTC칩을 탭했을 때 호출되고, 실제 lv_tileview 전환은
 * ui.c가 수행한다(chrome은 tileview를 모른다). */
lv_obj_t *ui_chrome_build_footer(lv_obj_t *parent, ui_chrome_goto_tile_cb_t goto_cb);

/* 활성 타일이 바뀔 때(ui.c의 tileview LV_EVENT_VALUE_CHANGED에서) 호출 —
 * 페이지 도트 하이라이트만 갱신한다. */
void ui_chrome_set_active_tile(int index);

/* ui_update()에서 매 프레임 호출. 지금은 골격만 — 실데이터/아이콘 배선은
 * 이후 phase(4/5)에서 채운다. */
void ui_chrome_update(void);

#ifdef __cplusplus
}
#endif
