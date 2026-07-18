#include "ai_state_machine.h"

#include <string.h>

#include "ai_config.h"

namespace {
bool elapsed(uint32_t nowMs, uint32_t sinceMs, uint32_t durationMs) {
  return static_cast<uint32_t>(nowMs - sinceMs) >= durationMs;
}

bool deadlinePassed(uint32_t nowMs, uint32_t deadlineMs) {
  return deadlineMs != 0 && static_cast<int32_t>(nowMs - deadlineMs) > 0;
}

bool isActive(BikeMbAiState state) {
  return state == BIKE_MB_AI_RECORDING ||
         state == BIKE_MB_AI_RECOGNIZING ||
         state == BIKE_MB_AI_THINKING ||
         state == BIKE_MB_AI_SYNTHESIZING ||
         state == BIKE_MB_AI_SPEAKING ||
         state == BIKE_MB_AI_CONNECTING_MUSIC ||
         state == BIKE_MB_AI_MUSIC_PLAYING;
}

bool isCloudState(BikeMbAiState state) {
  return state == BIKE_MB_AI_RECOGNIZING ||
         state == BIKE_MB_AI_THINKING ||
         state == BIKE_MB_AI_SYNTHESIZING;
}

bool isWorkerEvent(BikeMbAiEventType type) {
  return type == BIKE_MB_AI_EVENT_STT_READY ||
         type == BIKE_MB_AI_EVENT_LLM_READY ||
         type == BIKE_MB_AI_EVENT_TTS_STARTED ||
         type == BIKE_MB_AI_EVENT_PLAYBACK_DONE ||
         type == BIKE_MB_AI_EVENT_FAILURE;
}

const char *stateDetail(BikeMbAiState state) {
  switch (state) {
    case BIKE_MB_AI_DISABLED: return "AI disabled";
    case BIKE_MB_AI_IDLE: return "AI ready";
    case BIKE_MB_AI_RECORDING: return "Listening";
    case BIKE_MB_AI_RECOGNIZING: return "Recognizing";
    case BIKE_MB_AI_THINKING: return "Thinking";
    case BIKE_MB_AI_SYNTHESIZING: return "Preparing speech";
    case BIKE_MB_AI_SPEAKING: return "Speaking";
    case BIKE_MB_AI_CONNECTING_MUSIC: return "Connecting music";
    case BIKE_MB_AI_MUSIC_PLAYING: return "Music playing";
    case BIKE_MB_AI_ERROR: return "AI unavailable";
  }
  return "AI unavailable";
}

void setDetail(BikeMbAiSnapshot *snapshot, const char *detail) {
  const char *safeDetail = detail == nullptr ? "" : detail;
  strncpy(snapshot->detail, safeDetail, sizeof(snapshot->detail) - 1);
  snapshot->detail[sizeof(snapshot->detail) - 1] = '\0';
}

void setState(BikeMbAiStateMachine *machine, BikeMbAiState state, uint32_t nowMs) {
  machine->snapshot.state = state;
  machine->snapshot.stateSinceMs = nowMs;
  machine->snapshot.cancelAvailable = isActive(state);
  setDetail(&machine->snapshot, stateDetail(state));
}

void setError(BikeMbAiStateMachine *machine, uint32_t nowMs, const char *detail) {
  setState(machine, BIKE_MB_AI_ERROR, nowMs);
  machine->errorUntilMs = nowMs + BikeMbAiConfig::kErrorDisplayMs;
  setDetail(&machine->snapshot, detail == nullptr ? "AI unavailable" : detail);
}

uint32_t cancelEffects() {
  return BIKE_MB_AI_EFFECT_CANCEL_AUDIO | BIKE_MB_AI_EFFECT_CANCEL_CLOUD;
}
}

void BikeMbAiStateMachine_Init(
    BikeMbAiStateMachine *machine,
    bool enabled,
    uint32_t nowMs) {
  if (machine == nullptr) {
    return;
  }
  *machine = {};
  machine->enabled = enabled;
  setState(machine, enabled ? BIKE_MB_AI_IDLE : BIKE_MB_AI_DISABLED, nowMs);
}

uint32_t BikeMbAiStateMachine_Dispatch(
    BikeMbAiStateMachine *machine,
    const BikeMbAiEvent &event) {
  if (machine == nullptr) {
    return BIKE_MB_AI_EFFECT_NONE;
  }

  if (isWorkerEvent(event.type) && event.requestId != machine->snapshot.requestId) {
    return BIKE_MB_AI_EFFECT_NONE;
  }

  switch (event.type) {
    case BIKE_MB_AI_EVENT_SET_ENABLED:
      machine->enabled = event.value;
      machine->buttonHeld = false;
      machine->releaseRequired = false;
      machine->deadlineMs = 0;
      if (!event.value) {
        ++machine->snapshot.requestId;
        setState(machine, BIKE_MB_AI_DISABLED, event.nowMs);
        return cancelEffects();
      }
      setState(machine, BIKE_MB_AI_IDLE, event.nowMs);
      return BIKE_MB_AI_EFFECT_NONE;

    case BIKE_MB_AI_EVENT_SET_WIFI:
      machine->snapshot.wifiConnected = event.value;
      if (!event.value && isActive(machine->snapshot.state)) {
        ++machine->snapshot.requestId;
        setError(machine, event.nowMs, "Wi-Fi disconnected");
        return cancelEffects();
      }
      return BIKE_MB_AI_EFFECT_NONE;

    case BIKE_MB_AI_EVENT_BUTTON_PRESSED: {
      machine->buttonHeld = true;
      if (!machine->enabled) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      if (machine->snapshot.state == BIKE_MB_AI_ERROR &&
          !elapsed(event.nowMs, machine->snapshot.stateSinceMs,
                   BikeMbAiConfig::kErrorDisplayMs)) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      const bool replacing = isActive(machine->snapshot.state);
      if (!machine->snapshot.wifiConnected) {
        ++machine->snapshot.requestId;
        setError(machine, event.nowMs, "Wi-Fi unavailable");
        return replacing ? cancelEffects() : BIKE_MB_AI_EFFECT_NONE;
      }
      ++machine->snapshot.requestId;
      machine->releaseRequired = false;
      machine->deadlineMs = 0;
      setState(machine, BIKE_MB_AI_RECORDING, event.nowMs);
      uint32_t effects = BIKE_MB_AI_EFFECT_START_CAPTURE;
      if (replacing) {
        effects |= cancelEffects();
      }
      return effects;
    }

    case BIKE_MB_AI_EVENT_BUTTON_RELEASED: {
      machine->buttonHeld = false;
      if (machine->snapshot.state == BIKE_MB_AI_ERROR && machine->releaseRequired) {
        machine->releaseRequired = false;
        return BIKE_MB_AI_EFFECT_NONE;
      }
      if (machine->snapshot.state != BIKE_MB_AI_RECORDING) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      const uint32_t recordingMs = event.nowMs - machine->snapshot.stateSinceMs;
      if (recordingMs < BikeMbAiConfig::kMinRecordingMs) {
        setState(machine, BIKE_MB_AI_IDLE, event.nowMs);
        return BIKE_MB_AI_EFFECT_CANCEL_AUDIO;
      }
      if (recordingMs >= BikeMbAiConfig::kMaxRecordingMs) {
        setError(machine, event.nowMs, "Recording limit reached");
        return BIKE_MB_AI_EFFECT_CANCEL_AUDIO;
      }
      machine->deadlineMs = event.nowMs + BikeMbAiConfig::kCloudDeadlineMs;
      setState(machine, BIKE_MB_AI_RECOGNIZING, event.nowMs);
      return BIKE_MB_AI_EFFECT_FINISH_CAPTURE | BIKE_MB_AI_EFFECT_SUBMIT_STT;
    }

    case BIKE_MB_AI_EVENT_STT_READY:
      if (machine->snapshot.state != BIKE_MB_AI_RECOGNIZING) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      if (deadlinePassed(event.nowMs, machine->deadlineMs)) {
        setError(machine, event.nowMs, "AI request timed out");
        return cancelEffects();
      }
      setState(machine, BIKE_MB_AI_THINKING, event.nowMs);
      return BIKE_MB_AI_EFFECT_SUBMIT_LLM;

    case BIKE_MB_AI_EVENT_LLM_READY:
      if (machine->snapshot.state != BIKE_MB_AI_THINKING) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      if (deadlinePassed(event.nowMs, machine->deadlineMs)) {
        setError(machine, event.nowMs, "AI request timed out");
        return cancelEffects();
      }
      setState(machine, BIKE_MB_AI_SYNTHESIZING, event.nowMs);
      return BIKE_MB_AI_EFFECT_SUBMIT_TTS;

    case BIKE_MB_AI_EVENT_TTS_STARTED:
      if (machine->snapshot.state != BIKE_MB_AI_SYNTHESIZING) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      if (deadlinePassed(event.nowMs, machine->deadlineMs)) {
        setError(machine, event.nowMs, "AI request timed out");
        return cancelEffects();
      }
      setState(machine, BIKE_MB_AI_SPEAKING, event.nowMs);
      return BIKE_MB_AI_EFFECT_NONE;

    case BIKE_MB_AI_EVENT_PLAYBACK_DONE:
      if (machine->snapshot.state == BIKE_MB_AI_SPEAKING) {
        machine->deadlineMs = 0;
        setState(machine, BIKE_MB_AI_IDLE, event.nowMs);
      }
      return BIKE_MB_AI_EFFECT_NONE;

    case BIKE_MB_AI_EVENT_CANCEL:
      if (machine->snapshot.state == BIKE_MB_AI_DISABLED) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      ++machine->snapshot.requestId;
      machine->buttonHeld = false;
      machine->releaseRequired = false;
      machine->deadlineMs = 0;
      setState(machine, BIKE_MB_AI_IDLE, event.nowMs);
      return cancelEffects();

    case BIKE_MB_AI_EVENT_FAILURE:
      if (!isActive(machine->snapshot.state)) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      setError(machine, event.nowMs, event.detail);
      return cancelEffects();

    case BIKE_MB_AI_EVENT_TICK:
      if (machine->snapshot.state == BIKE_MB_AI_RECORDING &&
          elapsed(event.nowMs, machine->snapshot.stateSinceMs,
                  BikeMbAiConfig::kMaxRecordingMs)) {
        machine->releaseRequired = machine->buttonHeld;
        setError(machine, event.nowMs, "Recording limit reached");
        return BIKE_MB_AI_EFFECT_CANCEL_AUDIO;
      }
      if (isCloudState(machine->snapshot.state) &&
          deadlinePassed(event.nowMs, machine->deadlineMs)) {
        setError(machine, event.nowMs, "AI request timed out");
        return cancelEffects();
      }
      if (machine->snapshot.state == BIKE_MB_AI_ERROR &&
          !machine->releaseRequired && !machine->buttonHeld &&
          elapsed(event.nowMs, machine->snapshot.stateSinceMs,
                  BikeMbAiConfig::kErrorDisplayMs)) {
        setState(machine, BIKE_MB_AI_IDLE, event.nowMs);
      }
      return BIKE_MB_AI_EFFECT_NONE;
  }

  return BIKE_MB_AI_EFFECT_NONE;
}
