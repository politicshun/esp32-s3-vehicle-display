#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *ui_tile_faults_build(lv_obj_t *tile_parent);
void ui_tile_faults_update(void);

#ifdef __cplusplus
}
#endif
