#include <assert.h>

#include "../../src/firmware/bikemb/src/ai/ai_state_machine.h"

static uint32_t dispatch(
    BikeMbAiStateMachine *machine,
    BikeMbAiEventType type,
    uint32_t nowMs,
    uint32_t requestId = 0,
    bool value = false) {
  const BikeMbAiEvent event = {type, nowMs, requestId, value, nullptr};
  return BikeMbAiStateMachine_Dispatch(machine, event);
}

int main() {
  BikeMbAiStateMachine machine = {};
  BikeMbAiStateMachine_Init(&machine, false, 0);
  assert(machine.snapshot.state == BIKE_MB_AI_DISABLED);

  dispatch(&machine, BIKE_MB_AI_EVENT_SET_ENABLED, 1, 0, true);
  dispatch(&machine, BIKE_MB_AI_EVENT_SET_WIFI, 2, 0, true);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);

  uint32_t effects = dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 100);
  assert(machine.snapshot.state == BIKE_MB_AI_RECORDING);
  assert(machine.snapshot.requestId == 1);
  assert(effects == BIKE_MB_AI_EFFECT_START_CAPTURE);

  effects = dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_RELEASED, 250);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);
  assert((effects & BIKE_MB_AI_EFFECT_CANCEL_AUDIO) != 0);
  assert((effects & BIKE_MB_AI_EFFECT_SUBMIT_STT) == 0);

  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 1000);
  const uint32_t request = machine.snapshot.requestId;
  effects = dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_RELEASED, 1400);
  assert(machine.snapshot.state == BIKE_MB_AI_RECOGNIZING);
  assert(machine.deadlineMs == 16400);
  assert((effects & BIKE_MB_AI_EFFECT_SUBMIT_STT) != 0);

  dispatch(&machine, BIKE_MB_AI_EVENT_STT_READY, 2000, request);
  assert(machine.snapshot.state == BIKE_MB_AI_THINKING);
  dispatch(&machine, BIKE_MB_AI_EVENT_LLM_READY, 3000, request);
  assert(machine.snapshot.state == BIKE_MB_AI_SYNTHESIZING);
  dispatch(&machine, BIKE_MB_AI_EVENT_TTS_STARTED, 4000, request);
  assert(machine.snapshot.state == BIKE_MB_AI_SPEAKING);
  dispatch(&machine, BIKE_MB_AI_EVENT_PLAYBACK_DONE, 5000, request);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);

  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 6000);
  const uint32_t canceledRequest = machine.snapshot.requestId;
  effects = dispatch(&machine, BIKE_MB_AI_EVENT_CANCEL, 6100);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);
  assert(machine.snapshot.requestId == canceledRequest + 1);
  assert((effects & BIKE_MB_AI_EFFECT_CANCEL_CLOUD) != 0);
  dispatch(&machine, BIKE_MB_AI_EVENT_STT_READY, 6200, canceledRequest);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);

  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 7000);
  dispatch(&machine, BIKE_MB_AI_EVENT_TICK, 17000);
  assert(machine.snapshot.state == BIKE_MB_AI_ERROR);
  assert(machine.releaseRequired);
  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_RELEASED, 17100);
  dispatch(&machine, BIKE_MB_AI_EVENT_TICK, 18499);
  assert(machine.snapshot.state == BIKE_MB_AI_ERROR);
  dispatch(&machine, BIKE_MB_AI_EVENT_TICK, 18500);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);

  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 19000);
  const uint32_t timedRequest = machine.snapshot.requestId;
  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_RELEASED, 19400);
  dispatch(&machine, BIKE_MB_AI_EVENT_STT_READY, 34401, timedRequest);
  assert(machine.snapshot.state == BIKE_MB_AI_ERROR);

  BikeMbAiStateMachine unavailable = {};
  BikeMbAiStateMachine_Init(&unavailable, true, 0);
  effects = dispatch(&unavailable, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 100);
  assert(unavailable.snapshot.state == BIKE_MB_AI_ERROR);
  assert((effects & BIKE_MB_AI_EFFECT_START_CAPTURE) == 0);

  BikeMbAiStateMachine replacement = {};
  BikeMbAiStateMachine_Init(&replacement, true, 0);
  dispatch(&replacement, BIKE_MB_AI_EVENT_SET_WIFI, 1, 0, true);
  dispatch(&replacement, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 1000);
  dispatch(&replacement, BIKE_MB_AI_EVENT_BUTTON_RELEASED, 1400);
  const uint32_t replacedRequest = replacement.snapshot.requestId;
  effects = dispatch(&replacement, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 1500);
  assert(replacement.snapshot.state == BIKE_MB_AI_RECORDING);
  assert(replacement.snapshot.requestId == replacedRequest + 1);
  assert((effects & BIKE_MB_AI_EFFECT_CANCEL_CLOUD) != 0);
  assert((effects & BIKE_MB_AI_EFFECT_START_CAPTURE) != 0);
  return 0;
}
