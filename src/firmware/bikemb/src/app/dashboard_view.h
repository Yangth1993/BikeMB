#pragma once

#include "demo_metrics.h"
#include "ai_assistant_ui_state.h"
#include "dashboard_view_core.h"

void DashboardView_Create();
void DashboardView_Update(const DemoMetrics &metrics, const BikeMbDashboardAiUiState &ai);
void DashboardView_NextPage();
void DashboardView_PreviousPage();
void DashboardView_SetModeChangedCallback(BikeMbDashboardModeChangedCallback callback);
