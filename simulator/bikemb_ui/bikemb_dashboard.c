#include "bikemb_dashboard.h"

#include <stdint.h>
#include "dashboard_view_core.h"
#include "lvgl/lvgl.h"

enum {
  kOrbSize = 48,
  kOrbMinX = 26,
  kOrbMaxX = 360 - kOrbSize - 26,
  kOrbMinY = 198,
  kOrbMaxY = 360 - kOrbSize - 62,
};

static int16_t g_orb_x = 52;
static int16_t g_orb_y = 206;
static int8_t g_velocity_x = 3;
static int8_t g_velocity_y = 2;
static uint32_t g_frame_count;
static uint32_t g_last_fps_at;
static float g_fps;
static float g_cpu_load;

static void update_orb(void) {
  g_orb_x += g_velocity_x;
  g_orb_y += g_velocity_y;

  if (g_orb_x <= kOrbMinX || g_orb_x >= kOrbMaxX) {
    g_velocity_x = (int8_t)-g_velocity_x;
    g_orb_x += g_velocity_x;
  }

  if (g_orb_y <= kOrbMinY || g_orb_y >= kOrbMaxY) {
    g_velocity_y = (int8_t)-g_velocity_y;
    g_orb_y += g_velocity_y;
  }
}

static void update_dashboard(lv_timer_t *timer) {
  (void)timer;

  ++g_frame_count;
  update_orb();

  const uint32_t now = lv_tick_get();
  if (g_last_fps_at == 0) {
    g_last_fps_at = now;
  }

  if (now - g_last_fps_at >= 1000) {
    g_fps = g_frame_count * 1000.0f / (float)(now - g_last_fps_at);
    g_frame_count = 0;
    g_last_fps_at = now;
  }

  const uint32_t phase = (now / 33U) % 200U;
  const float wave = phase <= 100U ? (float)phase : (float)(200U - phase);
  g_cpu_load = 0.88f * g_cpu_load + 0.12f * (35.0f + wave * 0.55f);

  const int mem_load = 42 + (int)((now / 180U) % 28U);
  const int psram_load = 18 + (int)((now / 260U) % 18U);

  BikeMbDashboardMetrics metrics = {
      .fps = g_fps,
      .cpuLoad = g_cpu_load,
      .heapFree = (uint32_t)(100 - mem_load),
      .heapTotal = 100,
      .psramFree = (uint32_t)(100 - psram_load),
      .psramTotal = 100,
      .orbX = g_orb_x,
      .orbY = g_orb_y,
  };
  BikeMbDashboardView_Update(&metrics);
}

void bikemb_dashboard_create(void) {
  BikeMbDashboardView_Create();
  lv_timer_create(update_dashboard, 33, NULL);
}
