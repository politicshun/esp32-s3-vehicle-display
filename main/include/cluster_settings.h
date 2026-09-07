#pragma once

#include <stdint.h>
#include <stdbool.h>

// 2026-09-04: Setup 탭에서 운전자가 조절하는 값들 — CLUSTER -> INVERTER로 나가는
// ClusterSettings(0x500, docs/hardware/cluster.dbc) 페이로드의 소스.
// vehicle_data.h(INVERTER -> CLUSTER 수신값)와 방향이 반대라 별도 모듈로 뺐다.
//
// 여전히 로컬 UI 데모다 — main/ui/ui_tile_setup.c 상단 주석 참고(백라이트 PWM/NVS
// 배선이 없어 재부팅하면 초기값으로 돌아감). 이 구조체는 "그 값을 CAN으로 내보내는
// 파이프"만 만든 것 — 인버터가 받아서 뭘 하는지는 인버터측 구현 영역.
typedef struct {
    uint8_t brightness_pct;  // [10|100] — ui_tile_setup.c 슬라이더 range와 동일
    uint8_t regen_level;     // [0|3]
    bool    auto_headlight;
    bool    auto_day_night;
    bool    units_mph;       // false=km/h+km(기본), true=mph+mi
} ClusterSettings_t;

void cluster_settings_init(void);
void cluster_settings_set(const ClusterSettings_t *src);
void cluster_settings_get(ClusterSettings_t *dst);
