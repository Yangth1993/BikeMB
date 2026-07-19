#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "dashboard_view_core.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  BIKE_MB_DASHBOARD_PAGE_COUNT = 3,
  BIKE_MB_DASHBOARD_MODE_COUNT = 4,
  BIKE_MB_DASHBOARD_WAVE_POINT_COUNT = 24,
};

typedef struct BikeMbDashboardPages {
  lv_obj_t *dashboard_pages[BIKE_MB_DASHBOARD_PAGE_COUNT];
  lv_obj_t *settings_page;
  lv_obj_t *accessories_page;
  lv_obj_t *about_page;
  lv_obj_t *home_speed;
  lv_obj_t *home_speed_major;
  lv_obj_t *home_speed_decimal;
  lv_obj_t *home_trip;
  lv_obj_t *home_assist;
  lv_obj_t *home_assist_glow;
  lv_obj_t *home_battery;
  lv_obj_t *home_time;
  lv_obj_t *home_mode;
  lv_obj_t *home_mode_chip;
  lv_obj_t *home_unit;
  lv_obj_t *wave_value;
  lv_obj_t *wave_value_unit;
  lv_obj_t *wave_line;
  lv_point_t wave_points[BIKE_MB_DASHBOARD_WAVE_POINT_COUNT];
  lv_obj_t *wave_direction;
  lv_obj_t *wave_temp;
  lv_obj_t *wave_grade;
  lv_obj_t *wave_title;
  lv_obj_t *ai_battery;
  lv_obj_t *ai_network;
  lv_obj_t *ai_ring;
  lv_obj_t *ai_identity;
  lv_obj_t *ai_state_label;
  lv_obj_t *ai_action_hint;
  lv_obj_t *detail_trip;
  lv_obj_t *detail_time;
  lv_obj_t *detail_total;
  lv_obj_t *detail_average;
  lv_obj_t *detail_icons[4];
  lv_obj_t *page_dots[BIKE_MB_DASHBOARD_PAGE_COUNT][BIKE_MB_DASHBOARD_PAGE_COUNT];
  BikeMbDashboardModeChangedCallback mode_changed_callback;
  uint8_t home_mode_index;
  uint8_t last_dashboard_page;
  uint8_t active_view;
} BikeMbDashboardPages;

void BikeMbDashboardPages_Create(BikeMbDashboardPages *pages, lv_obj_t *page_objs[BIKE_MB_DASHBOARD_PAGE_COUNT]);
void BikeMbDashboardPages_Update(BikeMbDashboardPages *pages, const BikeMbDashboardMetrics *metrics);
void BikeMbDashboardPages_ShowDashboardPage(BikeMbDashboardPages *pages, uint8_t page_index);
void BikeMbDashboardPages_ShowSettings(BikeMbDashboardPages *pages);
void BikeMbDashboardPages_ReturnFromSettings(BikeMbDashboardPages *pages);
bool BikeMbDashboardPages_IsDashboardVisible(const BikeMbDashboardPages *pages);
void BikeMbDashboardPages_SetModeChangedCallback(BikeMbDashboardPages *pages,
                                                 BikeMbDashboardModeChangedCallback callback);

#ifdef __cplusplus
}
#endif
