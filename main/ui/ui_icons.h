#pragma once
/* scripts/gen_icons.py 로 생성 — 직접 수정하지 말 것 */
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t ui_icon_battery_22;
extern const lv_img_dsc_t ui_icon_battery_26;
extern const lv_img_dsc_t ui_icon_bluetooth_22;
extern const lv_img_dsc_t ui_icon_bluetooth_26;
extern const lv_img_dsc_t ui_icon_brake_22;
extern const lv_img_dsc_t ui_icon_brake_26;
extern const lv_img_dsc_t ui_icon_controller_22;
extern const lv_img_dsc_t ui_icon_controller_26;
extern const lv_img_dsc_t ui_icon_ev_warning_22;
extern const lv_img_dsc_t ui_icon_ev_warning_26;
extern const lv_img_dsc_t ui_icon_warning_tri_22;
extern const lv_img_dsc_t ui_icon_warning_tri_26;
extern const lv_img_dsc_t ui_icon_highbeam_22;
extern const lv_img_dsc_t ui_icon_highbeam_26;
extern const lv_img_dsc_t ui_icon_temp_22;
extern const lv_img_dsc_t ui_icon_temp_26;
extern const lv_img_dsc_t ui_icon_turn_r_22;
extern const lv_img_dsc_t ui_icon_turn_r_26;
extern const lv_img_dsc_t ui_icon_turn_l_22;
extern const lv_img_dsc_t ui_icon_turn_l_26;

/* tint: 신호색 등 런타임 틴트. off_opa: 꺼짐 상태 opa(0-255), 항상 hidden 아님 */
void ui_icon_set(lv_obj_t *img_obj, const lv_img_dsc_t *src, lv_color_t tint, lv_opa_t opa);

#ifdef __cplusplus
}
#endif
