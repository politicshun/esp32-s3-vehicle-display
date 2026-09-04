#include "ui_trip_state.h"
#include "vehicle_data.h"
#include "lvgl.h"
#include <math.h>
#include <string.h>
#include <stdbool.h>

static ui_trip_stats_t s_stats;
static uint32_t        s_last_tick_ms;
static uint32_t        s_elapsed_ms;
static bool             s_have_last_tick;

void ui_trip_state_init(void)
{
    memset(&s_stats, 0, sizeof(s_stats));
    s_last_tick_ms   = lv_tick_get();
    s_elapsed_ms     = 0;
    s_have_last_tick = true;
}

void ui_trip_state_reset(void)
{
    memset(&s_stats, 0, sizeof(s_stats));
    s_last_tick_ms = lv_tick_get();
    s_elapsed_ms   = 0;
}

void ui_trip_state_tick(void)
{
    uint32_t now_ms = lv_tick_get();
    if (!s_have_last_tick) {
        s_last_tick_ms   = now_ms;
        s_have_last_tick = true;
        return;
    }
    uint32_t dt_ms = lv_tick_elaps(s_last_tick_ms);
    s_last_tick_ms = now_ms;
    if (dt_ms == 0) return;

    VehicleData_t d;
    vehicle_data_get(&d);

    float dt_h = (float)dt_ms / 3600000.0f;
    float speed_abs = fabsf((float)d.speed);

    s_stats.dist_km   += speed_abs * dt_h;
    s_stats.energy_wh += d.power_kw * 1000.0f * dt_h;
    s_stats.regen_wh  += d.regen_kw * 1000.0f * dt_h;
    if (speed_abs > s_stats.max_speed_kmh) s_stats.max_speed_kmh = speed_abs;

    s_elapsed_ms += dt_ms;
    s_stats.ride_time_s = s_elapsed_ms / 1000;

    s_stats.avg_speed_kmh = (s_stats.ride_time_s > 0)
        ? s_stats.dist_km / ((float)s_stats.ride_time_s / 3600.0f)
        : 0.0f;
}

void ui_trip_state_get(ui_trip_stats_t *out)
{
    *out = s_stats;
}
