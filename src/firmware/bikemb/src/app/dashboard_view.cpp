#include "dashboard_view.h"

#include "dashboard_view_core.h"

void DashboardView_Create() {
  BikeMbDashboardView_Create();
}

void DashboardView_NextPage() {
  BikeMbDashboardView_NextPage();
}

void DashboardView_PreviousPage() {
  BikeMbDashboardView_PreviousPage();
}

void DashboardView_SetModeChangedCallback(BikeMbDashboardModeChangedCallback callback) {
  BikeMbDashboardView_SetModeChangedCallback(callback);
}

void DashboardView_Update(const DemoMetrics &metrics, const BikeMbDashboardAiUiState &ai) {
  BikeMbDashboardMetrics viewMetrics{};
  viewMetrics.fps = metrics.fps;
  viewMetrics.cpuLoad = metrics.cpuLoad;
  viewMetrics.heapFree = metrics.heapFree;
  viewMetrics.heapTotal = metrics.heapTotal;
  viewMetrics.psramFree = metrics.psramFree;
  viewMetrics.psramTotal = metrics.psramTotal;
  viewMetrics.orbX = metrics.orbX;
  viewMetrics.orbY = metrics.orbY;
  viewMetrics.uptimeMs = metrics.uptimeMs;
  viewMetrics.rideSeconds = metrics.rideSeconds;
  viewMetrics.speedKmh = metrics.speedKmh;
  viewMetrics.tripKm = metrics.tripKm;
  viewMetrics.totalKm = metrics.totalKm;
  viewMetrics.averageSpeedKmh = metrics.averageSpeedKmh;
  viewMetrics.assistPowerW = metrics.assistPowerW;
  viewMetrics.temperatureC = metrics.temperatureC;
  viewMetrics.gradePercent = metrics.gradePercent;
  viewMetrics.batteryPercent = metrics.batteryPercent;
  viewMetrics.wavePhase = metrics.wavePhase;
  viewMetrics.ai = ai;

  BikeMbDashboardView_Update(&viewMetrics);
}
