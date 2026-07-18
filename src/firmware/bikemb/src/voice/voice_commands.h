#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum BikeMbVoiceCommand {
  BIKE_MB_VOICE_COMMAND_NONE = 0,
  BIKE_MB_VOICE_COMMAND_NEXT_PAGE,
  BIKE_MB_VOICE_COMMAND_PREVIOUS_PAGE,
} BikeMbVoiceCommand;

void BikeMbVoiceCommands_Init(void);
BikeMbVoiceCommand BikeMbVoiceCommands_ConsumeCommand(void);

#ifdef __cplusplus
}
#endif
