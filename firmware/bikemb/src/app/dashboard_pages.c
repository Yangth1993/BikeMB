#include "dashboard_pages.h"

#include <stdio.h>
#include "dashboard_ui_style.h"

static void set_label_text_if_changed(lv_obj_t *label, const char *text) {
  BikeMbUi_SetLabelTextIfChanged(label, text);
}

static void mode_click_event_cb(lv_event_t *event) {
  BikeMbDashboardPages *pages = (BikeMbDashboardPages *)lv_event_get_user_data(event);
  if (pages == NULL) {
    return;
  }

  static const char *modes[] = {"ECO", "TRAIL", "BOOST"};
  pages->home_mode_index = (uint8_t)((pages->home_mode_index + 1) % 3);
  set_label_text_if_changed(pages->home_mode, modes[pages->home_mode_index]);
  if (pages->mode_changed_callback != NULL) {
    pages->mode_changed_callback(pages->home_mode_index);
  }
}

static void create_page_dots(BikeMbDashboardPages *pages, lv_obj_t *page, uint8_t page_index) {
  for (uint8_t i = 0; i < BIKE_MB_DASHBOARD_PAGE_COUNT; ++i) {
    lv_obj_t *dot = lv_obj_create(page);
    BikeMbUi_StyleClear(dot);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_set_pos(dot, 166 + i * 18, 328);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(
        dot, i == page_index ? BikeMbUi_ColorCyan() : BikeMbUi_Rgb(71, 75, 82), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    pages->page_dots[page_index][i] = dot;
  }
}

static lv_obj_t *create_mode_chip(BikeMbDashboardPages *pages, lv_obj_t *parent) {
  lv_obj_t *chip = lv_obj_create(parent);
  BikeMbUi_StylePanel(chip, 18);
  lv_obj_set_size(chip, 86, 40);
  lv_obj_set_pos(chip, 137, 48);
  lv_obj_add_flag(chip, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_border_color(chip, BikeMbUi_ColorLime(), 0);
  pages->home_mode = BikeMbUi_MakeFixedLabel(
      chip, "ECO", &lv_font_montserrat_18, BikeMbUi_ColorLime(), 0, 10, 86, LV_TEXT_ALIGN_CENTER);
  lv_obj_add_event_cb(chip, mode_click_event_cb, LV_EVENT_CLICKED, pages);
  return chip;
}

static void create_home_page(BikeMbDashboardPages *pages, lv_obj_t *page) {
  pages->home_time = BikeMbUi_MakeFixedLabel(
      page, "10:42", &lv_font_montserrat_18, BikeMbUi_ColorText(), 40, 61, 74, LV_TEXT_ALIGN_LEFT);
  create_mode_chip(pages, page);
  pages->home_battery = BikeMbUi_MakeFixedLabel(
      page, "78%", &lv_font_montserrat_18, BikeMbUi_ColorText(), 246, 61, 72, LV_TEXT_ALIGN_RIGHT);

  pages->home_speed = BikeMbUi_MakeFixedLabel(
      page, "24.6", &lv_font_montserrat_48, BikeMbUi_ColorText(), 42, 117, 276, LV_TEXT_ALIGN_CENTER);
  lv_obj_set_style_text_letter_space(pages->home_speed, 1, 0);
  BikeMbUi_MakeFixedLabel(
      page, "km/h", &lv_font_montserrat_22, BikeMbUi_ColorMuted(), 130, 201, 100, LV_TEXT_ALIGN_CENTER);

  BikeMbUi_MakeFixedLabel(
      page, "TRIP", &lv_font_montserrat_14, BikeMbUi_ColorMuted(), 43, 253, 112, LV_TEXT_ALIGN_CENTER);
  pages->home_trip = BikeMbUi_MakeFixedLabel(
      page, "12.4", &lv_font_montserrat_22, BikeMbUi_ColorText(), 37, 279, 92, LV_TEXT_ALIGN_RIGHT);
  BikeMbUi_MakeLabel(page, "km", &lv_font_montserrat_14, BikeMbUi_ColorMuted(), 133, 290);

  lv_obj_t *divider = lv_obj_create(page);
  BikeMbUi_StyleClear(divider);
  lv_obj_set_size(divider, 1, 86);
  lv_obj_set_pos(divider, 180, 242);
  lv_obj_set_style_bg_color(divider, BikeMbUi_Rgb(64, 68, 74), 0);
  lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);

  BikeMbUi_MakeFixedLabel(
      page, "ASSIST", &lv_font_montserrat_14, BikeMbUi_ColorMuted(), 205, 253, 116, LV_TEXT_ALIGN_CENTER);
  pages->home_assist = BikeMbUi_MakeFixedLabel(
      page, "168", &lv_font_montserrat_22, BikeMbUi_ColorText(), 201, 279, 92, LV_TEXT_ALIGN_RIGHT);
  BikeMbUi_MakeLabel(page, "W", &lv_font_montserrat_14, BikeMbUi_ColorMuted(), 298, 290);

  create_page_dots(pages, page, 0);
}

static lv_obj_t *make_graph_label(lv_obj_t *parent, const char *text, lv_coord_t x, lv_coord_t y) {
  return BikeMbUi_MakeLabel(parent, text, &lv_font_montserrat_14, BikeMbUi_ColorMuted(), x, y);
}

static void create_grid_line(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
  lv_obj_t *line = lv_obj_create(parent);
  BikeMbUi_StyleClear(line);
  lv_obj_set_size(line, w, h);
  lv_obj_set_pos(line, x, y);
  lv_obj_set_style_bg_color(line, BikeMbUi_Rgb(55, 62, 68), 0);
  lv_obj_set_style_bg_opa(line, LV_OPA_50, 0);
}

static void create_wave_page(BikeMbDashboardPages *pages, lv_obj_t *page) {
  BikeMbUi_MakeLabel(page, "SPEED TREND", &lv_font_montserrat_22, BikeMbUi_ColorText(), 108, 39);
  BikeMbUi_MakeLabel(page, "KM/H", &lv_font_montserrat_14, BikeMbUi_ColorCyan(), 156, 68);

  lv_obj_t *graph = lv_obj_create(page);
  BikeMbUi_StylePanel(graph, 18);
  lv_obj_set_size(graph, 284, 184);
  lv_obj_set_pos(graph, 38, 90);

  for (uint8_t i = 0; i < 5; ++i) {
    create_grid_line(graph, 52, 28 + i * 34, 202, 1);
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

  pages->wave_line = lv_line_create(graph);
  lv_obj_set_style_line_color(pages->wave_line, BikeMbUi_ColorCyan(), 0);
  lv_obj_set_style_line_width(pages->wave_line, 3, 0);
  lv_obj_set_style_line_rounded(pages->wave_line, true, 0);
  lv_obj_set_pos(pages->wave_line, 0, 0);

  lv_obj_t *chip = lv_obj_create(graph);
  BikeMbUi_StylePanel(chip, 12);
  lv_obj_set_size(chip, 66, 38);
  lv_obj_set_pos(chip, 236, 60);
  lv_obj_set_style_bg_color(chip, BikeMbUi_ColorCyan(), 0);
  lv_obj_set_style_border_width(chip, 0, 0);
  pages->wave_value = BikeMbUi_MakeFixedLabel(
      chip, "28.6", &lv_font_montserrat_18, BikeMbUi_Rgb(2, 12, 18), 0, 10, 66, LV_TEXT_ALIGN_CENTER);

  lv_obj_t *status = lv_obj_create(page);
  BikeMbUi_StylePanel(status, 16);
  lv_obj_set_size(status, 258, 54);
  lv_obj_set_pos(status, 51, 284);
  pages->wave_direction = BikeMbUi_MakeFixedLabel(
      status, "NE", &lv_font_montserrat_18, BikeMbUi_ColorText(), 27, 17, 48, LV_TEXT_ALIGN_CENTER);
  pages->wave_temp = BikeMbUi_MakeFixedLabel(
      status, "18C", &lv_font_montserrat_18, BikeMbUi_ColorText(), 104, 17, 56, LV_TEXT_ALIGN_CENTER);
  pages->wave_grade = BikeMbUi_MakeFixedLabel(
      status, "+4%", &lv_font_montserrat_18, BikeMbUi_ColorText(), 188, 17, 58, LV_TEXT_ALIGN_CENTER);
  BikeMbUi_MakeLabel(status, ">", &lv_font_montserrat_22, BikeMbUi_ColorCyan(), 12, 14);
  BikeMbUi_MakeLabel(status, "|", &lv_font_montserrat_22, BikeMbUi_Rgb(68, 72, 78), 86, 14);
  BikeMbUi_MakeLabel(status, "|", &lv_font_montserrat_22, BikeMbUi_Rgb(68, 72, 78), 172, 14);

  create_page_dots(pages, page, 1);
}

static lv_obj_t *create_detail_row(lv_obj_t *parent,
                                   const char *icon,
                                   const char *label,
                                   lv_coord_t y,
                                   lv_color_t accent,
                                   lv_obj_t **value_out) {
  lv_obj_t *row = lv_obj_create(parent);
  BikeMbUi_StylePanel(row, 22);
  lv_obj_set_size(row, 278, 52);
  lv_obj_set_pos(row, 41, y);
  BikeMbUi_MakeLabel(row, icon, &lv_font_montserrat_22, accent, 23, 15);
  BikeMbUi_MakeFixedLabel(row, label, &lv_font_montserrat_12, BikeMbUi_ColorMuted(), 70, 19, 96, LV_TEXT_ALIGN_LEFT);
  *value_out = BikeMbUi_MakeFixedLabel(
      row, "0", &lv_font_montserrat_18, BikeMbUi_ColorText(), 154, 17, 108, LV_TEXT_ALIGN_RIGHT);
  return row;
}

static void create_details_page(BikeMbDashboardPages *pages, lv_obj_t *page) {
  BikeMbUi_MakeLabel(page, "RIDE DETAILS", &lv_font_montserrat_22, BikeMbUi_ColorText(), 105, 46);

  lv_obj_t *title_line = lv_obj_create(page);
  BikeMbUi_StyleClear(title_line);
  lv_obj_set_size(title_line, 46, 3);
  lv_obj_set_pos(title_line, 157, 78);
  lv_obj_set_style_bg_color(title_line, BikeMbUi_ColorCyan(), 0);
  lv_obj_set_style_bg_opa(title_line, LV_OPA_COVER, 0);

  create_detail_row(page, "~", "TRIP DISTANCE", 92, BikeMbUi_ColorCyan(), &pages->detail_trip);
  create_detail_row(page, "o", "RIDE TIME", 153, BikeMbUi_ColorCyan(), &pages->detail_time);
  create_detail_row(page, "/|", "TOTAL DISTANCE", 214, BikeMbUi_ColorLime(), &pages->detail_total);
  create_detail_row(page, ">", "AVG SPEED", 275, BikeMbUi_ColorCyan(), &pages->detail_average);
  create_page_dots(pages, page, 2);
}

static void update_wave(BikeMbDashboardPages *pages, uint8_t phase) {
  static const uint8_t shape[BIKE_MB_DASHBOARD_WAVE_POINT_COUNT] = {
      12, 18, 22, 21, 24, 18, 17, 16,
      20, 23, 22, 18, 21, 17, 16, 19,
      24, 28, 31, 26, 23, 18, 17, 20,
  };

  for (uint8_t i = 0; i < BIKE_MB_DASHBOARD_WAVE_POINT_COUNT; ++i) {
    const uint8_t value = shape[(i + phase) % BIKE_MB_DASHBOARD_WAVE_POINT_COUNT];
    pages->wave_points[i].x = 52 + i * 9;
    pages->wave_points[i].y = 160 - value * 3;
  }

  lv_line_set_points(pages->wave_line, pages->wave_points, BIKE_MB_DASHBOARD_WAVE_POINT_COUNT);
}

static void format_time(char *buffer, size_t size, uint32_t seconds) {
  const uint32_t hours = (seconds / 3600) % 100;
  const uint32_t minutes = (seconds / 60) % 60;
  snprintf(buffer, size, "%02lu:%02lu", (unsigned long)hours, (unsigned long)minutes);
}

void BikeMbDashboardPages_Create(BikeMbDashboardPages *pages, lv_obj_t *page_objs[BIKE_MB_DASHBOARD_PAGE_COUNT]) {
  if (pages == NULL || page_objs == NULL) {
    return;
  }

  pages->home_mode_index = 0;
  pages->mode_changed_callback = NULL;
  create_home_page(pages, page_objs[0]);
  create_wave_page(pages, page_objs[1]);
  create_details_page(pages, page_objs[2]);
}

void BikeMbDashboardPages_SetModeChangedCallback(BikeMbDashboardPages *pages,
                                                 BikeMbDashboardModeChangedCallback callback) {
  if (pages == NULL) {
    return;
  }
  pages->mode_changed_callback = callback;
}

void BikeMbDashboardPages_Update(BikeMbDashboardPages *pages, const BikeMbDashboardMetrics *metrics) {
  if (pages == NULL || metrics == NULL) {
    return;
  }

  char text[32];

  snprintf(text, sizeof(text), "%.1f", (double)metrics->speedKmh);
  set_label_text_if_changed(pages->home_speed, text);

  snprintf(text, sizeof(text), "%u%%", metrics->batteryPercent);
  set_label_text_if_changed(pages->home_battery, text);

  format_time(text, sizeof(text), metrics->uptimeMs / 1000);
  set_label_text_if_changed(pages->home_time, text);

  snprintf(text, sizeof(text), "%.1f", (double)metrics->tripKm);
  set_label_text_if_changed(pages->home_trip, text);

  snprintf(text, sizeof(text), "%u", (unsigned int)(metrics->assistPowerW + 0.5f));
  set_label_text_if_changed(pages->home_assist, text);

  snprintf(text, sizeof(text), "%.1f", (double)metrics->speedKmh + 4.0);
  set_label_text_if_changed(pages->wave_value, text);

  set_label_text_if_changed(pages->wave_direction, "NE");

  snprintf(text, sizeof(text), "%uC", (unsigned int)(metrics->temperatureC + 0.5f));
  set_label_text_if_changed(pages->wave_temp, text);

  snprintf(text, sizeof(text), "+%u%%", (unsigned int)(metrics->gradePercent + 0.5f));
  set_label_text_if_changed(pages->wave_grade, text);

  snprintf(text, sizeof(text), "%.1f km", (double)metrics->tripKm);
  set_label_text_if_changed(pages->detail_trip, text);

  format_time(text, sizeof(text), metrics->rideSeconds);
  set_label_text_if_changed(pages->detail_time, text);

  snprintf(text, sizeof(text), "%u km", (unsigned int)(metrics->totalKm + 0.5f));
  set_label_text_if_changed(pages->detail_total, text);

  snprintf(text, sizeof(text), "%.1f km/h", (double)metrics->averageSpeedKmh);
  set_label_text_if_changed(pages->detail_average, text);

  update_wave(pages, metrics->wavePhase);
}
