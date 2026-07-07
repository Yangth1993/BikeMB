#include "dashboard_app.h"

#include <Arduino.h>

#include "dashboard_view.h"
#include "demo_metrics.h"

namespace {

constexpr uint32_t kFrameIntervalMs = 33;

uint32_t g_lastUpdateMs = 0;
uint32_t g_renderWorkMs = 0;

}  // namespace

void DashboardApp_Init() {
  DemoMetrics_Init();
  DashboardView_Create();
  g_lastUpdateMs = millis();
}

void DashboardApp_SetRenderWorkMs(uint32_t renderWorkMs) {
  g_renderWorkMs = renderWorkMs;
}

void DashboardApp_Tick(uint32_t nowMs) {
  if (nowMs - g_lastUpdateMs < kFrameIntervalMs) {
    return;
  }

  const uint32_t elapsedMs = nowMs - g_lastUpdateMs;
  g_lastUpdateMs = nowMs;

  const DemoMetrics metrics = DemoMetrics_Update(elapsedMs, kFrameIntervalMs, g_renderWorkMs);
  DashboardView_Update(metrics);
}
