#include "audio_prompts.h"

#ifndef BIKE_MB_ENABLE_AUDIO_PROMPTS
#define BIKE_MB_ENABLE_AUDIO_PROMPTS 0
#endif

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && BIKE_MB_ENABLE_AUDIO_PROMPTS

#include <Arduino.h>
#include "ESP_I2S.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../drivers/I2C_Driver.h"
#include "audio_prompt_assets.h"

namespace {

constexpr uint8_t kEs8311Address = 0x18;
constexpr uint32_t kSampleRate = 16000;
constexpr uint16_t kSamplesPerChunk = 96;

I2SClass g_i2s(I2S_NUM_0);
bool g_audioReady = false;
TaskHandle_t g_promptTask = nullptr;
portMUX_TYPE g_promptMux = portMUX_INITIALIZER_UNLOCKED;
BikeMbAudioPromptMode g_requestedMode = BIKE_MB_AUDIO_PROMPT_MODE_ECO;
uint32_t g_requestSerial = 0;

bool writeRegister(uint8_t reg, uint8_t value) {
  return !I2C_Write(kEs8311Address, reg, &value, 1);
}

bool initSpeakerCodec() {
  bool ok = true;
  ok &= writeRegister(0x00, 0x1F);
  delay(20);
  ok &= writeRegister(0x00, 0x00);
  ok &= writeRegister(0x00, 0x80);

  ok &= writeRegister(0x01, 0x3F);
  ok &= writeRegister(0x02, 0x00);
  ok &= writeRegister(0x03, 0x10);
  ok &= writeRegister(0x04, 0x10);
  ok &= writeRegister(0x05, 0x00);
  ok &= writeRegister(0x06, 0x03);
  ok &= writeRegister(0x07, 0x00);
  ok &= writeRegister(0x08, 0xFF);
  ok &= writeRegister(0x09, 0x0C);
  ok &= writeRegister(0x0A, 0x0C);

  ok &= writeRegister(0x0D, 0x01);
  ok &= writeRegister(0x0E, 0x02);
  ok &= writeRegister(0x12, 0x00);
  ok &= writeRegister(0x13, 0x10);
  ok &= writeRegister(0x14, 0x1A);
  ok &= writeRegister(0x17, 0xC8);
  ok &= writeRegister(0x1C, 0x6A);
  ok &= writeRegister(0x32, 0x7F);
  ok &= writeRegister(0x37, 0x08);
  return ok;
}

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

  int16_t stereo[kSamplesPerChunk * 2];
  uint32_t writtenSamples = 0;
  while (writtenSamples < sampleCount) {
    if (expectedSerial != getRequestSerial()) {
      return;
    }

    const uint16_t chunkSamples =
        static_cast<uint16_t>(min<uint32_t>(kSamplesPerChunk, sampleCount - writtenSamples));
    for (uint16_t i = 0; i < chunkSamples; ++i) {
      const int16_t sample = samples[writtenSamples + i];
      stereo[i * 2] = sample;
      stereo[i * 2 + 1] = sample;
    }
    g_i2s.write(reinterpret_cast<const uint8_t *>(stereo), chunkSamples * 2 * sizeof(stereo[0]));
    writtenSamples += chunkSamples;
  }
}

void playRequestedPrompt(BikeMbAudioPromptMode mode, uint32_t requestSerial) {
  if (mode == BIKE_MB_AUDIO_PROMPT_MODE_ECO) {
    writePrompt(kBikeMbPromptEcoPcm, kBikeMbPromptEcoPcmSampleCount, requestSerial);
  } else if (mode == BIKE_MB_AUDIO_PROMPT_MODE_TRAIL) {
    writePrompt(kBikeMbPromptTrailPcm, kBikeMbPromptTrailPcmSampleCount, requestSerial);
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

  pinMode(GPIO_NUM_15, OUTPUT);
  digitalWrite(GPIO_NUM_15, HIGH);

  if (!initSpeakerCodec()) {
    Serial.println("[BikeMB][audio] ES8311 prompt init failed");
    return;
  }

  g_i2s.setPins(GPIO_NUM_48, GPIO_NUM_38, GPIO_NUM_47, GPIO_NUM_NC, GPIO_NUM_2);
  g_audioReady =
      g_i2s.begin(I2S_MODE_STD, kSampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  if (!g_audioReady) {
    Serial.println("[BikeMB][audio] prompt I2S init failed");
    return;
  }

  if (xTaskCreate(promptTask, "bikemb_prompt", 4096, nullptr, 1, &g_promptTask) != pdPASS) {
    g_promptTask = nullptr;
    Serial.println("[BikeMB][audio] prompt task create failed");
    return;
  }

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
