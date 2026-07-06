#include "dashboard_app.h"

#include <Arduino.h>

#include "dashboard_view.h"
#include "demo_metrics.h"

namespace {

constexpr uint32_t kFrameIntervalMs = 33;

uint32_t g_lastUpdateMs = 0;

}  // namespace

void DashboardApp_Init() {
  DemoMetrics_Init();
  DashboardView_Create();
  g_lastUpdateMs = millis();
}

void DashboardApp_Tick(uint32_t nowMs) {
  if (nowMs - g_lastUpdateMs < kFrameIntervalMs) {
    return;
  }

  const uint32_t elapsedMs = nowMs - g_lastUpdateMs;
  g_lastUpdateMs = nowMs;

  const DemoMetrics metrics = DemoMetrics_Update(elapsedMs, kFrameIntervalMs);
  DashboardView_Update(metrics);

  static uint32_t lastLogMs = 0;
  if (nowMs - lastLogMs >= 1000) {
    lastLogMs = nowMs;
    Serial.printf(
        "[BikeMB] cpu=%2.0f%% fps=%4.1f heap=%u/%u psram=%u/%u\n",
        metrics.cpuLoad,
        metrics.fps,
        metrics.heapFree,
        metrics.heapTotal,
        metrics.psramFree,
        metrics.psramTotal);
  }
}
