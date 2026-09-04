#pragma once
/*
 * ui_trip_state.h — 클라이언트 측 트립 적분기 (계획 §2 데이터 계약: "트립 통계는
 * 발명이 아니라 실측값의 계산이므로 허용"). speed/power_kw/regen_kw 실측값을
 * lv_tick 기준으로 적분해 트립 거리/평균·최고속도/에너지/회생량/경과시간을 낸다.
 *
 * Ride 탭(Trip A 카드, phase 6)과 Trip 탭(phase 8) 양쪽에서 공유하는 단일 소스 —
 * 활성 타일과 무관하게 ui_update()에서 매 프레임 tick()해야 주행 중 다른 탭을
 * 보고 있어도 거리가 계속 누적된다.
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float    dist_km;        /* |speed| 적분 */
    float    avg_speed_kmh;  /* dist_km / (ride_time_s/3600), 무주행이면 0 */
    float    max_speed_kmh;  /* |speed| 최댓값 */
    float    energy_wh;      /* power_kw 적분 (Wh) */
    float    regen_wh;       /* regen_kw 적분 (Wh) */
    uint32_t ride_time_s;    /* 마지막 reset 이후 경과시간(주행 여부 무관, wall time) */
} ui_trip_stats_t;

void ui_trip_state_init(void);
void ui_trip_state_tick(void);
void ui_trip_state_reset(void);
void ui_trip_state_get(ui_trip_stats_t *out);

#ifdef __cplusplus
}
#endif
