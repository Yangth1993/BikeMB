#pragma once

#include <stdbool.h>
#include <stdint.h>

enum BikeMbAiButtonEvent {
  BIKE_MB_AI_BUTTON_EVENT_NONE = 0,
  BIKE_MB_AI_BUTTON_EVENT_PRESSED,
  BIKE_MB_AI_BUTTON_EVENT_RELEASED,
};

struct BikeMbAiButtonLogic {
  bool armed;
  bool releaseCandidateActive;
  bool stablePressed;
  bool candidatePressed;
  uint32_t releaseCandidateSinceMs;
  uint32_t edgeCandidateSinceMs;
};

void BikeMbAiButtonLogic_Init(BikeMbAiButtonLogic *logic);
BikeMbAiButtonEvent BikeMbAiButtonLogic_Update(
    BikeMbAiButtonLogic *logic,
    uint32_t nowMs,
    bool rawPressed);
