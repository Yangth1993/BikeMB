#include "audio_prompts.h"

#ifndef BIKE_MB_ENABLE_AUDIO_PROMPTS
#define BIKE_MB_ENABLE_AUDIO_PROMPTS 0
#endif

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && BIKE_MB_ENABLE_AUDIO_PROMPTS

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "audio_session.h"
#include "audio_prompt_assets.h"

namespace {

constexpr uint32_t kSampleRate = 16000;
constexpr uint16_t kSamplesPerChunk = 96;

bool g_audioReady = false;
TaskHandle_t g_promptTask = nullptr;
portMUX_TYPE g_promptMux = portMUX_INITIALIZER_UNLOCKED;
BikeMbAudioPromptMode g_requestedMode = BIKE_MB_AUDIO_PROMPT_MODE_ECO;
uint32_t g_requestSerial = 0;

uint32_t getRequestSerial() {
  portENTER_CRITICAL(&g_promptMux);
  const uint32_t requestSerial = g_requestSerial;
  portEXIT_CRITICAL(&g_promptMux);
  return requestSerial;
}

void getRequest(BikeMbAudioPromptMode *mode, uint32_t *requestSerial) {
  portENTER_CRITICAL(&g_promptMux);
  *mode = g_requestedMode;
  *requestSerial = g_requestSerial;
  portEXIT_CRITICAL(&g_promptMux);
}

uint32_t setRequest(BikeMbAudioPromptMode mode) {
  portENTER_CRITICAL(&g_promptMux);
  g_requestedMode = mode;
  const uint32_t requestSerial = ++g_requestSerial;
  portEXIT_CRITICAL(&g_promptMux);
  return requestSerial;
}

void writePrompt(const int16_t *samples, uint32_t sampleCount, uint32_t expectedSerial) {
  if (!g_audioReady || samples == nullptr || sampleCount == 0) {
    return;
  }

  if (!BikeMbAudioSession_Acquire(BIKE_MB_AUDIO_SESSION_OWNER_PROMPT, expectedSerial)) {
    Serial.println("[BikeMB][audio] prompt audio session busy");
    return;
  }

  int16_t stereo[kSamplesPerChunk * 2];
  uint32_t writtenSamples = 0;
  while (writtenSamples < sampleCount) {
    if (expectedSerial != getRequestSerial()) {
      BikeMbAudioSession_Release(BIKE_MB_AUDIO_SESSION_OWNER_PROMPT, expectedSerial);
      return;
    }

    const uint16_t chunkSamples =
        static_cast<uint16_t>(min<uint32_t>(kSamplesPerChunk, sampleCount - writtenSamples));
    for (uint16_t i = 0; i < chunkSamples; ++i) {
      const int16_t sample = samples[writtenSamples + i];
      stereo[i * 2] = sample;
      stereo[i * 2 + 1] = sample;
    }
    BikeMbAudioSession_WriteStereoPcm(
        BIKE_MB_AUDIO_SESSION_OWNER_PROMPT, expectedSerial, stereo, chunkSamples);
    writtenSamples += chunkSamples;
  }

  BikeMbAudioSession_Release(BIKE_MB_AUDIO_SESSION_OWNER_PROMPT, expectedSerial);
}

void playRequestedPrompt(BikeMbAudioPromptMode mode, uint32_t requestSerial) {
  if (mode == BIKE_MB_AUDIO_PROMPT_MODE_ECO) {
    writePrompt(kBikeMbPromptEcoPcm, kBikeMbPromptEcoPcmSampleCount, requestSerial);
  } else if (mode == BIKE_MB_AUDIO_PROMPT_MODE_TRAIL) {
    writePrompt(kBikeMbPromptTrailPcm, kBikeMbPromptTrailPcmSampleCount, requestSerial);
  } else if (mode == BIKE_MB_AUDIO_PROMPT_MODE_AUTO) {
    return;
  } else if (mode == BIKE_MB_AUDIO_PROMPT_MODE_BOOST) {
    writePrompt(kBikeMbPromptBoostPcm, kBikeMbPromptBoostPcmSampleCount, requestSerial);
  }
}

void promptTask(void *parameter) {
  (void)parameter;

  uint32_t handledSerial = 0;
  for (;;) {
    uint32_t notifiedSerial = 0;
    xTaskNotifyWait(0, UINT32_MAX, &notifiedSerial, portMAX_DELAY);
    if (notifiedSerial <= handledSerial) {
      continue;
    }

    for (;;) {
      BikeMbAudioPromptMode mode = BIKE_MB_AUDIO_PROMPT_MODE_ECO;
      uint32_t requestSerial = 0;
      getRequest(&mode, &requestSerial);
      if (requestSerial <= handledSerial) {
        break;
      }

      playRequestedPrompt(mode, requestSerial);
      if (requestSerial == getRequestSerial()) {
        handledSerial = requestSerial;
        break;
      }
    }
  }
}

}  // namespace

void BikeMbAudioPrompts_Init(void) {
  Serial.println("[BikeMB][audio] mode prompts enabled");

  if (xTaskCreate(promptTask, "bikemb_prompt", 4096, nullptr, 1, &g_promptTask) != pdPASS) {
    g_promptTask = nullptr;
    Serial.println("[BikeMB][audio] prompt task create failed");
    return;
  }

  g_audioReady = true;
  Serial.println("[BikeMB][audio] mode prompts ready");
}

void BikeMbAudioPrompts_PlayMode(BikeMbAudioPromptMode mode) {
  if (!g_audioReady || g_promptTask == nullptr) {
    return;
  }

  const uint32_t requestSerial = setRequest(mode);
  xTaskNotify(g_promptTask, requestSerial, eSetValueWithOverwrite);
}

#else

void BikeMbAudioPrompts_Init(void) {}
void BikeMbAudioPrompts_PlayMode(BikeMbAudioPromptMode mode) {
  (void)mode;
}

#endif
