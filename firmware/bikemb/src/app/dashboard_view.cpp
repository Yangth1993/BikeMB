#include "dashboard_view.h"

#include "dashboard_view_core.h"

void DashboardView_Create() {
  BikeMbDashboardView_Create();
}

void DashboardView_Update(const DemoMetrics &metrics) {
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
  viewMetrics.activePage = metrics.activePage;
  viewMetrics.wavePhase = metrics.wavePhase;

  BikeMbDashboardView_Update(&viewMetrics);
}
