#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum BikeMbAudioSessionOwner {
  BIKE_MB_AUDIO_SESSION_OWNER_NONE = 0,
  BIKE_MB_AUDIO_SESSION_OWNER_SELF_TEST,
  BIKE_MB_AUDIO_SESSION_OWNER_PROMPT,
  BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE,
  BIKE_MB_AUDIO_SESSION_OWNER_AI_PLAYBACK,
  BIKE_MB_AUDIO_SESSION_OWNER_MUSIC,
} BikeMbAudioSessionOwner;

typedef struct BikeMbAudioSessionState {
  BikeMbAudioSessionOwner owner;
  uint32_t requestId;
} BikeMbAudioSessionState;

void BikeMbAudioSessionCore_Init(BikeMbAudioSessionState *state);
bool BikeMbAudioSessionCore_Acquire(
    BikeMbAudioSessionState *state, BikeMbAudioSessionOwner owner, uint32_t requestId);
void BikeMbAudioSessionCore_Release(
    BikeMbAudioSessionState *state, BikeMbAudioSessionOwner owner, uint32_t requestId);
void BikeMbAudioSessionCore_ReleaseAll(BikeMbAudioSessionState *state);
BikeMbAudioSessionOwner BikeMbAudioSessionCore_GetOwner(const BikeMbAudioSessionState *state);
uint32_t BikeMbAudioSessionCore_GetRequestId(const BikeMbAudioSessionState *state);

#ifdef __cplusplus
}
#endif
