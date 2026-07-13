#include "dashboard_view_core.h"

#include "../drivers/Touch_CST816.h"
#include "dashboard_pages.h"
#include "dashboard_ui_style.h"
#include "lvgl.h"

enum {
  kScreenSize = 360,
};

static lv_obj_t *g_pages[BIKE_MB_DASHBOARD_PAGE_COUNT];
static BikeMbDashboardPages g_dashboard_pages;
static uint8_t g_visible_page = 0;

static void create_outer_ring(lv_obj_t *screen) {
  lv_obj_t *ring = lv_obj_create(screen);
  BikeMbUi_StyleClear(ring);
  lv_obj_set_size(ring, kScreenSize, kScreenSize);
  lv_obj_center(ring);
  lv_obj_set_style_bg_color(ring, BikeMbUi_ColorBg(), 0);
  lv_obj_set_style_bg_opa(ring, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(ring, 2, 0);
  lv_obj_set_style_border_color(ring, BikeMbUi_Rgb(38, 41, 46), 0);
  lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
}

static lv_obj_t *create_page(lv_obj_t *screen, uint8_t index) {
  lv_obj_t *page = lv_obj_create(screen);
  BikeMbUi_StylePage(page);
  if (index != 0) {
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
  }
  g_pages[index] = page;
  return page;
}

static void update_visible_page(uint8_t page) {
  page %= BIKE_MB_DASHBOARD_PAGE_COUNT;
  if (page == g_visible_page) {
    return;
  }

  for (uint8_t i = 0; i < BIKE_MB_DASHBOARD_PAGE_COUNT; ++i) {
    if (i == page) {
      lv_obj_clear_flag(g_pages[i], LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(g_pages[i], LV_OBJ_FLAG_HIDDEN);
    }
  }
  g_visible_page = page;
}

void BikeMbDashboardView_NextPage(void) {
  update_visible_page((uint8_t)((g_visible_page + 1) % BIKE_MB_DASHBOARD_PAGE_COUNT));
}

void BikeMbDashboardView_PreviousPage(void) {
  update_visible_page((uint8_t)((g_visible_page + BIKE_MB_DASHBOARD_PAGE_COUNT - 1) %
                                BIKE_MB_DASHBOARD_PAGE_COUNT));
}

void BikeMbDashboardView_SetModeChangedCallback(BikeMbDashboardModeChangedCallback callback) {
  BikeMbDashboardPages_SetModeChangedCallback(&g_dashboard_pages, callback);
}

static void handle_touch_gesture(void) {
  const Cst816Gesture gesture = TouchCst816_ConsumeGesture();
  if (gesture == CST816_GESTURE_SWIPE_LEFT) {
    BikeMbDashboardView_NextPage();
  } else if (gesture == CST816_GESTURE_SWIPE_RIGHT) {
    BikeMbDashboardView_PreviousPage();
  }
}

void BikeMbDashboardView_Create(void) {
  lv_obj_t *screen = lv_obj_create(NULL);
  BikeMbUi_StyleClear(screen);
  lv_obj_set_style_bg_color(screen, BikeMbUi_ColorBg(), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(screen, 0, 0);

  create_outer_ring(screen);
  for (uint8_t i = 0; i < BIKE_MB_DASHBOARD_PAGE_COUNT; ++i) {
    create_page(screen, i);
  }
  BikeMbDashboardPages_Create(&g_dashboard_pages, g_pages);

  lv_scr_load(screen);
}

void BikeMbDashboardView_Update(const BikeMbDashboardMetrics *metrics) {
  BikeMbDashboardPages_Update(&g_dashboard_pages, metrics);
  handle_touch_gesture();
}
