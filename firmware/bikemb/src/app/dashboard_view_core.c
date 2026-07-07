#include "dashboard_view_core.h"

#include <stdio.h>
#include <string.h>
#include "lvgl.h"

enum {
  kScreenSize = 360,
  kOrbSize = 48,
};

static lv_obj_t *g_cpu_label;
static lv_obj_t *g_mem_label;
static lv_obj_t *g_fps_label;
static lv_obj_t *g_psram_label;
static lv_obj_t *g_cpu_bar;
static lv_obj_t *g_mem_bar;
static lv_obj_t *g_psram_bar;
static lv_obj_t *g_orb;

static lv_color_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return lv_color_make(r, g, b);
}

static void apply_base_style(lv_obj_t *obj, lv_color_t bg) {
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(obj, bg, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_radius(obj, 18, 0);
}

static void set_label_text_if_changed(lv_obj_t *label, const char *text) {
  const char *current = lv_label_get_text(label);
  if (current == NULL || strcmp(current, text) != 0) {
    lv_label_set_text(label, text);
  }
}

static void set_bar_value_if_changed(lv_obj_t *bar, int32_t value) {
  if (lv_bar_get_value(bar) != value) {
    lv_bar_set_value(bar, value, LV_ANIM_OFF);
  }
}

static void create_stat_row(lv_obj_t *parent,
                            lv_obj_t **label_out,
                            lv_obj_t **bar_out,
                            lv_coord_t y,
                            const char *initial_text,
                            lv_color_t bar_color) {
  *bar_out = lv_bar_create(parent);
  lv_obj_set_size(*bar_out, 116, 14);
  lv_obj_set_pos(*bar_out, 148, y + 2);
  lv_bar_set_range(*bar_out, 0, 100);
  lv_bar_set_value(*bar_out, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(*bar_out, rgb(29, 43, 60), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(*bar_out, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(*bar_out, 6, LV_PART_MAIN);
  lv_obj_set_style_bg_color(*bar_out, bar_color, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(*bar_out, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(*bar_out, 6, LV_PART_INDICATOR);

  *label_out = lv_label_create(parent);
  lv_obj_set_pos(*label_out, 18, y);
  lv_obj_set_style_text_font(*label_out, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(*label_out, rgb(238, 244, 248), 0);
  lv_label_set_text(*label_out, initial_text);
  lv_obj_move_foreground(*label_out);
}

void BikeMbDashboardView_Create(void) {
  lv_obj_t *screen = lv_obj_create(NULL);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(screen, rgb(3, 7, 13), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(screen, 0, 0);
  lv_obj_set_style_radius(screen, 0, 0);
  lv_obj_set_style_pad_all(screen, 0, 0);

  lv_obj_t *dial = lv_obj_create(screen);
  lv_obj_clear_flag(dial, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(dial, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(dial, kScreenSize, kScreenSize);
  lv_obj_center(dial);
  lv_obj_set_style_bg_color(dial, rgb(7, 13, 24), 0);
  lv_obj_set_style_bg_opa(dial, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(dial, 2, 0);
  lv_obj_set_style_border_color(dial, rgb(38, 56, 74), 0);
  lv_obj_set_style_radius(dial, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_pad_all(dial, 0, 0);

  lv_obj_t *content = lv_obj_create(screen);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(content, kScreenSize, kScreenSize);
  lv_obj_center(content);
  lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(content, 0, 0);
  lv_obj_set_style_radius(content, 0, 0);
  lv_obj_set_style_pad_all(content, 0, 0);

  lv_obj_t *title = lv_label_create(content);
  lv_obj_set_pos(title, 80, 24);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(title, rgb(238, 244, 248), 0);
  lv_label_set_text(title, "BIKEMB LIVE");

  lv_obj_t *subtitle = lv_label_create(content);
  lv_obj_set_pos(subtitle, 76, 322);
  lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(subtitle, rgb(118, 139, 156), 0);
  lv_label_set_text(subtitle, "LVGL DEMO DASHBOARD");

  lv_obj_t *top_line = lv_obj_create(content);
  apply_base_style(top_line, rgb(38, 56, 74));
  lv_obj_set_size(top_line, 300, 2);
  lv_obj_set_pos(top_line, 30, 64);

  lv_obj_t *bottom_line = lv_obj_create(content);
  apply_base_style(bottom_line, rgb(38, 56, 74));
  lv_obj_set_size(bottom_line, 300, 2);
  lv_obj_set_pos(bottom_line, 30, 304);

  lv_obj_t *stats_panel = lv_obj_create(content);
  lv_obj_clear_flag(stats_panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(stats_panel, 276, 106);
  lv_obj_set_pos(stats_panel, 42, 82);
  lv_obj_set_style_bg_color(stats_panel, rgb(16, 27, 42), 0);
  lv_obj_set_style_bg_opa(stats_panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(stats_panel, 0, 0);
  lv_obj_set_style_radius(stats_panel, 18, 0);
  lv_obj_set_style_pad_all(stats_panel, 0, 0);

  create_stat_row(stats_panel, &g_cpu_label, &g_cpu_bar, 14, "CPU:  0%", rgb(42, 210, 142));
  create_stat_row(stats_panel, &g_mem_label, &g_mem_bar, 44, "MEM:  0%", rgb(51, 190, 235));

  g_fps_label = lv_label_create(stats_panel);
  lv_obj_set_pos(g_fps_label, 18, 74);
  lv_obj_set_style_text_font(g_fps_label, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(g_fps_label, rgb(238, 244, 248), 0);
  lv_label_set_text(g_fps_label, "FPS: 0.0");

  g_psram_label = lv_label_create(stats_panel);
  lv_obj_set_pos(g_psram_label, 142, 74);
  lv_obj_set_style_text_font(g_psram_label, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(g_psram_label, rgb(118, 139, 156), 0);
  lv_label_set_text(g_psram_label, "PS:  0%");

  g_psram_bar = lv_bar_create(stats_panel);
  lv_obj_set_size(g_psram_bar, 116, 10);
  lv_obj_set_pos(g_psram_bar, 142, 90);
  lv_bar_set_range(g_psram_bar, 0, 100);
  lv_bar_set_value(g_psram_bar, 0, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(g_psram_bar, rgb(29, 43, 60), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(g_psram_bar, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(g_psram_bar, 6, LV_PART_MAIN);
  lv_obj_set_style_bg_color(g_psram_bar, rgb(255, 187, 68), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(g_psram_bar, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(g_psram_bar, 6, LV_PART_INDICATOR);
  lv_obj_move_foreground(g_psram_label);

  g_orb = lv_obj_create(content);
  lv_obj_set_size(g_orb, kOrbSize, kOrbSize);
  lv_obj_set_style_radius(g_orb, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(g_orb, rgb(51, 190, 235), 0);
  lv_obj_set_style_bg_grad_color(g_orb, rgb(42, 210, 142), 0);
  lv_obj_set_style_bg_grad_dir(g_orb, LV_GRAD_DIR_HOR, 0);
  lv_obj_set_style_border_width(g_orb, 2, 0);
  lv_obj_set_style_border_color(g_orb, rgb(255, 255, 255), 0);
  lv_obj_set_style_shadow_width(g_orb, 18, 0);
  lv_obj_set_style_shadow_color(g_orb, rgb(51, 190, 235), 0);
  lv_obj_set_style_shadow_opa(g_orb, LV_OPA_40, 0);

  lv_scr_load(screen);
}

void BikeMbDashboardView_Update(const BikeMbDashboardMetrics *metrics) {
  char text[32];
  const int32_t cpu_load = (int32_t)(metrics->cpuLoad + 0.5f);

  snprintf(text, sizeof(text), "CPU:%3d%%", (int)cpu_load);
  set_label_text_if_changed(g_cpu_label, text);
  set_bar_value_if_changed(g_cpu_bar, cpu_load);

  const float heap_load = metrics->heapTotal == 0
      ? 0.0f
      : 100.0f * (metrics->heapTotal - metrics->heapFree) / metrics->heapTotal;
  const int32_t mem_load = (int32_t)(heap_load + 0.5f);
  snprintf(text, sizeof(text), "MEM:%3d%%", (int)mem_load);
  set_label_text_if_changed(g_mem_label, text);
  set_bar_value_if_changed(g_mem_bar, mem_load);

  snprintf(text, sizeof(text), "FPS:%4.1f", (double)metrics->fps);
  set_label_text_if_changed(g_fps_label, text);

  const float psram_load = metrics->psramTotal == 0
      ? 0.0f
      : 100.0f * (metrics->psramTotal - metrics->psramFree) / metrics->psramTotal;
  const int32_t psram_load_int = (int32_t)(psram_load + 0.5f);
  snprintf(text, sizeof(text), "PS:%3d%%", (int)psram_load_int);
  set_label_text_if_changed(g_psram_label, text);
  set_bar_value_if_changed(g_psram_bar, psram_load_int);

  lv_obj_set_pos(g_orb, metrics->orbX, metrics->orbY);
}
