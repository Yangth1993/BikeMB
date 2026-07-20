#include <cassert>
#include <cstdint>

#include "../../src/firmware/bikemb/src/audio/audio_session_core.h"

int main() {
  BikeMbAudioSessionState state;
  BikeMbAudioSessionCore_Init(&state);

  assert(BikeMbAudioSessionCore_GetOwner(&state) == BIKE_MB_AUDIO_SESSION_OWNER_NONE);
  assert(BikeMbAudioSessionCore_GetRequestId(&state) == 0);

  assert(!BikeMbAudioSessionCore_Acquire(&state, BIKE_MB_AUDIO_SESSION_OWNER_NONE, 1));
  assert(BikeMbAudioSessionCore_GetOwner(&state) == BIKE_MB_AUDIO_SESSION_OWNER_NONE);

  assert(BikeMbAudioSessionCore_Acquire(&state, BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE, 42));
  assert(BikeMbAudioSessionCore_GetOwner(&state) == BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE);
  assert(BikeMbAudioSessionCore_GetRequestId(&state) == 42);

  assert(BikeMbAudioSessionCore_Acquire(&state, BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE, 42));
  assert(!BikeMbAudioSessionCore_Acquire(&state, BIKE_MB_AUDIO_SESSION_OWNER_PROMPT, 43));
  assert(BikeMbAudioSessionCore_GetOwner(&state) == BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE);

  BikeMbAudioSessionCore_Release(&state, BIKE_MB_AUDIO_SESSION_OWNER_PROMPT, 42);
  assert(BikeMbAudioSessionCore_GetOwner(&state) == BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE);

  BikeMbAudioSessionCore_Release(&state, BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE, 41);
  assert(BikeMbAudioSessionCore_GetOwner(&state) == BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE);

  BikeMbAudioSessionCore_Release(&state, BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE, 42);
  assert(BikeMbAudioSessionCore_GetOwner(&state) == BIKE_MB_AUDIO_SESSION_OWNER_NONE);
  assert(BikeMbAudioSessionCore_GetRequestId(&state) == 0);

  assert(BikeMbAudioSessionCore_Acquire(&state, BIKE_MB_AUDIO_SESSION_OWNER_MUSIC, 77));
  BikeMbAudioSessionCore_ReleaseAll(&state);
  assert(BikeMbAudioSessionCore_GetOwner(&state) == BIKE_MB_AUDIO_SESSION_OWNER_NONE);
  assert(BikeMbAudioSessionCore_GetRequestId(&state) == 0);

  return 0;
}
