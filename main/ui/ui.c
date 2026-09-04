#include "ui.h"
#include "ui_style.h"
#include "ui_chrome.h"
#include "ui_trip_state.h"
#include "ui_tile_ride.h"
#include "ui_tile_faults.h"
#include "ui_tile_setup.h"
#include "ui_tile_charging.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * ui.c — Voltline 클러스터 오케스트레이터 (2026-09-02, 팀장 제공 디자인
 * 전면 반영 — 이전 4탭/단일페이지 구조는 폐기, 구조 참고는 git 커밋 44af0e7).
 * 2026-09-03: 원래 5탭(Ride/Trip/Faults/Setup/Charging)이었으나 내부 SRAM 예산
 * 초과로 Trip 탭 제거, 4탭으로 축소(project_2026-09-02_voltline_5tile_ui_rewrite
 * 메모리 참고 — Trip A/ODO 정보는 Ride 탭 카드에 이미 있어 정보 손실 없음).
 *
 * 화면 = 세로 flex 3단: 헤더(44px, ui_chrome) / 콘텐츠(가변, lv_tileview 4칸) /
 * 푸터(44px, ui_chrome). 헤더·푸터는 타일이 바뀌어도 절대 언마운트되지 않는다.
 *
 * 내비게이션: 스크롤/드래그 없음(2026-09-02 사용자 지시) — 콘텐츠 밴드를 좌/우
 * 반반으로 덮는 투명 탭존(구 4탭 구현 44af0e7의 패턴 그대로 재사용)과 푸터
 * 페이지도트로만 전환한다. 각 타일은 LV_DIR_NONE으로 만들어 tileview 자체의
 * 내부 드래그도 비활성.
 */

#define UI_TILE_COUNT UI_CHROME_TILE_COUNT /* 4: Ride, Faults, Setup, Charging */

static lv_obj_t *s_tileview;
static lv_obj_t *s_tiles[UI_TILE_COUNT];
static int s_active_tile = 0;

extern void lvgl_request_tearfree_page_switch(void);

static void update_tile(int index)
{
    switch (index) {
        case 0: ui_tile_ride_update(); break;
        case 1: ui_tile_faults_update(); break;
        case 2: ui_tile_setup_update(); break;
        case 3: ui_tile_charging_update(); break;
        default: break;
    }
}

static void goto_tile(int target)
{
    if (target < 0) target = 0;
    if (target > UI_TILE_COUNT - 1) target = UI_TILE_COUNT - 1;
    if (target == s_active_tile) return;

    /* 2026-09-04(11차): lvgl_request_tearfree_page_switch() 호출을 뺐다 — 실기기
     * 사진으로 확인 결과(docs/20260904_102603.jpg) 탭을 누르면 화면이 대각선 노이즈
     * 줄무늬를 넘어 아예 화면 대부분이 까맣게 죽어버리는, 이전보다 훨씬 심한 증상이
     * 나왔다. 이 스왑은 탭 전체를 LVGL이 소프트웨어로 PSRAM 프레임버퍼에 직접
     * 래스터라이즈하게 만드는데(수 ms 소요, LCD가 같은 PSRAM 버스를 통해 계속
     * bounce buffer를 리필해야 하는 것과 경합) — ESP32-S3 RGB LCD+PSRAM 조합에서
     * 잘 알려진 "PSRAM 버스 경합으로 패널이 그 순간 스캔아웃할 데이터를 못 받아
     * 깨진 픽셀/동기 유실"이 일어나는 것으로 보임(8~10차에서 시도한 버퍼 스왑
     * 인덱스 수정들은 이 문제와 무관해서 다 효과가 없었음). 8~10차 수정으로 이
     * 함수 자체의 "어느 버퍼에 그릴지" 버그는 고쳤지만, 그거와 별개로 이 함수를
     * 아예 안 쓰는 쪽으로 되돌린다 — 대신 일반 48라인 SRAM 밴드 버퍼로 그린다.
     * 밴드 카피는 라이브 버퍼에 CPU memcpy라 여전히 테어링 여지는 있지만
     * "화면이 죽는" 수준보다는 훨씬 낫다. `update_tile(target)`은 유지 — 새로
     * 보이는 탭이 낡은 값이 아니라 최신값으로 바로 뜨는 건 여전히 맞는 개선. */
    lv_obj_set_tile_id(s_tileview, target, 0, LV_ANIM_OFF);
    update_tile(target);
}

static void goto_tile_delta(int delta)
{
    goto_tile(s_active_tile + delta);
}

static void tileview_event_cb(lv_event_t *e)
{
    lv_obj_t *tv = lv_event_get_target(e);
    lv_obj_t *act = lv_tileview_get_tile_act(tv);

    for (int i = 0; i < UI_TILE_COUNT; i++) {
        if (s_tiles[i] == act) {
            s_active_tile = i;
            break;
        }
    }
    ui_chrome_set_active_tile(s_active_tile);
}

static void tap_zone_event_cb(lv_event_t *e)
{
    int delta = (int)(intptr_t)lv_event_get_user_data(e);
    goto_tile_delta(delta);
}

static void build_tap_zones(lv_obj_t *content_parent)
{
    lv_obj_t *tap_left = lv_obj_create(content_parent);
    lv_obj_remove_style_all(tap_left);
    lv_obj_set_size(tap_left, lv_pct(50), lv_pct(100));
    lv_obj_align(tap_left, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(tap_left, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(tap_left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tap_left, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tap_left, tap_zone_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)-1);

    lv_obj_t *tap_right = lv_obj_create(content_parent);
    lv_obj_remove_style_all(tap_right);
    lv_obj_set_size(tap_right, lv_pct(50), lv_pct(100));
    lv_obj_align(tap_right, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(tap_right, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(tap_right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tap_right, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(tap_right, tap_zone_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)1);
}

void ui_init(lv_obj_t *parent)
{
    ui_style_init();
    ui_trip_state_init();

    lv_obj_remove_style_all(parent);
    lv_obj_add_style(parent, &ui_style_bg, 0);
    lv_obj_set_style_pad_all(parent, 8, 0); /* 세이프 에어리어 8px 전방향 (스펙 §1) */
    lv_obj_set_style_pad_row(parent, 8, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    ui_chrome_build_header(parent);

    lv_obj_t *content = lv_obj_create(parent);
    lv_obj_remove_style_all(content);
    lv_obj_set_width(content, lv_pct(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    s_tileview = lv_tileview_create(content);
    lv_obj_add_style(s_tileview, &ui_style_bg, 0);
    lv_obj_set_size(s_tileview, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(s_tileview, LV_OBJ_FLAG_SCROLLABLE);

    /* 실기기 확인(2026-09-03): 5탭 전부를 여기서 한 번에 지으면 lvgl_ui_task가
     * Core1을 계속 점유해 IDLE1이 못 돌고 task watchdog이 터진다(부팅 로그로 실측 —
     * 5탭 빌드에 10초 이상 소요, 이전 4탭 구조보다 위젯 수가 훨씬 많아짐). 타일 사이에
     * 1틱씩 양보해 idle 태스크가 watchdog을 리셋할 기회를 준다 — 화면은 아직 안 보이는
     * 시점(첫 lv_timer_handler 전)이라 사용자 체감 지연은 없다. */
    s_tiles[0] = ui_tile_ride_build(s_tileview);
    vTaskDelay(1);
    s_tiles[1] = ui_tile_faults_build(s_tileview);
    vTaskDelay(1);
    s_tiles[2] = ui_tile_setup_build(s_tileview);
    vTaskDelay(1);
    s_tiles[3] = ui_tile_charging_build(s_tileview);
    lv_obj_add_event_cb(s_tileview, tileview_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    build_tap_zones(content); /* tileview 다음에 추가돼야 터치를 위에서 가로챈다 */

    ui_chrome_build_footer(parent, goto_tile);
    ui_chrome_set_active_tile(s_active_tile);
}

void ui_update(void)
{
    ui_chrome_update();
    /* 활성 탭과 무관하게 매 프레임 적분 — Faults/Setup 탭을 보고 있는 동안에도
     * 트립 거리는 계속 쌓여야 한다(Ride 탭의 TRIP A/ODO 카드가 봄, ui_trip_state.h 참고). */
    ui_trip_state_tick();

    update_tile(s_active_tile);
}
