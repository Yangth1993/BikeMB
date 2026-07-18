#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum BikeMbAiState {
  BIKE_MB_AI_DISABLED = 0,
  BIKE_MB_AI_IDLE,
  BIKE_MB_AI_RECORDING,
  BIKE_MB_AI_RECOGNIZING,
  BIKE_MB_AI_THINKING,
  BIKE_MB_AI_SYNTHESIZING,
  BIKE_MB_AI_SPEAKING,
  BIKE_MB_AI_CONNECTING_MUSIC,
  BIKE_MB_AI_MUSIC_PLAYING,
  BIKE_MB_AI_ERROR,
} BikeMbAiState;

typedef struct BikeMbAiSnapshot {
  BikeMbAiState state;
  uint32_t requestId;
  uint32_t stateSinceMs;
  bool wifiConnected;
  bool cancelAvailable;
  char detail[96];
} BikeMbAiSnapshot;

typedef enum BikeMbAiEventType {
  BIKE_MB_AI_EVENT_SET_ENABLED = 0,
  BIKE_MB_AI_EVENT_SET_WIFI,
  BIKE_MB_AI_EVENT_BUTTON_PRESSED,
  BIKE_MB_AI_EVENT_BUTTON_RELEASED,
  BIKE_MB_AI_EVENT_STT_READY,
  BIKE_MB_AI_EVENT_LLM_READY,
  BIKE_MB_AI_EVENT_TTS_STARTED,
  BIKE_MB_AI_EVENT_PLAYBACK_DONE,
  BIKE_MB_AI_EVENT_CANCEL,
  BIKE_MB_AI_EVENT_FAILURE,
  BIKE_MB_AI_EVENT_TICK,
} BikeMbAiEventType;

typedef struct BikeMbAiEvent {
  BikeMbAiEventType type;
  uint32_t nowMs;
  uint32_t requestId;
  bool value;
  const char *detail;
} BikeMbAiEvent;

enum BikeMbAiEffect : uint32_t {
  BIKE_MB_AI_EFFECT_NONE = 0,
  BIKE_MB_AI_EFFECT_START_CAPTURE = 1U << 0,
  BIKE_MB_AI_EFFECT_FINISH_CAPTURE = 1U << 1,
  BIKE_MB_AI_EFFECT_CANCEL_AUDIO = 1U << 2,
  BIKE_MB_AI_EFFECT_CANCEL_CLOUD = 1U << 3,
  BIKE_MB_AI_EFFECT_SUBMIT_STT = 1U << 4,
  BIKE_MB_AI_EFFECT_SUBMIT_LLM = 1U << 5,
  BIKE_MB_AI_EFFECT_SUBMIT_TTS = 1U << 6,
};
