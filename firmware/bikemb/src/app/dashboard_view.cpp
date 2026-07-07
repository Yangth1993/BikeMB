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

  BikeMbDashboardView_Update(&viewMetrics);
}
