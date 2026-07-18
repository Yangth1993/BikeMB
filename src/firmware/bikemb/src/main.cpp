#if defined(BIKE_MB_USE_ESPIDF_RUNTIME)
#include "esp_log.h"
#else
#include <Arduino.h>
#endif

#include "audio/audio_prompts.h"
#include "audio/audio_self_test.h"
#include "app/dashboard_app.h"
#include "app/display_diagnostics.h"
#include "platform/board_support.h"
#include "platform/bike_platform.h"
#include "platform/lvgl_port.h"
#include "runtime/bike_runtime.h"
#include "voice/voice_commands.h"

#ifndef BIKE_MB_RUN_DISPLAY_DIAGNOSTIC
#define BIKE_MB_RUN_DISPLAY_DIAGNOSTIC 0
#endif

#ifndef BIKE_MB_ENABLE_AUDIO_SELF_TEST
#define BIKE_MB_ENABLE_AUDIO_SELF_TEST 0
#endif

#ifndef BIKE_MB_ENABLE_VOICE_COMMANDS
#define BIKE_MB_ENABLE_VOICE_COMMANDS 0
#endif

#ifndef BIKE_MB_ENABLE_AUDIO_PROMPTS
#define BIKE_MB_ENABLE_AUDIO_PROMPTS 0
#endif

namespace {
uint32_t g_lastTickMs = 0;
constexpr const char *TAG = "BikeMB.Main";

void HandleModeChanged(uint8_t modeIndex) {
#if BIKE_MB_ENABLE_AUDIO_PROMPTS
  BikeMbAudioPrompts_PlayMode(static_cast<BikeMbAudioPromptMode>(modeIndex));
#else
  (void)modeIndex;
#endif
}

void HandleAudioSelfTestCommand() {
  const BikeMbAudioSelfTestCommand command = BikeMbAudioSelfTest_ConsumeCommand();
  if (command == BIKE_MB_AUDIO_SELF_TEST_COMMAND_NEXT_PAGE) {
    Serial.println("[BikeMB][audio] routed command: next page");
    DashboardApp_NextPage();
    BikeMbAudioSelfTest_PlayPageTone(true);
  } else if (command == BIKE_MB_AUDIO_SELF_TEST_COMMAND_PREVIOUS_PAGE) {
    Serial.println("[BikeMB][audio] routed command: previous page");
    DashboardApp_PreviousPage();
    BikeMbAudioSelfTest_PlayPageTone(false);
  }
}

void HandleVoiceCommand() {
  const BikeMbVoiceCommand command = BikeMbVoiceCommands_ConsumeCommand();
  if (command == BIKE_MB_VOICE_COMMAND_NEXT_PAGE) {
    Serial.println("[BikeMB][voice] routed command: next page");
    DashboardApp_NextPage();
  } else if (command == BIKE_MB_VOICE_COMMAND_PREVIOUS_PAGE) {
    Serial.println("[BikeMB][voice] routed command: previous page");
    DashboardApp_PreviousPage();
  }
}
}

#if defined(BIKE_MB_USE_ESPIDF_RUNTIME)
extern "C" void app_main() {
  ESP_LOGI(TAG, "bikemb ESP-IDF runtime boot");

#if BIKE_MB_RUN_DISPLAY_DIAGNOSTIC
  BoardSupport_Init();
  DisplayDiagnostics_Run();
  while (true) {
    BikePlatform_DelayMs(1000);
  }
#else
  BikeRuntime_Init();
  BikeRuntime_Start();
  ESP_LOGI(TAG, "BikeMB runtime started");
#endif
}
#else
void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("[BikeMB] bikemb lvgl demo boot");

  BoardSupport_Init();
  BikeMbAudioPrompts_Init();
  BikeMbAudioSelfTest_Init();
  BikeMbVoiceCommands_Init();
#if BIKE_MB_RUN_DISPLAY_DIAGNOSTIC
  DisplayDiagnostics_Run();
#else
  LvglPort_Init();
  DashboardApp_Init();
  DashboardApp_SetModeChangedCallback(HandleModeChanged);

  g_lastTickMs = millis();
  Serial.println("[BikeMB] lvgl dashboard ready");
#endif
}

void loop() {
#if BIKE_MB_RUN_DISPLAY_DIAGNOSTIC
  delay(1000);
#else
  const uint32_t now = millis();
  const uint32_t deltaMs = now - g_lastTickMs;
  g_lastTickMs = now;

  LvglPort_Tick(deltaMs);
  DashboardApp_Tick(now);
  DashboardApp_SetRenderWorkMs(LvglPort_Run());
  BikeMbAudioSelfTest_Tick(now);
  HandleAudioSelfTestCommand();
  HandleVoiceCommand();

  delay(5);
#endif
}
#endif
