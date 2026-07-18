#pragma once

#include "ai_types.h"

struct BikeMbAiStateMachine {
  BikeMbAiSnapshot snapshot;
  bool enabled;
  bool buttonHeld;
  bool releaseRequired;
  uint32_t deadlineMs;
  uint32_t errorUntilMs;
};

void BikeMbAiStateMachine_Init(
    BikeMbAiStateMachine *machine,
    bool enabled,
    uint32_t nowMs);
uint32_t BikeMbAiStateMachine_Dispatch(
    BikeMbAiStateMachine *machine,
    const BikeMbAiEvent &event);
