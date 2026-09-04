#include "ui_icons.h"

/* ui_icons.c/.h는 scripts/gen_icons.py가 생성한 순수 데이터라 재생성 시 덮어써진다
 * — 이 파일은 손으로 관리하는 구현부라 별도로 뒀다. */
void ui_icon_set(lv_obj_t *img_obj, const lv_img_dsc_t *src, lv_color_t tint, lv_opa_t opa)
{
    lv_img_set_src(img_obj, src);
    lv_obj_set_style_img_recolor(img_obj, tint, 0);
    lv_obj_set_style_img_recolor_opa(img_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_img_opa(img_obj, opa, 0);
}
