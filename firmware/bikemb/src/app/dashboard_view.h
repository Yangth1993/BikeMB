#pragma once

#include "demo_metrics.h"
#include "dashboard_view_core.h"

void DashboardView_Create();
void DashboardView_Update(const DemoMetrics &metrics);
void DashboardView_NextPage();
void DashboardView_PreviousPage();
void DashboardView_SetModeChangedCallback(BikeMbDashboardModeChangedCallback callback);
