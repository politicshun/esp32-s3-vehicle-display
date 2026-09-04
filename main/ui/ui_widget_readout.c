#include "ui_widget_readout.h"
#include "ui_style.h"
#include "ui_fonts_label.h"

lv_obj_t *ui_widget_readout_create(lv_obj_t *parent, const lv_font_t *num_font,
                                    lv_color_t color, const char *unit_text)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_column(row, 4, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *val = lv_label_create(row);
    lv_obj_set_style_text_font(val, num_font, 0);
    lv_obj_set_style_text_color(val, color, 0);
    lv_label_set_text(val, "-");

    if (unit_text) {
        lv_obj_t *unit = lv_label_create(row);
        /* 스펙은 단위 접미사도 숫자 폰트(Barlow) 상속이지만, 그 폰트는 digits-only
         * 서브셋이라 문자(k/m/h/V/W/C)를 못 그린다 — 라벨 폰트로 대체(계획 §3 phase 9 비고). */
        lv_obj_set_style_text_font(unit, &ui_font_label_13, 0);
        lv_obj_set_style_text_color(unit, UI_TEXT_TERTIARY, 0);
        lv_label_set_text(unit, unit_text);
    }
    return val;
}
