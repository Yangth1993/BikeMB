#pragma once

#include <stdint.h>

#include "app/demo_metrics.h"

void MetricsService_Init();
DemoMetrics MetricsService_UpdateDashboard(uint32_t elapsedMs, uint32_t targetFrameMs, uint32_t renderWorkMs);
