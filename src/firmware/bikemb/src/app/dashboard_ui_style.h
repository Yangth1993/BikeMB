#pragma once

#include <stdint.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_color_t BikeMbUi_Rgb(uint8_t r, uint8_t g, uint8_t b);
lv_color_t BikeMbUi_ColorBg(void);
lv_color_t BikeMbUi_ColorPanel(void);
lv_color_t BikeMbUi_ColorCyan(void);
lv_color_t BikeMbUi_ColorLime(void);
lv_color_t BikeMbUi_ColorEco(void);
lv_color_t BikeMbUi_ColorTrail(void);
lv_color_t BikeMbUi_ColorAuto(void);
lv_color_t BikeMbUi_ColorBoost(void);
lv_color_t BikeMbUi_ModeColor(uint8_t mode_index);
lv_color_t BikeMbUi_ColorText(void);
lv_color_t BikeMbUi_ColorMuted(void);

void BikeMbUi_StyleClear(lv_obj_t *obj);
void BikeMbUi_StylePage(lv_obj_t *obj);
void BikeMbUi_StylePanel(lv_obj_t *obj, lv_coord_t radius);

lv_obj_t *BikeMbUi_MakeLabel(lv_obj_t *parent,
                             const char *text,
                             const lv_font_t *font,
                             lv_color_t color,
                             lv_coord_t x,
                             lv_coord_t y);
lv_obj_t *BikeMbUi_MakeFixedLabel(lv_obj_t *parent,
                                  const char *text,
                                  const lv_font_t *font,
                                  lv_color_t color,
                                  lv_coord_t x,
                                  lv_coord_t y,
                                  lv_coord_t width,
                                  lv_text_align_t align);
void BikeMbUi_SetLabelTextIfChanged(lv_obj_t *label, const char *text);

#ifdef __cplusplus
}
#endif
