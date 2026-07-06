#include "dashboard_view.h"

#include <Arduino.h>
#include <lvgl.h>

namespace {

lv_obj_t *g_screen = nullptr;
lv_obj_t *g_titleLabel = nullptr;
lv_obj_t *g_subtitleLabel = nullptr;
lv_obj_t *g_statsPanel = nullptr;
lv_obj_t *g_cpuLabel = nullptr;
lv_obj_t *g_memLabel = nullptr;
lv_obj_t *g_fpsLabel = nullptr;
lv_obj_t *g_psramLabel = nullptr;
lv_obj_t *g_cpuBar = nullptr;
lv_obj_t *g_memBar = nullptr;
lv_obj_t *g_psramBar = nullptr;
lv_obj_t *g_orb = nullptr;

lv_color_t Rgb(uint8_t r, uint8_t g, uint8_t b) {
  return lv_color_make(r, g, b);
}

void ApplyBaseStyle(lv_obj_t *obj, lv_color_t bg) {
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(obj, bg, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_radius(obj, 18, 0);
}

void CreateStatRow(
    lv_obj_t *parent,
    lv_obj_t **labelOut,
    lv_obj_t **barOut,
    lv_coord_t y,
    const char *initialText,
    lv_color_t barColor) {
  *labelOut = lv_label_create(parent);
  lv_obj_set_pos(*labelOut, 18, y);
  lv_obj_set_style_text_font(*labelOut, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(*labelOut, Rgb(238, 244, 248), 0);
  lv_label_set_text(*labelOut, initialText);

  *barOut = lv_bar_create(parent);
  lv_obj_set_size(*barOut, 116, 14);
  lv_obj_set_pos(*barOut, 148, y + 2);
  lv_bar_set_range(*barOut, 0, 100);
  lv_bar_set_value(*barOut, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(*barOut, Rgb(29, 43, 60), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(*barOut, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(*barOut, 6, LV_PART_MAIN);
  lv_obj_set_style_bg_color(*barOut, barColor, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(*barOut, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(*barOut, 6, LV_PART_INDICATOR);
}

}  // namespace

void DashboardView_Create() {
  g_screen = lv_obj_create(nullptr);
  lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(g_screen, Rgb(7, 13, 24), 0);
  lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(g_screen, 0, 0);
  lv_obj_set_style_radius(g_screen, 0, 0);

  g_titleLabel = lv_label_create(g_screen);
  lv_obj_set_pos(g_titleLabel, 80, 24);
  lv_obj_set_style_text_font(g_titleLabel, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(g_titleLabel, Rgb(238, 244, 248), 0);
  lv_label_set_text(g_titleLabel, "BIKEMB LIVE");

  g_subtitleLabel = lv_label_create(g_screen);
  lv_obj_set_pos(g_subtitleLabel, 76, 322);
  lv_obj_set_style_text_font(g_subtitleLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(g_subtitleLabel, Rgb(118, 139, 156), 0);
  lv_label_set_text(g_subtitleLabel, "LVGL DEMO DASHBOARD");

  lv_obj_t *topLine = lv_obj_create(g_screen);
  ApplyBaseStyle(topLine, Rgb(38, 56, 74));
  lv_obj_set_size(topLine, 300, 2);
  lv_obj_set_pos(topLine, 30, 64);

  lv_obj_t *bottomLine = lv_obj_create(g_screen);
  ApplyBaseStyle(bottomLine, Rgb(38, 56, 74));
  lv_obj_set_size(bottomLine, 300, 2);
  lv_obj_set_pos(bottomLine, 30, 304);

  g_statsPanel = lv_obj_create(g_screen);
  lv_obj_clear_flag(g_statsPanel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(g_statsPanel, 276, 106);
  lv_obj_set_pos(g_statsPanel, 42, 82);
  lv_obj_set_style_bg_color(g_statsPanel, Rgb(16, 27, 42), 0);
  lv_obj_set_style_bg_opa(g_statsPanel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(g_statsPanel, 0, 0);
  lv_obj_set_style_radius(g_statsPanel, 18, 0);

  CreateStatRow(g_statsPanel, &g_cpuLabel, &g_cpuBar, 14, "CPU:  0%", Rgb(42, 210, 142));
  CreateStatRow(g_statsPanel, &g_memLabel, &g_memBar, 44, "MEM:  0%", Rgb(51, 190, 235));

  g_fpsLabel = lv_label_create(g_statsPanel);
  lv_obj_set_pos(g_fpsLabel, 18, 74);
  lv_obj_set_style_text_font(g_fpsLabel, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(g_fpsLabel, Rgb(238, 244, 248), 0);
  lv_label_set_text(g_fpsLabel, "FPS: 0.0");

  g_psramLabel = lv_label_create(g_statsPanel);
  lv_obj_set_pos(g_psramLabel, 142, 74);
  lv_obj_set_style_text_font(g_psramLabel, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(g_psramLabel, Rgb(118, 139, 156), 0);
  lv_label_set_text(g_psramLabel, "PS:  0%");

  g_psramBar = lv_bar_create(g_statsPanel);
  lv_obj_set_size(g_psramBar, 116, 10);
  lv_obj_set_pos(g_psramBar, 142, 90);
  lv_bar_set_range(g_psramBar, 0, 100);
  lv_bar_set_value(g_psramBar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(g_psramBar, Rgb(29, 43, 60), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(g_psramBar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(g_psramBar, 6, LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_psramBar, Rgb(255, 187, 68), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(g_psramBar, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(g_psramBar, 6, LV_PART_INDICATOR);

  g_orb = lv_obj_create(g_screen);
  lv_obj_set_size(g_orb, 48, 48);
  lv_obj_set_style_radius(g_orb, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(g_orb, Rgb(51, 190, 235), 0);
  lv_obj_set_style_bg_grad_color(g_orb, Rgb(42, 210, 142), 0);
  lv_obj_set_style_bg_grad_dir(g_orb, LV_GRAD_DIR_HOR, 0);
  lv_obj_set_style_border_width(g_orb, 2, 0);
  lv_obj_set_style_border_color(g_orb, Rgb(255, 255, 255), 0);
  lv_obj_set_style_shadow_width(g_orb, 18, 0);
  lv_obj_set_style_shadow_color(g_orb, Rgb(51, 190, 235), 0);
  lv_obj_set_style_shadow_opa(g_orb, LV_OPA_40, 0);

  lv_scr_load(g_screen);
}

void DashboardView_Update(const DemoMetrics &metrics) {
  char text[32];

  snprintf(text, sizeof(text), "CPU:%3d%%", static_cast<int>(metrics.cpuLoad + 0.5f));
  lv_label_set_text(g_cpuLabel, text);
  lv_bar_set_value(g_cpuBar, static_cast<int>(metrics.cpuLoad + 0.5f), LV_ANIM_OFF);

  const float heapLoad = metrics.heapTotal == 0
      ? 0.0f
      : 100.0f * (metrics.heapTotal - metrics.heapFree) / metrics.heapTotal;
  snprintf(text, sizeof(text), "MEM:%3d%%", static_cast<int>(heapLoad + 0.5f));
  lv_label_set_text(g_memLabel, text);
  lv_bar_set_value(g_memBar, static_cast<int>(heapLoad + 0.5f), LV_ANIM_OFF);

  snprintf(text, sizeof(text), "FPS:%4.1f", metrics.fps);
  lv_label_set_text(g_fpsLabel, text);

  const float psramLoad = metrics.psramTotal == 0
      ? 0.0f
      : 100.0f * (metrics.psramTotal - metrics.psramFree) / metrics.psramTotal;
  snprintf(text, sizeof(text), "PS:%3d%%", static_cast<int>(psramLoad + 0.5f));
  lv_label_set_text(g_psramLabel, text);
  lv_bar_set_value(g_psramBar, static_cast<int>(psramLoad + 0.5f), LV_ANIM_OFF);

  lv_obj_set_pos(g_orb, metrics.orbX, metrics.orbY);
}
