#include "dashboard_pages.h"

#include <stdio.h>
#include "assets/dashboard_assets.h"
#include "dashboard_ui_style.h"

#ifndef BIKE_MB_FIRMWARE_VERSION
#define BIKE_MB_FIRMWARE_VERSION "v0.1.0"
#endif

enum {
  kViewDashboard = 0,
  kViewSettings = 1,
  kViewAccessories = 2,
  kViewAbout = 3,
};

static const char *kModeLabels[BIKE_MB_DASHBOARD_MODE_COUNT] = {
    "ECO",
    "TRAIL",
    "AUTO",
    "BOOST",
};

static void set_label_text_if_changed(lv_obj_t *label, const char *text) {
  BikeMbUi_SetLabelTextIfChanged(label, text);
}

static void hide_obj(lv_obj_t *obj) {
  if (obj != NULL) {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
}

static void show_obj(lv_obj_t *obj) {
  if (obj != NULL) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
}

static void update_page_dots(BikeMbDashboardPages *pages) {
  const lv_color_t accent = BikeMbUi_ModeColor(pages->home_mode_index);
  for (uint8_t page = 0; page < BIKE_MB_DASHBOARD_PAGE_COUNT; ++page) {
    for (uint8_t i = 0; i < BIKE_MB_DASHBOARD_PAGE_COUNT; ++i) {
      if (pages->page_dots[page][i] != NULL) {
        lv_obj_set_style_bg_color(
            pages->page_dots[page][i], i == page ? accent : BikeMbUi_Rgb(71, 75, 82), 0);
      }
    }
  }
}

static void update_mode_accents(BikeMbDashboardPages *pages) {
  const lv_color_t accent = BikeMbUi_ModeColor(pages->home_mode_index);

  if (pages->home_mode != NULL) {
    set_label_text_if_changed(pages->home_mode, kModeLabels[pages->home_mode_index]);
    lv_obj_set_style_text_color(pages->home_mode, accent, 0);
  }
  if (pages->home_mode_chip != NULL) {
    lv_obj_set_style_border_color(pages->home_mode_chip, accent, 0);
  }
  if (pages->home_unit != NULL) {
    lv_obj_set_style_text_color(pages->home_unit, BikeMbUi_ColorMuted(), 0);
  }
  if (pages->home_assist_glow != NULL) {
    lv_obj_set_style_img_recolor(pages->home_assist_glow, accent, 0);
    lv_obj_set_style_img_recolor_opa(pages->home_assist_glow, LV_OPA_COVER, 0);
  }
  if (pages->wave_line != NULL) {
    lv_obj_set_style_line_color(pages->wave_line, accent, 0);
  }
  if (pages->ai_identity != NULL) {
    lv_obj_set_style_text_color(pages->ai_identity, accent, 0);
  }
  if (pages->wave_value_unit != NULL) {
    lv_obj_set_style_text_color(pages->wave_value_unit, BikeMbUi_ColorMuted(), 0);
  }
  for (uint8_t i = 0; i < 4; ++i) {
    if (pages->detail_icons[i] != NULL) {
      lv_obj_set_style_text_color(pages->detail_icons[i], accent, 0);
    }
  }

  update_page_dots(pages);
}

static void mode_click_event_cb(lv_event_t *event) {
  BikeMbDashboardPages *pages = (BikeMbDashboardPages *)lv_event_get_user_data(event);
  if (pages == NULL) {
    return;
  }

  pages->home_mode_index = (uint8_t)((pages->home_mode_index + 1) % BIKE_MB_DASHBOARD_MODE_COUNT);
  update_mode_accents(pages);
  if (pages->mode_changed_callback != NULL) {
    pages->mode_changed_callback(pages->home_mode_index);
  }
}

static void create_page_dots(BikeMbDashboardPages *pages, lv_obj_t *page, uint8_t page_index) {
  for (uint8_t i = 0; i < BIKE_MB_DASHBOARD_PAGE_COUNT; ++i) {
    lv_obj_t *dot = lv_obj_create(page);
    BikeMbUi_StyleClear(dot);
    lv_obj_set_size(dot, 9, 9);
    lv_obj_set_pos(dot, 164 + i * 18, 340);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    pages->page_dots[page_index][i] = dot;
  }
}

static lv_obj_t *create_mode_chip(BikeMbDashboardPages *pages, lv_obj_t *parent) {
  lv_obj_t *chip = lv_obj_create(parent);
  BikeMbUi_StyleClear(chip);
  lv_obj_set_size(chip, 104, 38);
  lv_obj_set_pos(chip, 72, 54);
  lv_obj_add_flag(chip, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(chip, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(chip, 0, 0);
  lv_obj_set_style_radius(chip, 14, 0);
  pages->home_mode = BikeMbUi_MakeFixedLabel(
      chip, "AUTO", &lv_font_montserrat_22, BikeMbUi_ColorAuto(), 0, 4, 104, LV_TEXT_ALIGN_CENTER);
  lv_obj_add_event_cb(chip, mode_click_event_cb, LV_EVENT_CLICKED, pages);
  pages->home_mode_chip = chip;
  return chip;
}

static void update_home_speed(BikeMbDashboardPages *pages, float speed_kmh) {
  if (pages == NULL) {
    return;
  }

  char major[12];
  char decimal[4];
  int32_t speed_tenths = (int32_t)(speed_kmh * 10.0f + 0.5f);
  if (speed_tenths < 0) {
    speed_tenths = 0;
  }

  snprintf(major, sizeof(major), "%ld", (long)(speed_tenths / 10));
  snprintf(decimal, sizeof(decimal), ".%ld", (long)(speed_tenths % 10));
  set_label_text_if_changed(pages->home_speed_major, major);
  set_label_text_if_changed(pages->home_speed_decimal, decimal);
}

static void create_dashboard_bezel(lv_obj_t *page) {
  lv_obj_t *bezel = lv_img_create(page);
  lv_img_set_src(bezel, &bike_mb_img_home_bezel);
  lv_obj_set_pos(bezel, 0, 0);
  lv_obj_set_style_img_recolor(bezel, BikeMbUi_Rgb(255, 255, 255), 0);
  lv_obj_set_style_img_recolor_opa(bezel, LV_OPA_COVER, 0);
  lv_obj_clear_flag(bezel, LV_OBJ_FLAG_CLICKABLE);
}

static void create_home_page(BikeMbDashboardPages *pages, lv_obj_t *page) {
  create_dashboard_bezel(page);

  pages->home_assist_glow = lv_img_create(page);
  lv_img_set_src(pages->home_assist_glow, &bike_mb_img_home_assist_glow);
  lv_obj_set_pos(pages->home_assist_glow, 0, 0);
  lv_obj_clear_flag(pages->home_assist_glow, LV_OBJ_FLAG_CLICKABLE);

  create_mode_chip(pages, page);
  pages->home_battery = BikeMbUi_MakeFixedLabel(
      page, "96%", &lv_font_montserrat_18, BikeMbUi_ColorText(), 204, 55, 64, LV_TEXT_ALIGN_RIGHT);
  BikeMbUi_MakeLabel(page, LV_SYMBOL_BATTERY_FULL, &lv_font_montserrat_18, BikeMbUi_ColorText(), 276, 55);

  lv_obj_t *divider = lv_obj_create(page);
  BikeMbUi_StyleClear(divider);
  lv_obj_set_size(divider, 236, 1);
  lv_obj_set_pos(divider, 72, 87);
  lv_obj_set_style_bg_color(divider, BikeMbUi_Rgb(54, 58, 64), 0);
  lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);

  pages->home_speed_major = BikeMbUi_MakeFixedLabel(
      page, "24", &lv_font_montserrat_48, BikeMbUi_ColorText(), 54, 120, 145, LV_TEXT_ALIGN_RIGHT);
  lv_obj_set_style_text_letter_space(pages->home_speed_major, 1, 0);
  pages->home_speed_decimal = BikeMbUi_MakeFixedLabel(
      page, ".5", &lv_font_montserrat_28, BikeMbUi_ColorText(), 204, 139, 70, LV_TEXT_ALIGN_LEFT);
  lv_obj_set_style_text_letter_space(pages->home_speed_decimal, 1, 0);
  pages->home_speed = pages->home_speed_major;
  pages->home_unit = BikeMbUi_MakeFixedLabel(
      page, "km/h", &lv_font_montserrat_22, BikeMbUi_ColorMuted(), 130, 178, 100, LV_TEXT_ALIGN_CENTER);

  pages->home_time = BikeMbUi_MakeFixedLabel(
      page, "12:30", &lv_font_montserrat_22, BikeMbUi_ColorText(), 99, 303, 94, LV_TEXT_ALIGN_LEFT);

  BikeMbUi_MakeFixedLabel(
      page, "ASSIST", &lv_font_montserrat_14, BikeMbUi_ColorAuto(), 142, 260, 76, LV_TEXT_ALIGN_CENTER);
  pages->home_trip = BikeMbUi_MakeFixedLabel(
      page, "58", &lv_font_montserrat_22, BikeMbUi_ColorText(), 150, 294, 60, LV_TEXT_ALIGN_CENTER);
  BikeMbUi_MakeFixedLabel(
      page, "km", &lv_font_montserrat_14, BikeMbUi_ColorMuted(), 150, 314, 60, LV_TEXT_ALIGN_CENTER);

  pages->home_assist = BikeMbUi_MakeFixedLabel(
      page, "186", &lv_font_montserrat_22, BikeMbUi_ColorText(), 0, 0, 64, LV_TEXT_ALIGN_RIGHT);
  lv_obj_add_flag(pages->home_assist, LV_OBJ_FLAG_HIDDEN);

  create_page_dots(pages, page, 0);
}

static void create_ai_page(BikeMbDashboardPages *pages, lv_obj_t *page) {
  create_dashboard_bezel(page);
  pages->ai_battery = BikeMbUi_MakeFixedLabel(
      page, "96%", &lv_font_montserrat_18, BikeMbUi_ColorText(), 58, 55, 72, LV_TEXT_ALIGN_LEFT);
  BikeMbUi_MakeLabel(page, LV_SYMBOL_BATTERY_FULL, &lv_font_montserrat_18, BikeMbUi_ColorText(), 33, 55);
  pages->ai_network = BikeMbUi_MakeFixedLabel(
      page, "Offline", &lv_font_montserrat_18, BikeMbUi_ColorMuted(), 214, 55, 96, LV_TEXT_ALIGN_RIGHT);

  pages->ai_ring = lv_arc_create(page);
  lv_obj_set_size(pages->ai_ring, 184, 184);
  lv_obj_set_pos(pages->ai_ring, 88, 78);
  lv_arc_set_range(pages->ai_ring, 0, 100);
  lv_arc_set_value(pages->ai_ring, 15);
  lv_obj_clear_flag(pages->ai_ring, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_style(pages->ai_ring, NULL, LV_PART_KNOB);
  lv_obj_set_style_arc_width(pages->ai_ring, 9, LV_PART_MAIN);
  lv_obj_set_style_arc_width(pages->ai_ring, 9, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(pages->ai_ring, BikeMbUi_Rgb(48, 54, 58), LV_PART_MAIN);
  lv_obj_set_style_arc_color(pages->ai_ring, BikeMbUi_ColorMuted(), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(pages->ai_ring, LV_OPA_TRANSP, 0);

  pages->wave_line = lv_line_create(page);
  lv_obj_set_style_line_color(pages->wave_line, BikeMbUi_ColorAuto(), 0);
  lv_obj_set_style_line_width(pages->wave_line, 5, 0);
  lv_obj_set_style_line_rounded(pages->wave_line, true, 0);
  lv_obj_set_pos(pages->wave_line, 0, 0);

  pages->ai_identity = BikeMbUi_MakeFixedLabel(
      page, "AI", &lv_font_montserrat_18, BikeMbUi_ColorAuto(), 144, 189, 72, LV_TEXT_ALIGN_CENTER);
  pages->ai_state_label = BikeMbUi_MakeFixedLabel(
      page, "Offline", &lv_font_montserrat_28, BikeMbUi_ColorText(), 64, 246, 232, LV_TEXT_ALIGN_CENTER);
  pages->ai_action_hint = BikeMbUi_MakeFixedLabel(
      page, "AI unavailable", &lv_font_montserrat_18, BikeMbUi_ColorMuted(), 64, 286, 232, LV_TEXT_ALIGN_CENTER);

  pages->wave_value = NULL;
  pages->wave_value_unit = NULL;
  pages->wave_direction = NULL;
  pages->wave_temp = NULL;
  pages->wave_grade = NULL;
  pages->wave_title = NULL;
  create_page_dots(pages, page, 1);
}

static lv_obj_t *create_detail_row(lv_obj_t *parent,
                                   const char *icon,
                                   const char *label,
                                   lv_coord_t y,
                                   lv_obj_t **icon_out,
                                   lv_obj_t **value_out) {
  lv_obj_t *row = lv_obj_create(parent);
  BikeMbUi_StylePanel(row, 14);
  lv_obj_set_size(row, 278, 52);
  lv_obj_set_pos(row, 41, y);
  *icon_out = BikeMbUi_MakeLabel(row, icon, &lv_font_montserrat_22, BikeMbUi_ColorAuto(), 22, 15);
  BikeMbUi_MakeFixedLabel(row, label, &lv_font_montserrat_14, BikeMbUi_ColorMuted(), 72, 18, 94, LV_TEXT_ALIGN_LEFT);
  *value_out = BikeMbUi_MakeFixedLabel(
      row, "0", &lv_font_montserrat_22, BikeMbUi_ColorText(), 160, 14, 90, LV_TEXT_ALIGN_RIGHT);
  return row;
}

static void create_details_page(BikeMbDashboardPages *pages, lv_obj_t *page) {
  create_dashboard_bezel(page);
  BikeMbUi_MakeLabel(page, "AUTO", &lv_font_montserrat_22, BikeMbUi_ColorAuto(), 58, 43);
  BikeMbUi_MakeFixedLabel(
      page, "96%", &lv_font_montserrat_18, BikeMbUi_ColorText(), 235, 55, 64, LV_TEXT_ALIGN_RIGHT);
  BikeMbUi_MakeLabel(page, LV_SYMBOL_BATTERY_FULL, &lv_font_montserrat_18, BikeMbUi_ColorText(), 303, 55);

  create_detail_row(page, "/\\", "DISTANCE", 88, &pages->detail_icons[0], &pages->detail_trip);
  create_detail_row(page, "o", "DURATION", 149, &pages->detail_icons[1], &pages->detail_time);
  create_detail_row(page, "+", "CADENCE", 210, &pages->detail_icons[2], &pages->detail_total);
  create_detail_row(page, "^", "ELEV. GAIN", 271, &pages->detail_icons[3], &pages->detail_average);
  create_page_dots(pages, page, 2);
}

static lv_obj_t *create_settings_row(lv_obj_t *parent,
                                     const char *icon,
                                     const char *label,
                                     lv_coord_t y,
                                     lv_event_cb_t event_cb,
                                     BikeMbDashboardPages *pages) {
  lv_obj_t *row = lv_obj_create(parent);
  BikeMbUi_StylePanel(row, 16);
  lv_obj_set_size(row, 276, 58);
  lv_obj_set_pos(row, 42, y);
  lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
  BikeMbUi_MakeLabel(row, icon, &lv_font_montserrat_22, BikeMbUi_ColorAuto(), 24, 18);
  BikeMbUi_MakeFixedLabel(row, label, &lv_font_montserrat_18, BikeMbUi_ColorText(), 72, 18, 148, LV_TEXT_ALIGN_LEFT);
  BikeMbUi_MakeLabel(row, ">", &lv_font_montserrat_22, BikeMbUi_ColorMuted(), 238, 18);
  lv_obj_add_event_cb(row, event_cb, LV_EVENT_CLICKED, pages);
  return row;
}

static void show_accessories_event_cb(lv_event_t *event) {
  BikeMbDashboardPages *pages = (BikeMbDashboardPages *)lv_event_get_user_data(event);
  if (pages == NULL) {
    return;
  }
  hide_obj(pages->settings_page);
  show_obj(pages->accessories_page);
  hide_obj(pages->about_page);
  pages->active_view = kViewAccessories;
}

static void show_about_event_cb(lv_event_t *event) {
  BikeMbDashboardPages *pages = (BikeMbDashboardPages *)lv_event_get_user_data(event);
  if (pages == NULL) {
    return;
  }
  hide_obj(pages->settings_page);
  hide_obj(pages->accessories_page);
  show_obj(pages->about_page);
  pages->active_view = kViewAbout;
}

static void back_to_settings_event_cb(lv_event_t *event) {
  BikeMbDashboardPages *pages = (BikeMbDashboardPages *)lv_event_get_user_data(event);
  if (pages == NULL) {
    return;
  }
  hide_obj(pages->accessories_page);
  hide_obj(pages->about_page);
  show_obj(pages->settings_page);
  pages->active_view = kViewSettings;
}

static void create_settings_header(lv_obj_t *page, const char *title, BikeMbDashboardPages *pages) {
  lv_obj_t *back = BikeMbUi_MakeLabel(page, "<", &lv_font_montserrat_22, BikeMbUi_ColorAuto(), 42, 47);
  lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(back, back_to_settings_event_cb, LV_EVENT_CLICKED, pages);
  BikeMbUi_MakeFixedLabel(page, title, &lv_font_montserrat_22, BikeMbUi_ColorText(), 74, 48, 220, LV_TEXT_ALIGN_CENTER);
}

static void create_settings_pages(BikeMbDashboardPages *pages, lv_obj_t *screen) {
  pages->settings_page = lv_obj_create(screen);
  BikeMbUi_StylePage(pages->settings_page);
  lv_obj_add_flag(pages->settings_page, LV_OBJ_FLAG_HIDDEN);
  BikeMbUi_MakeFixedLabel(
      pages->settings_page, "SETTINGS", &lv_font_montserrat_22, BikeMbUi_ColorText(), 74, 54, 212, LV_TEXT_ALIGN_CENTER);
  create_settings_row(pages->settings_page, "*", "ACCESSORIES", 128, show_accessories_event_cb, pages);
  create_settings_row(pages->settings_page, "i", "ABOUT DEVICE", 202, show_about_event_cb, pages);

  pages->accessories_page = lv_obj_create(screen);
  BikeMbUi_StylePage(pages->accessories_page);
  lv_obj_add_flag(pages->accessories_page, LV_OBJ_FLAG_HIDDEN);
  create_settings_header(pages->accessories_page, "ACCESSORIES", pages);
  BikeMbUi_MakeFixedLabel(
      pages->accessories_page, "No accessories connected", &lv_font_montserrat_18,
      BikeMbUi_ColorMuted(), 58, 126, 244, LV_TEXT_ALIGN_CENTER);
  BikeMbUi_MakeFixedLabel(
      pages->accessories_page, "Heart Rate   Radar   Light", &lv_font_montserrat_14,
      BikeMbUi_ColorMuted(), 46, 188, 268, LV_TEXT_ALIGN_CENTER);

  pages->about_page = lv_obj_create(screen);
  BikeMbUi_StylePage(pages->about_page);
  lv_obj_add_flag(pages->about_page, LV_OBJ_FLAG_HIDDEN);
  create_settings_header(pages->about_page, "ABOUT DEVICE", pages);
  BikeMbUi_MakeFixedLabel(
      pages->about_page, "BikeMB", &lv_font_montserrat_22, BikeMbUi_ColorText(), 78, 128, 204, LV_TEXT_ALIGN_CENTER);
  BikeMbUi_MakeFixedLabel(
      pages->about_page, BIKE_MB_FIRMWARE_VERSION, &lv_font_montserrat_18,
      BikeMbUi_ColorAuto(), 78, 172, 204, LV_TEXT_ALIGN_CENTER);
  BikeMbUi_MakeFixedLabel(
      pages->about_page, "Arduino / LVGL", &lv_font_montserrat_14,
      BikeMbUi_ColorMuted(), 78, 214, 204, LV_TEXT_ALIGN_CENTER);
}

static lv_color_t ai_accent_color(BikeMbDashboardAiVisualState state) {
  switch (state) {
    case BIKE_MB_DASHBOARD_AI_VISUAL_ERROR:
      return BikeMbUi_ColorBoost();
    case BIKE_MB_DASHBOARD_AI_VISUAL_OFFLINE:
      return BikeMbUi_ColorMuted();
    default:
      return BikeMbUi_ColorAuto();
  }
}

static uint8_t ai_ring_value(const BikeMbDashboardAiUiState *ai, uint8_t phase) {
  switch (ai->visual_state) {
    case BIKE_MB_DASHBOARD_AI_VISUAL_OFFLINE:
      return 12;
    case BIKE_MB_DASHBOARD_AI_VISUAL_IDLE:
      return 28;
    case BIKE_MB_DASHBOARD_AI_VISUAL_THINKING:
    case BIKE_MB_DASHBOARD_AI_VISUAL_SENDING:
      return (uint8_t)(45 + (phase % 12) * 4);
    case BIKE_MB_DASHBOARD_AI_VISUAL_ERROR:
      return 70;
    default:
      return 82;
  }
}

static void update_ai_wave(BikeMbDashboardPages *pages,
                           const BikeMbDashboardAiUiState *ai,
                           uint8_t phase) {
  static const uint8_t active_shape[BIKE_MB_DASHBOARD_WAVE_POINT_COUNT] = {
      16, 26, 44, 56, 38, 22, 34, 62,
      72, 48, 28, 18, 36, 48, 60, 78,
      54, 36, 28, 46, 64, 42, 24, 34,
  };

  for (uint8_t i = 0; i < BIKE_MB_DASHBOARD_WAVE_POINT_COUNT; ++i) {
    uint8_t value = 10;
    if (ai->visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_LISTENING ||
        ai->visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_SPEAKING ||
        ai->visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_MUSIC) {
      value = active_shape[(i + phase) % BIKE_MB_DASHBOARD_WAVE_POINT_COUNT];
    } else if (ai->visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_SENDING ||
               ai->visual_state == BIKE_MB_DASHBOARD_AI_VISUAL_THINKING) {
      value = (uint8_t)(12 + ((i + phase) % 6) * 7);
    }

    pages->wave_points[i].x = 88 + i * 8;
    pages->wave_points[i].y = 170 - (value / 2);
  }

  lv_line_set_points(pages->wave_line, pages->wave_points, BIKE_MB_DASHBOARD_WAVE_POINT_COUNT);
}

static void update_ai_page(BikeMbDashboardPages *pages, const BikeMbDashboardMetrics *metrics) {
  const BikeMbDashboardAiUiState *ai = &metrics->ai;
  const lv_color_t accent = ai_accent_color(ai->visual_state);
  char text[16];

  snprintf(text, sizeof(text), "%u%%", ai->battery_percent);
  set_label_text_if_changed(pages->ai_battery, text);
  set_label_text_if_changed(pages->ai_network, ai->network_text);
  set_label_text_if_changed(pages->ai_state_label, ai->state_text);
  set_label_text_if_changed(pages->ai_action_hint, ai->action_hint);

  lv_obj_set_style_text_color(pages->ai_network, accent, 0);
  lv_obj_set_style_text_color(pages->ai_identity, accent, 0);
  lv_obj_set_style_arc_color(pages->ai_ring, accent, LV_PART_INDICATOR);
  lv_arc_set_value(pages->ai_ring, ai_ring_value(ai, metrics->wavePhase));
  lv_obj_set_style_line_color(pages->wave_line, accent, 0);
  update_ai_wave(pages, ai, metrics->wavePhase);
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

  pages->home_mode_index = 2;
  pages->last_dashboard_page = 0;
  pages->active_view = kViewDashboard;
  pages->mode_changed_callback = NULL;

  for (uint8_t i = 0; i < BIKE_MB_DASHBOARD_PAGE_COUNT; ++i) {
    pages->dashboard_pages[i] = page_objs[i];
  }

  create_home_page(pages, page_objs[0]);
  create_ai_page(pages, page_objs[1]);
  create_details_page(pages, page_objs[2]);
  create_settings_pages(pages, lv_obj_get_parent(page_objs[0]));
  update_mode_accents(pages);
}

void BikeMbDashboardPages_ShowDashboardPage(BikeMbDashboardPages *pages, uint8_t page_index) {
  if (pages == NULL) {
    return;
  }

  page_index %= BIKE_MB_DASHBOARD_PAGE_COUNT;
  for (uint8_t i = 0; i < BIKE_MB_DASHBOARD_PAGE_COUNT; ++i) {
    if (i == page_index) {
      show_obj(pages->dashboard_pages[i]);
    } else {
      hide_obj(pages->dashboard_pages[i]);
    }
  }

  hide_obj(pages->settings_page);
  hide_obj(pages->accessories_page);
  hide_obj(pages->about_page);
  pages->last_dashboard_page = page_index;
  pages->active_view = kViewDashboard;
}

void BikeMbDashboardPages_ShowSettings(BikeMbDashboardPages *pages) {
  if (pages == NULL) {
    return;
  }

  for (uint8_t i = 0; i < BIKE_MB_DASHBOARD_PAGE_COUNT; ++i) {
    hide_obj(pages->dashboard_pages[i]);
  }
  show_obj(pages->settings_page);
  hide_obj(pages->accessories_page);
  hide_obj(pages->about_page);
  pages->active_view = kViewSettings;
}

void BikeMbDashboardPages_ReturnFromSettings(BikeMbDashboardPages *pages) {
  if (pages == NULL) {
    return;
  }

  if (pages->active_view == kViewSettings) {
    BikeMbDashboardPages_ShowDashboardPage(pages, pages->last_dashboard_page);
    return;
  }

  hide_obj(pages->accessories_page);
  hide_obj(pages->about_page);
  show_obj(pages->settings_page);
  pages->active_view = kViewSettings;
}

bool BikeMbDashboardPages_IsDashboardVisible(const BikeMbDashboardPages *pages) {
  return pages != NULL && pages->active_view == kViewDashboard;
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

  update_home_speed(pages, metrics->speedKmh);

  snprintf(text, sizeof(text), "%u%%", metrics->batteryPercent);
  set_label_text_if_changed(pages->home_battery, text);

  format_time(text, sizeof(text), metrics->uptimeMs / 1000);
  set_label_text_if_changed(pages->home_time, text);

  snprintf(text, sizeof(text), "%u", (unsigned int)(58.0f - metrics->tripKm * 0.2f));
  set_label_text_if_changed(pages->home_trip, text);

  snprintf(text, sizeof(text), "%u", (unsigned int)(metrics->assistPowerW + 0.5f));
  set_label_text_if_changed(pages->home_assist, text);

  snprintf(text, sizeof(text), "%.1f km", (double)metrics->tripKm);
  set_label_text_if_changed(pages->detail_trip, text);

  format_time(text, sizeof(text), metrics->rideSeconds);
  set_label_text_if_changed(pages->detail_time, text);

  set_label_text_if_changed(pages->detail_total, "70 rpm");
  set_label_text_if_changed(pages->detail_average, "495 m");

  update_ai_page(pages, metrics);
}
