#include "metrics_service.h"

#include "app/demo_metrics.h"

void MetricsService_Init() {
  DemoMetrics_Init();
}

DemoMetrics MetricsService_UpdateDashboard(uint32_t elapsedMs, uint32_t targetFrameMs, uint32_t renderWorkMs) {
  return DemoMetrics_Update(elapsedMs, targetFrameMs, renderWorkMs);
}
