#include "dashboard_view_core.h"

#include <stdio.h>
#include <string.h>
#include "../drivers/Touch_CST816.h"
#include "lvgl.h"

enum {
  kScreenSize = 360,
  kPageCount = 3,
  kWavePointCount = 24,
};

static lv_obj_t *g_pages[kPageCount];
static lv_obj_t *g_home_speed;
static lv_obj_t *g_home_trip;
static lv_obj_t *g_home_assist;
static lv_obj_t *g_home_battery;
static lv_obj_t *g_home_time;
static lv_obj_t *g_home_mode;
static lv_obj_t *g_wave_value;
static lv_obj_t *g_wave_line;
static lv_point_t g_wave_points[kWavePointCount];
static lv_obj_t *g_wave_direction;
static lv_obj_t *g_wave_temp;
static lv_obj_t *g_wave_grade;
static lv_obj_t *g_detail_trip;
static lv_obj_t *g_detail_time;
static lv_obj_t *g_detail_total;
static lv_obj_t *g_detail_average;
static lv_obj_t *g_page_dots[kPageCount][kPageCount];
static uint8_t g_visible_page = 0;
static uint8_t g_home_mode_index = 0;

static void update_visible_page(uint8_t page);

static lv_color_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return lv_color_make(r, g, b);
}

static lv_color_t color_bg(void) {
  return rgb(3, 5, 8);
}

static lv_color_t color_panel(void) {
  return rgb(23, 25, 29);
}

static lv_color_t color_cyan(void) {
  return rgb(24, 222, 239);
}

static lv_color_t color_lime(void) {
  return rgb(178, 245, 38);
}

static lv_color_t color_text(void) {
  return rgb(246, 248, 250);
}

static lv_color_t color_muted(void) {
  return rgb(143, 149, 157);
}

static void style_clear(lv_obj_t *obj) {
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
}

static void style_page(lv_obj_t *obj) {
  style_clear(obj);
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(obj, kScreenSize, kScreenSize);
  lv_obj_center(obj);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
}

static void style_panel(lv_obj_t *obj, lv_coord_t radius) {
  style_clear(obj);
  lv_obj_set_style_bg_color(obj, color_panel(), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_border_color(obj, rgb(58, 62, 68), 0);
  lv_obj_set_style_radius(obj, radius, 0);
}

static lv_obj_t *make_label(lv_obj_t *parent,
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

static lv_obj_t *make_fixed_label(lv_obj_t *parent,
                                  const char *text,
                                  const lv_font_t *font,
                                  lv_color_t color,
                                  lv_coord_t x,
                                  lv_coord_t y,
                                  lv_coord_t width,
                                  lv_text_align_t align) {
  lv_obj_t *label = make_label(parent, text, font, color, x, y);
  lv_obj_set_width(label, width);
  lv_obj_set_style_text_align(label, align, 0);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  return label;
}

static void set_label_text_if_changed(lv_obj_t *label, const char *text) {
  const char *current = lv_label_get_text(label);
  if (current == NULL || strcmp(current, text) != 0) {
    lv_label_set_text(label, text);
  }
}

static void mode_click_event_cb(lv_event_t *event) {
  (void)event;

  static const char *modes[] = {"ECO", "TRAIL", "BOOST"};
  g_home_mode_index = (uint8_t)((g_home_mode_index + 1) % 3);
  set_label_text_if_changed(g_home_mode, modes[g_home_mode_index]);
}

static void create_outer_ring(lv_obj_t *screen) {
  lv_obj_t *ring = lv_obj_create(screen);
  style_clear(ring);
  lv_obj_set_size(ring, kScreenSize, kScreenSize);
  lv_obj_center(ring);
  lv_obj_set_style_bg_color(ring, color_bg(), 0);
  lv_obj_set_style_bg_opa(ring, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(ring, 2, 0);
  lv_obj_set_style_border_color(ring, rgb(38, 41, 46), 0);
  lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
}

static lv_obj_t *create_page(lv_obj_t *screen, uint8_t index) {
  lv_obj_t *page = lv_obj_create(screen);
  style_page(page);
  if (index != 0) {
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
  }
  g_pages[index] = page;
  return page;
}

static void create_page_dots(lv_obj_t *page, uint8_t page_index) {
  for (uint8_t i = 0; i < kPageCount; ++i) {
    lv_obj_t *dot = lv_obj_create(page);
    style_clear(dot);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_pos(dot, 166 + i * 18, 328);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, i == page_index ? color_cyan() : rgb(71, 75, 82), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    g_page_dots[page_index][i] = dot;
  }
}

static lv_obj_t *create_chip(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y) {
  lv_obj_t *chip = lv_obj_create(parent);
  style_panel(chip, 18);
  lv_obj_set_size(chip, 86, 40);
  lv_obj_set_pos(chip, x, y);
  lv_obj_add_flag(chip, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_color(chip, color_lime(), 0);
  g_home_mode = make_fixed_label(chip, text, &lv_font_montserrat_18, color_lime(), 0, 10, 86, LV_TEXT_ALIGN_CENTER);
  lv_obj_add_event_cb(chip, mode_click_event_cb, LV_EVENT_CLICKED, NULL);
  return chip;
}

static void create_home_page(lv_obj_t *page) {
  g_home_time = make_fixed_label(page, "10:42", &lv_font_montserrat_18, color_text(), 40, 61, 74, LV_TEXT_ALIGN_LEFT);
  create_chip(page, "ECO", 137, 48);
  g_home_battery = make_fixed_label(page, "78%", &lv_font_montserrat_18, color_text(), 246, 61, 72, LV_TEXT_ALIGN_RIGHT);

  g_home_speed = make_fixed_label(page, "24.6", &lv_font_montserrat_48, color_text(), 42, 117, 276, LV_TEXT_ALIGN_CENTER);
  lv_obj_set_style_text_letter_space(g_home_speed, 1, 0);
  make_fixed_label(page, "km/h", &lv_font_montserrat_22, color_muted(), 130, 201, 100, LV_TEXT_ALIGN_CENTER);

  make_fixed_label(page, "TRIP", &lv_font_montserrat_14, color_muted(), 43, 253, 112, LV_TEXT_ALIGN_CENTER);
  g_home_trip = make_fixed_label(page, "12.4", &lv_font_montserrat_22, color_text(), 37, 279, 92, LV_TEXT_ALIGN_RIGHT);
  make_label(page, "km", &lv_font_montserrat_14, color_muted(), 133, 290);

  lv_obj_t *divider = lv_obj_create(page);
  style_clear(divider);
  lv_obj_set_size(divider, 1, 86);
  lv_obj_set_pos(divider, 180, 242);
  lv_obj_set_style_bg_color(divider, rgb(64, 68, 74), 0);
  lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);

  make_fixed_label(page, "ASSIST", &lv_font_montserrat_14, color_muted(), 205, 253, 116, LV_TEXT_ALIGN_CENTER);
  g_home_assist = make_fixed_label(page, "168", &lv_font_montserrat_22, color_text(), 201, 279, 92, LV_TEXT_ALIGN_RIGHT);
  make_label(page, "W", &lv_font_montserrat_14, color_muted(), 298, 290);
}

static lv_obj_t *make_graph_label(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y) {
  return make_label(parent, text, &lv_font_montserrat_14, color_muted(), x, y);
}

static void create_grid_line(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
  lv_obj_t *line = lv_obj_create(parent);
  style_clear(line);
  lv_obj_set_size(line, w, h);
  lv_obj_set_pos(line, x, y);
  lv_obj_set_style_bg_color(line, rgb(55, 62, 68), 0);
  lv_obj_set_style_bg_opa(line, LV_OPA_50, 0);
}

static void create_wave_page(lv_obj_t *page) {
  make_label(page, "SPEED TREND", &lv_font_montserrat_22, color_text(), 108, 39);
  make_label(page, "KM/H", &lv_font_montserrat_14, color_cyan(), 156, 68);

  lv_obj_t *graph = lv_obj_create(page);
  style_panel(graph, 18);
  lv_obj_set_size(graph, 284, 184);
  lv_obj_set_pos(graph, 38, 90);

  for (uint8_t i = 0; i < 5; ++i) {
    create_grid_line(graph, 52, 28 + i * 34, 202, 1);
  }
  for (uint8_t i = 0; i < 5; ++i) {
    create_grid_line(graph, 52 + i * 50, 24, 1, 138);
  }

  make_graph_label(graph, "40", 14, 16);
  make_graph_label(graph, "30", 14, 50);
  make_graph_label(graph, "20", 14, 84);
  make_graph_label(graph, "10", 14, 118);
  make_graph_label(graph, "0", 24, 150);
  make_graph_label(graph, "-5 MIN", 37, 158);
  make_graph_label(graph, "-3", 93, 158);
  make_graph_label(graph, "-2", 142, 158);
  make_graph_label(graph, "-1", 194, 158);
  make_graph_label(graph, "NOW", 232, 158);

  g_wave_line = lv_line_create(graph);
  lv_obj_set_style_line_color(g_wave_line, color_cyan(), 0);
  lv_obj_set_style_line_width(g_wave_line, 3, 0);
  lv_obj_set_style_line_rounded(g_wave_line, true, 0);
  lv_obj_set_pos(g_wave_line, 0, 0);

  lv_obj_t *chip = lv_obj_create(graph);
  style_panel(chip, 12);
  lv_obj_set_size(chip, 66, 38);
  lv_obj_set_pos(chip, 236, 60);
  lv_obj_set_style_bg_color(chip, color_cyan(), 0);
  lv_obj_set_style_border_width(chip, 0, 0);
  g_wave_value = make_fixed_label(chip, "28.6", &lv_font_montserrat_18, rgb(2, 12, 18), 0, 10, 66, LV_TEXT_ALIGN_CENTER);

  lv_obj_t *status = lv_obj_create(page);
  style_panel(status, 16);
  lv_obj_set_size(status, 258, 54);
  lv_obj_set_pos(status, 51, 284);
  g_wave_direction = make_fixed_label(status, "NE", &lv_font_montserrat_18, color_text(), 27, 17, 48, LV_TEXT_ALIGN_CENTER);
  g_wave_temp = make_fixed_label(status, "18C", &lv_font_montserrat_18, color_text(), 104, 17, 56, LV_TEXT_ALIGN_CENTER);
  g_wave_grade = make_fixed_label(status, "+4%", &lv_font_montserrat_18, color_text(), 188, 17, 58, LV_TEXT_ALIGN_CENTER);
  make_label(status, ">", &lv_font_montserrat_22, color_cyan(), 12, 14);
  make_label(status, "|", &lv_font_montserrat_22, rgb(68, 72, 78), 86, 14);
  make_label(status, "|", &lv_font_montserrat_22, rgb(68, 72, 78), 172, 14);

  create_page_dots(page, 1);
}

static lv_obj_t *create_detail_row(lv_obj_t *parent,
                                   const char *icon,
                                   const char *label,
                                   lv_coord_t y,
                                   lv_color_t accent,
                                   lv_obj_t **value_out) {
  lv_obj_t *row = lv_obj_create(parent);
  style_panel(row, 22);
  lv_obj_set_size(row, 278, 52);
  lv_obj_set_pos(row, 41, y);
  make_label(row, icon, &lv_font_montserrat_22, accent, 23, 15);
  make_fixed_label(row, label, &lv_font_montserrat_12, color_muted(), 70, 19, 96, LV_TEXT_ALIGN_LEFT);
  *value_out = make_fixed_label(row, "0", &lv_font_montserrat_18, color_text(), 154, 17, 108, LV_TEXT_ALIGN_RIGHT);
  return row;
}

static void create_details_page(lv_obj_t *page) {
  make_label(page, "RIDE DETAILS", &lv_font_montserrat_22, color_text(), 105, 46);

  lv_obj_t *title_line = lv_obj_create(page);
  style_clear(title_line);
  lv_obj_set_size(title_line, 46, 3);
  lv_obj_set_pos(title_line, 157, 78);
  lv_obj_set_style_bg_color(title_line, color_cyan(), 0);
  lv_obj_set_style_bg_opa(title_line, LV_OPA_COVER, 0);

  create_detail_row(page, "~", "TRIP DISTANCE", 92, color_cyan(), &g_detail_trip);
  create_detail_row(page, "o", "RIDE TIME", 153, color_cyan(), &g_detail_time);
  create_detail_row(page, "/|", "TOTAL DISTANCE", 214, color_lime(), &g_detail_total);
  create_detail_row(page, ">", "AVG SPEED", 275, color_cyan(), &g_detail_average);
}

static void update_visible_page(uint8_t page) {
  page %= kPageCount;
  if (page == g_visible_page) {
    return;
  }

  for (uint8_t i = 0; i < kPageCount; ++i) {
    if (i == page) {
      lv_obj_clear_flag(g_pages[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(g_pages[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
  g_visible_page = page;
}

static void update_wave(uint8_t phase) {
  static const uint8_t shape[kWavePointCount] = {
      12, 18, 22, 21, 24, 18, 17, 16,
      20, 23, 22, 18, 21, 17, 16, 19,
      24, 28, 31, 26, 23, 18, 17, 20,
  };

  for (uint8_t i = 0; i < kWavePointCount; ++i) {
    const uint8_t value = shape[(i + phase) % kWavePointCount];
    g_wave_points[i].x = 52 + i * 9;
    g_wave_points[i].y = 160 - value * 3;
  }

  lv_line_set_points(g_wave_line, g_wave_points, kWavePointCount);
}

static void format_time(char *buffer, size_t size, uint32_t seconds) {
  const uint32_t hours = (seconds / 3600) % 100;
  const uint32_t minutes = (seconds / 60) % 60;
  snprintf(buffer, size, "%02lu:%02lu", (unsigned long)hours, (unsigned long)minutes);
}

void BikeMbDashboardView_Create(void) {
  lv_obj_t *screen = lv_obj_create(NULL);
  style_clear(screen);
  lv_obj_set_style_bg_color(screen, color_bg(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(screen, 0, 0);

  create_outer_ring(screen);

  lv_obj_t *home = create_page(screen, 0);
  lv_obj_t *wave = create_page(screen, 1);
  lv_obj_t *details = create_page(screen, 2);

  create_home_page(home);
  create_wave_page(wave);
  create_details_page(details);
  create_page_dots(home, 0);

  lv_scr_load(screen);
}

void BikeMbDashboardView_Update(const BikeMbDashboardMetrics *metrics) {
  char text[32];

  snprintf(text, sizeof(text), "%.1f", (double)metrics->speedKmh);
  set_label_text_if_changed(g_home_speed, text);

  snprintf(text, sizeof(text), "%u%%", metrics->batteryPercent);
  set_label_text_if_changed(g_home_battery, text);

  format_time(text, sizeof(text), metrics->uptimeMs / 1000);
  set_label_text_if_changed(g_home_time, text);

  snprintf(text, sizeof(text), "%.1f", (double)metrics->tripKm);
  set_label_text_if_changed(g_home_trip, text);

  snprintf(text, sizeof(text), "%u", (unsigned int)(metrics->assistPowerW + 0.5f));
  set_label_text_if_changed(g_home_assist, text);

  snprintf(text, sizeof(text), "%.1f", (double)metrics->speedKmh + 4.0);
  set_label_text_if_changed(g_wave_value, text);

  set_label_text_if_changed(g_wave_direction, "NE");

  snprintf(text, sizeof(text), "%uC", (unsigned int)(metrics->temperatureC + 0.5f));
  set_label_text_if_changed(g_wave_temp, text);

  snprintf(text, sizeof(text), "+%u%%", (unsigned int)(metrics->gradePercent + 0.5f));
  set_label_text_if_changed(g_wave_grade, text);

  snprintf(text, sizeof(text), "%.1f km", (double)metrics->tripKm);
  set_label_text_if_changed(g_detail_trip, text);

  format_time(text, sizeof(text), metrics->rideSeconds);
  set_label_text_if_changed(g_detail_time, text);

  snprintf(text, sizeof(text), "%u km", (unsigned int)(metrics->totalKm + 0.5f));
  set_label_text_if_changed(g_detail_total, text);

  snprintf(text, sizeof(text), "%.1f km/h", (double)metrics->averageSpeedKmh);
  set_label_text_if_changed(g_detail_average, text);

  update_wave(metrics->wavePhase);

  const Cst816Gesture gesture = TouchCst816_ConsumeGesture();
  if (gesture == CST816_GESTURE_SWIPE_LEFT) {
    update_visible_page((uint8_t)((g_visible_page + 1) % kPageCount));
  } else if (gesture == CST816_GESTURE_SWIPE_RIGHT) {
    update_visible_page((uint8_t)((g_visible_page + kPageCount - 1) % kPageCount));
  }
}
