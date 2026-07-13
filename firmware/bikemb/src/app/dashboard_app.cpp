#include "dashboard_app.h"

#include "dashboard_view.h"
#include "platform/bike_platform.h"
#include "services/metrics_service.h"

namespace {

constexpr uint32_t kFrameIntervalMs = 33;

uint32_t g_lastUpdateMs = 0;
uint32_t g_renderWorkMs = 0;

}  // namespace

void DashboardApp_Init() {
  MetricsService_Init();
  DashboardView_Create();
  g_lastUpdateMs = BikePlatform_Millis();
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

  const DemoMetrics metrics = MetricsService_UpdateDashboard(elapsedMs, kFrameIntervalMs, g_renderWorkMs);
  DashboardView_Update(metrics);
}
