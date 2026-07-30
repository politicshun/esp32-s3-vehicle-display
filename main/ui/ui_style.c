#include "ui_style.h"

lv_style_t ui_style_bg;
lv_style_t ui_style_label_big;
lv_style_t ui_style_label_mid;
lv_style_t ui_style_label_small;

void ui_style_init(void)
{
    lv_style_init(&ui_style_bg);
    lv_style_set_bg_color(&ui_style_bg, UI_COLOR_BG);
    lv_style_set_bg_opa(&ui_style_bg, LV_OPA_COVER);
    lv_style_set_border_width(&ui_style_bg, 0);
    lv_style_set_pad_all(&ui_style_bg, 0);

    lv_style_init(&ui_style_label_big);
    lv_style_set_text_font(&ui_style_label_big, &lv_font_montserrat_48);
    lv_style_set_text_color(&ui_style_label_big, UI_COLOR_TEXT_PRI);

    lv_style_init(&ui_style_label_mid);
    lv_style_set_text_font(&ui_style_label_mid, &lv_font_montserrat_24);
    lv_style_set_text_color(&ui_style_label_mid, UI_COLOR_TEXT_PRI);

    lv_style_init(&ui_style_label_small);
    lv_style_set_text_font(&ui_style_label_small, &lv_font_montserrat_14);
    lv_style_set_text_color(&ui_style_label_small, UI_COLOR_TEXT_SEC);
}
