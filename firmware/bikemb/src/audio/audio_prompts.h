#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum BikeMbAudioPromptMode {
  BIKE_MB_AUDIO_PROMPT_MODE_ECO = 0,
  BIKE_MB_AUDIO_PROMPT_MODE_TRAIL = 1,
  BIKE_MB_AUDIO_PROMPT_MODE_BOOST = 2,
} BikeMbAudioPromptMode;

void BikeMbAudioPrompts_Init(void);
void BikeMbAudioPrompts_PlayMode(BikeMbAudioPromptMode mode);

#ifdef __cplusplus
}
#endif
