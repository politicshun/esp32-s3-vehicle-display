#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *ui_tile_charging_build(lv_obj_t *tile_parent);
void ui_tile_charging_update(void);

#ifdef __cplusplus
}
#endif
