#pragma once

#include <stdint.h>

void DashboardApp_Init();
void DashboardApp_SetRenderWorkMs(uint32_t renderWorkMs);
void DashboardApp_Tick(uint32_t nowMs);
