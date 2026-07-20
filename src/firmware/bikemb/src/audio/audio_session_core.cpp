#include "audio_session_core.h"

void BikeMbAudioSessionCore_Init(BikeMbAudioSessionState *state) {
  if (state == nullptr) {
    return;
  }

  state->owner = BIKE_MB_AUDIO_SESSION_OWNER_NONE;
  state->requestId = 0;
}

bool BikeMbAudioSessionCore_Acquire(
    BikeMbAudioSessionState *state, BikeMbAudioSessionOwner owner, uint32_t requestId) {
  if (state == nullptr || owner == BIKE_MB_AUDIO_SESSION_OWNER_NONE) {
    return false;
  }

  if (state->owner == BIKE_MB_AUDIO_SESSION_OWNER_NONE) {
    state->owner = owner;
    state->requestId = requestId;
    return true;
  }

  return state->owner == owner && state->requestId == requestId;
}

void BikeMbAudioSessionCore_Release(
    BikeMbAudioSessionState *state, BikeMbAudioSessionOwner owner, uint32_t requestId) {
  if (state == nullptr) {
    return;
  }

  if (state->owner != owner || state->requestId != requestId) {
    return;
  }

  BikeMbAudioSessionCore_ReleaseAll(state);
}

void BikeMbAudioSessionCore_ReleaseAll(BikeMbAudioSessionState *state) {
  if (state == nullptr) {
    return;
  }

  state->owner = BIKE_MB_AUDIO_SESSION_OWNER_NONE;
  state->requestId = 0;
}

BikeMbAudioSessionOwner BikeMbAudioSessionCore_GetOwner(const BikeMbAudioSessionState *state) {
  if (state == nullptr) {
    return BIKE_MB_AUDIO_SESSION_OWNER_NONE;
  }

  return state->owner;
}

uint32_t BikeMbAudioSessionCore_GetRequestId(const BikeMbAudioSessionState *state) {
  if (state == nullptr) {
    return 0;
  }

  return state->requestId;
}
