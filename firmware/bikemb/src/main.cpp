#if defined(BIKE_MB_USE_ESPIDF_RUNTIME)
#include "esp_log.h"
#else
#include <Arduino.h>
#endif

#include "app/dashboard_app.h"
#include "app/display_diagnostics.h"
#include "platform/board_support.h"
#include "platform/bike_platform.h"
#include "platform/lvgl_port.h"
#include "runtime/bike_runtime.h"

#ifndef BIKE_MB_RUN_DISPLAY_DIAGNOSTIC
#define BIKE_MB_RUN_DISPLAY_DIAGNOSTIC 0
#endif

namespace {
uint32_t g_lastTickMs = 0;
constexpr const char *TAG = "BikeMB.Main";
}

#if defined(BIKE_MB_USE_ESPIDF_RUNTIME)
extern "C" void app_main() {
  ESP_LOGI(TAG, "bikemb ESP-IDF runtime boot");

#if BIKE_MB_RUN_DISPLAY_DIAGNOSTIC
  BoardSupport_Init();
  DisplayDiagnostics_Run();
  while (true) {
    BikePlatform_DelayMs(1000);
  }
#else
  BikeRuntime_Init();
  BikeRuntime_Start();
  ESP_LOGI(TAG, "BikeMB runtime started");
#endif
}
#else
void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("[BikeMB] bikemb lvgl demo boot");

  BoardSupport_Init();
#if BIKE_MB_RUN_DISPLAY_DIAGNOSTIC
  DisplayDiagnostics_Run();
#else
  LvglPort_Init();
  DashboardApp_Init();

  g_lastTickMs = millis();
  Serial.println("[BikeMB] lvgl dashboard ready");
#endif
}

void loop() {
#if BIKE_MB_RUN_DISPLAY_DIAGNOSTIC
  delay(1000);
#else
  const uint32_t now = millis();
  const uint32_t deltaMs = now - g_lastTickMs;
  g_lastTickMs = now;

  LvglPort_Tick(deltaMs);
  DashboardApp_Tick(now);
  DashboardApp_SetRenderWorkMs(LvglPort_Run());

  delay(5);
#endif
}
#endif
