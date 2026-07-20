#include "dashboard_app.h"

#include "ai/ai_assistant.h"
#include "ai/ai_config.h"
#include "ai/ai_types.h"
#include "ai_assistant_ui_state.h"
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

void DashboardApp_ShowAiPage() {
  DashboardView_ShowAiPage();
}

void DashboardApp_NextPage() {
  DashboardView_NextPage();
}

void DashboardApp_PreviousPage() {
  DashboardView_PreviousPage();
}

void DashboardApp_SetModeChangedCallback(BikeMbDashboardModeChangedCallback callback) {
  DashboardView_SetModeChangedCallback(callback);
}

void DashboardApp_Tick(uint32_t nowMs) {
  if (nowMs - g_lastUpdateMs < kFrameIntervalMs) {
    return;
  }

  const uint32_t elapsedMs = nowMs - g_lastUpdateMs;
  g_lastUpdateMs = nowMs;

  const DemoMetrics metrics = MetricsService_UpdateDashboard(elapsedMs, kFrameIntervalMs, g_renderWorkMs);
  BikeMbAiSnapshot aiSnapshot{};
#if BIKE_MB_ENABLE_AI_ASSISTANT
  BikeMbAiAssistant_GetSnapshot(&aiSnapshot);
#else
  aiSnapshot.state = BIKE_MB_AI_DISABLED;
  aiSnapshot.stateSinceMs = nowMs;
  aiSnapshot.wifiConnected = false;
  aiSnapshot.cancelAvailable = false;
#endif

  const BikeMbDashboardAiUiState aiUi = BikeMbAiUiState_FromSnapshot(aiSnapshot, metrics.batteryPercent);
  DashboardView_Update(metrics, aiUi);
}
