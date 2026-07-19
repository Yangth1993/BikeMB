#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include "../ai/ai_types.h"
extern "C" {
#endif

typedef enum BikeMbDashboardAiVisualState {
  BIKE_MB_DASHBOARD_AI_VISUAL_OFFLINE = 0,
  BIKE_MB_DASHBOARD_AI_VISUAL_IDLE,
  BIKE_MB_DASHBOARD_AI_VISUAL_LISTENING,
  BIKE_MB_DASHBOARD_AI_VISUAL_SENDING,
  BIKE_MB_DASHBOARD_AI_VISUAL_THINKING,
  BIKE_MB_DASHBOARD_AI_VISUAL_SPEAKING,
  BIKE_MB_DASHBOARD_AI_VISUAL_MUSIC,
  BIKE_MB_DASHBOARD_AI_VISUAL_ERROR,
} BikeMbDashboardAiVisualState;

typedef enum BikeMbDashboardAiSurface {
  BIKE_MB_DASHBOARD_AI_SURFACE_CHIP = 0,
  BIKE_MB_DASHBOARD_AI_SURFACE_MINI_OVERLAY,
  BIKE_MB_DASHBOARD_AI_SURFACE_FULL_PAGE,
} BikeMbDashboardAiSurface;

typedef struct BikeMbDashboardAiUiState {
  BikeMbDashboardAiVisualState visual_state;
  BikeMbDashboardAiSurface preferred_surface;
  uint8_t battery_percent;
  uint8_t can_cancel;
  uint8_t can_retry;
  const char *network_text;
  const char *state_text;
  const char *action_hint;
} BikeMbDashboardAiUiState;

#ifdef __cplusplus
}

BikeMbDashboardAiUiState BikeMbAiUiState_FromSnapshot(const BikeMbAiSnapshot &snapshot,
                                                      uint8_t battery_percent);
#endif
