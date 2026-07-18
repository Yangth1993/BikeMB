#include "ai_button_logic.h"

#include "../ai/ai_config.h"

namespace {
bool elapsed(uint32_t nowMs, uint32_t sinceMs, uint32_t durationMs) {
  return static_cast<uint32_t>(nowMs - sinceMs) >= durationMs;
}
}

void BikeMbAiButtonLogic_Init(BikeMbAiButtonLogic *logic) {
  if (logic == nullptr) {
    return;
  }
  *logic = {};
}

BikeMbAiButtonEvent BikeMbAiButtonLogic_Update(
    BikeMbAiButtonLogic *logic,
    uint32_t nowMs,
    bool rawPressed) {
  if (logic == nullptr || nowMs < BikeMbAiConfig::kStartupGuardMs) {
    return BIKE_MB_AI_BUTTON_EVENT_NONE;
  }

  if (!logic->armed) {
    if (rawPressed) {
      logic->releaseCandidateActive = false;
      return BIKE_MB_AI_BUTTON_EVENT_NONE;
    }
    if (!logic->releaseCandidateActive) {
      logic->releaseCandidateActive = true;
      logic->releaseCandidateSinceMs = nowMs;
      return BIKE_MB_AI_BUTTON_EVENT_NONE;
    }
    if (!elapsed(nowMs, logic->releaseCandidateSinceMs, BikeMbAiConfig::kReleaseToArmMs)) {
      return BIKE_MB_AI_BUTTON_EVENT_NONE;
    }
    logic->armed = true;
    logic->stablePressed = false;
    logic->candidatePressed = false;
    logic->edgeCandidateSinceMs = nowMs;
    return BIKE_MB_AI_BUTTON_EVENT_NONE;
  }

  if (rawPressed != logic->candidatePressed) {
    logic->candidatePressed = rawPressed;
    logic->edgeCandidateSinceMs = nowMs;
    return BIKE_MB_AI_BUTTON_EVENT_NONE;
  }
  if (logic->candidatePressed == logic->stablePressed ||
      !elapsed(nowMs, logic->edgeCandidateSinceMs, BikeMbAiConfig::kDebounceMs)) {
    return BIKE_MB_AI_BUTTON_EVENT_NONE;
  }

  logic->stablePressed = logic->candidatePressed;
  return logic->stablePressed ? BIKE_MB_AI_BUTTON_EVENT_PRESSED
                              : BIKE_MB_AI_BUTTON_EVENT_RELEASED;
}
