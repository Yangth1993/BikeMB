#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum BikeMbAudioSelfTestCommand {
  BIKE_MB_AUDIO_SELF_TEST_COMMAND_NONE = 0,
  BIKE_MB_AUDIO_SELF_TEST_COMMAND_NEXT_PAGE,
  BIKE_MB_AUDIO_SELF_TEST_COMMAND_PREVIOUS_PAGE,
} BikeMbAudioSelfTestCommand;

void BikeMbAudioSelfTest_Init(void);
void BikeMbAudioSelfTest_Tick(uint32_t nowMs);
void BikeMbAudioSelfTest_PlayPageTone(bool nextPage);
BikeMbAudioSelfTestCommand BikeMbAudioSelfTest_ConsumeCommand(void);

#ifdef __cplusplus
}
#endif
