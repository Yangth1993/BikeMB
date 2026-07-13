#include "dashboard_ui_style.h"

#include <string.h>

lv_color_t BikeMbUi_Rgb(uint8_t r, uint8_t g, uint8_t b) {
  return lv_color_make(r, g, b);
}

lv_color_t BikeMbUi_ColorBg(void) {
  return BikeMbUi_Rgb(3, 5, 8);
}

lv_color_t BikeMbUi_ColorPanel(void) {
  return BikeMbUi_Rgb(23, 25, 29);
}

lv_color_t BikeMbUi_ColorCyan(void) {
  return BikeMbUi_Rgb(24, 222, 239);
}

lv_color_t BikeMbUi_ColorLime(void) {
  return BikeMbUi_Rgb(178, 245, 38);
}

lv_color_t BikeMbUi_ColorText(void) {
  return BikeMbUi_Rgb(246, 248, 250);
}

lv_color_t BikeMbUi_ColorMuted(void) {
  return BikeMbUi_Rgb(143, 149, 157);
}

void BikeMbUi_StyleClear(lv_obj_t *obj) {
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
}

void BikeMbUi_StylePage(lv_obj_t *obj) {
  BikeMbUi_StyleClear(obj);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(obj, 360, 360);
  lv_obj_center(obj);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
}

void BikeMbUi_StylePanel(lv_obj_t *obj, lv_coord_t radius) {
  BikeMbUi_StyleClear(obj);
  lv_obj_set_style_bg_color(obj, BikeMbUi_ColorPanel(), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_border_color(obj, BikeMbUi_Rgb(58, 62, 68), 0);
  lv_obj_set_style_radius(obj, radius, 0);
}

lv_obj_t *BikeMbUi_MakeLabel(lv_obj_t *parent,
                             const char *text,
                             const lv_font_t *font,
                             lv_color_t color,
                             lv_coord_t x,
                             lv_coord_t y) {
  lv_obj_t *label = lv_label_create(parent);
  lv_obj_set_pos(label, x, y);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, color, 0);
  lv_obj_set_style_text_letter_space(label, 0, 0);
  lv_label_set_text(label, text);
  return label;
}

lv_obj_t *BikeMbUi_MakeFixedLabel(lv_obj_t *parent,
                                  const char *text,
                                  const lv_font_t *font,
                                  lv_color_t color,
                                  lv_coord_t x,
                                  lv_coord_t y,
                                  lv_coord_t width,
                                  lv_text_align_t align) {
  lv_obj_t *label = BikeMbUi_MakeLabel(parent, text, font, color, x, y);
  lv_obj_set_width(label, width);
  lv_obj_set_style_text_align(label, align, 0);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  return label;
}

void BikeMbUi_SetLabelTextIfChanged(lv_obj_t *label, const char *text) {
  const char *current = lv_label_get_text(label);
  if (current == NULL || strcmp(current, text) != 0) {
    lv_label_set_text(label, text);
  }
}

