#pragma once

#include <stdint.h>
#include "dashboard_view_core.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  BIKE_MB_DASHBOARD_PAGE_COUNT = 3,
  BIKE_MB_DASHBOARD_WAVE_POINT_COUNT = 24,
};

typedef struct BikeMbDashboardPages {
  lv_obj_t *home_speed;
  lv_obj_t *home_trip;
  lv_obj_t *home_assist;
  lv_obj_t *home_battery;
  lv_obj_t *home_time;
  lv_obj_t *home_mode;
  lv_obj_t *wave_value;
  lv_obj_t *wave_line;
  lv_point_t wave_points[BIKE_MB_DASHBOARD_WAVE_POINT_COUNT];
  lv_obj_t *wave_direction;
  lv_obj_t *wave_temp;
  lv_obj_t *wave_grade;
  lv_obj_t *detail_trip;
  lv_obj_t *detail_time;
  lv_obj_t *detail_total;
  lv_obj_t *detail_average;
  lv_obj_t *page_dots[BIKE_MB_DASHBOARD_PAGE_COUNT][BIKE_MB_DASHBOARD_PAGE_COUNT];
  BikeMbDashboardModeChangedCallback mode_changed_callback;
  uint8_t home_mode_index;
} BikeMbDashboardPages;

void BikeMbDashboardPages_Create(BikeMbDashboardPages *pages, lv_obj_t *page_objs[BIKE_MB_DASHBOARD_PAGE_COUNT]);
void BikeMbDashboardPages_Update(BikeMbDashboardPages *pages, const BikeMbDashboardMetrics *metrics);
void BikeMbDashboardPages_SetModeChangedCallback(BikeMbDashboardPages *pages,
                                                 BikeMbDashboardModeChangedCallback callback);

#ifdef __cplusplus
}
#endif
