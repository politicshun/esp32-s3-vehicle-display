#include "ui_style.h"

lv_style_t ui_style_bg;
lv_style_t ui_style_panel;
lv_style_t ui_style_text_primary;
lv_style_t ui_style_text_secondary;
lv_style_t ui_style_text_tertiary;

void ui_style_init(void)
{
    lv_style_init(&ui_style_bg);
    lv_style_set_bg_color(&ui_style_bg, UI_BG);
    lv_style_set_bg_opa(&ui_style_bg, LV_OPA_COVER);
    lv_style_set_border_width(&ui_style_bg, 0);
    lv_style_set_pad_all(&ui_style_bg, 0);

    lv_style_init(&ui_style_panel);
    lv_style_set_bg_color(&ui_style_panel, UI_PANEL);
    lv_style_set_bg_opa(&ui_style_panel, LV_OPA_COVER);
    lv_style_set_border_color(&ui_style_panel, UI_HAIRLINE);
    lv_style_set_border_width(&ui_style_panel, 1);
    lv_style_set_border_opa(&ui_style_panel, LV_OPA_COVER);
    lv_style_set_radius(&ui_style_panel, UI_RADIUS_TILE);
    lv_style_set_pad_all(&ui_style_panel, 14);
    lv_style_set_shadow_width(&ui_style_panel, 0);

    lv_style_init(&ui_style_text_primary);
    lv_style_set_text_color(&ui_style_text_primary, UI_TEXT_PRIMARY);

    lv_style_init(&ui_style_text_secondary);
    lv_style_set_text_color(&ui_style_text_secondary, UI_TEXT_SECONDARY);

    lv_style_init(&ui_style_text_tertiary);
    lv_style_set_text_color(&ui_style_text_tertiary, UI_TEXT_TERTIARY);
}
