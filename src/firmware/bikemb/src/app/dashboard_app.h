#pragma once

#include <stdint.h>

#include "dashboard_view_core.h"

void DashboardApp_Init();
void DashboardApp_SetRenderWorkMs(uint32_t renderWorkMs);
void DashboardApp_Tick(uint32_t nowMs);
void DashboardApp_ShowAiPage();
void DashboardApp_NextPage();
void DashboardApp_PreviousPage();
void DashboardApp_SetModeChangedCallback(BikeMbDashboardModeChangedCallback callback);
