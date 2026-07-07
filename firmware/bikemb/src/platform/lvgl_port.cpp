#include "lvgl_port.h"

#include <Arduino.h>
#include "esp_heap_caps.h"
#include <lvgl.h>

#include "../drivers/Display_ST77916.h"

namespace {

constexpr uint16_t kHorRes = EXAMPLE_LCD_WIDTH;
constexpr uint16_t kVerRes = EXAMPLE_LCD_HEIGHT;
constexpr uint16_t kBufferLines = 24;
constexpr size_t kBufferPixels = static_cast<size_t>(kHorRes) * kBufferLines;

lv_disp_draw_buf_t g_drawBuffer;
lv_disp_drv_t g_dispDrv;
lv_color_t *g_bufferA = nullptr;
lv_color_t *g_bufferB = nullptr;

void FlushCallback(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colorPtr) {
  LCD_addWindow(
      static_cast<uint16_t>(area->x1),
      static_cast<uint16_t>(area->y1),
      static_cast<uint16_t>(area->x2),
      static_cast<uint16_t>(area->y2),
      reinterpret_cast<uint16_t *>(colorPtr));
  lv_disp_flush_ready(disp);
}

}  // namespace

void LvglPort_Init() {
  lv_init();

  g_bufferA = static_cast<lv_color_t *>(
      heap_caps_malloc(kBufferPixels * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  g_bufferB = static_cast<lv_color_t *>(
      heap_caps_malloc(kBufferPixels * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (g_bufferA == nullptr || g_bufferB == nullptr) {
    Serial.println("[BikeMB] lvgl draw buffer allocation failed");
    while (true) {
      delay(1000);
    }
  }

  lv_disp_draw_buf_init(&g_drawBuffer, g_bufferA, g_bufferB, kBufferPixels);

  lv_disp_drv_init(&g_dispDrv);
  g_dispDrv.hor_res = kHorRes;
  g_dispDrv.ver_res = kVerRes;
  g_dispDrv.flush_cb = FlushCallback;
  g_dispDrv.draw_buf = &g_drawBuffer;
  lv_disp_drv_register(&g_dispDrv);
}

void LvglPort_Tick(unsigned long deltaMs) {
  lv_tick_inc(deltaMs);
}

uint32_t LvglPort_Run() {
  const uint32_t startUs = micros();
  lv_timer_handler();
  return (micros() - startUs + 999U) / 1000U;
}
