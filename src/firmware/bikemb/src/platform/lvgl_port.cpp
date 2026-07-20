#include "lvgl_port.h"

#include "esp_heap_caps.h"
#include <lvgl.h>

#include "../drivers/Display_ST77916.h"
#include "../drivers/Touch_CST816.h"
#include "bike_platform.h"

namespace {

constexpr uint16_t kHorRes = EXAMPLE_LCD_WIDTH;
constexpr uint16_t kVerRes = EXAMPLE_LCD_HEIGHT;
#ifndef BIKE_MB_LVGL_BUFFER_LINES
#define BIKE_MB_LVGL_BUFFER_LINES 40
#endif
static_assert(BIKE_MB_LVGL_BUFFER_LINES > 0, "BIKE_MB_LVGL_BUFFER_LINES must be positive");
constexpr uint16_t kBufferLines = BIKE_MB_LVGL_BUFFER_LINES;
constexpr size_t kBufferPixels = static_cast<size_t>(kHorRes) * kBufferLines;

lv_disp_draw_buf_t g_drawBuffer;
lv_disp_drv_t g_dispDrv;
lv_indev_drv_t g_touchDrv;
lv_color_t *g_bufferA = nullptr;
lv_color_t *g_bufferB = nullptr;
LvglPortPerfStats g_lvglPerfStats = {};

void FlushCallback(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colorPtr) {
  const uint32_t width = static_cast<uint32_t>(area->x2 - area->x1 + 1);
  const uint32_t height = static_cast<uint32_t>(area->y2 - area->y1 + 1);
  ++g_lvglPerfStats.flushCount;
  g_lvglPerfStats.flushPixelCount += static_cast<uint64_t>(width) * height;

  LCD_addWindow(
      static_cast<uint16_t>(area->x1),
      static_cast<uint16_t>(area->y1),
      static_cast<uint16_t>(area->x2),
      static_cast<uint16_t>(area->y2),
      reinterpret_cast<uint16_t *>(colorPtr));
  lv_disp_flush_ready(disp);
}

void TouchReadCallback(lv_indev_drv_t *indev, lv_indev_data_t *data) {
  (void)indev;

  Cst816TouchPoint point = {};
  if (TouchCst816_Read(&point) && point.pressed) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = point.x;
    data->point.y = point.y;
    return;
  }

  data->state = LV_INDEV_STATE_REL;
}

}  // namespace

void LvglPort_Init() {
  lv_init();

  g_bufferA = static_cast<lv_color_t *>(
      heap_caps_malloc(kBufferPixels * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  g_bufferB = static_cast<lv_color_t *>(
      heap_caps_malloc(kBufferPixels * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (g_bufferA == nullptr || g_bufferB == nullptr) {
    BikePlatform_LogInfo("BikeMB.LvglPort", "lvgl draw buffer allocation failed");
    while (true) {
      BikePlatform_DelayMs(1000);
    }
  }

  lv_disp_draw_buf_init(&g_drawBuffer, g_bufferA, g_bufferB, kBufferPixels);

  lv_disp_drv_init(&g_dispDrv);
  g_dispDrv.hor_res = kHorRes;
  g_dispDrv.ver_res = kVerRes;
  g_dispDrv.flush_cb = FlushCallback;
  g_dispDrv.draw_buf = &g_drawBuffer;
  lv_disp_drv_register(&g_dispDrv);

  if (TouchCst816_Init()) {
    lv_indev_drv_init(&g_touchDrv);
    g_touchDrv.type = LV_INDEV_TYPE_POINTER;
    g_touchDrv.read_cb = TouchReadCallback;
    lv_indev_drv_register(&g_touchDrv);
  } else {
    BikePlatform_LogInfo("BikeMB.LvglPort", "CST816 touch init failed");
  }
}

void LvglPort_Tick(unsigned long deltaMs) {
  lv_tick_inc(deltaMs);
}

uint32_t LvglPort_Run() {
  const uint32_t startUs = BikePlatform_Micros();
  lv_timer_handler();
  const uint32_t elapsedUs = BikePlatform_Micros() - startUs;
  ++g_lvglPerfStats.handlerRunCount;
  g_lvglPerfStats.handlerTotalUs += elapsedUs;
  if (elapsedUs > g_lvglPerfStats.handlerMaxUs) {
    g_lvglPerfStats.handlerMaxUs = elapsedUs;
  }
  return (elapsedUs + 999U) / 1000U;
}

LvglPortPerfStats LvglPort_GetPerfStats() {
  return g_lvglPerfStats;
}

void LvglPort_ResetPerfStats() {
  g_lvglPerfStats = {};
  LCD_ResetPerfStats();
}
