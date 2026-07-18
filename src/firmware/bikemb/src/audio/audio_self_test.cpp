#include "audio_self_test.h"

#ifndef BIKE_MB_ENABLE_AUDIO_SELF_TEST
#define BIKE_MB_ENABLE_AUDIO_SELF_TEST 0
#endif

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && BIKE_MB_ENABLE_AUDIO_SELF_TEST

#include <Arduino.h>
#include "ESP_I2S.h"
#include "driver/gpio.h"

#include "../drivers/I2C_Driver.h"

namespace {

constexpr uint8_t kEs8311Address = 0x18;
constexpr uint8_t kEs7210Address = 0x40;
constexpr uint32_t kSampleRate = 16000;
constexpr uint16_t kSamplesPerChunk = 96;
constexpr uint32_t kMicReportIntervalMs = 1000;

I2SClass g_i2s(I2S_NUM_0);
bool g_audioReady = false;
uint32_t g_lastMicReportMs = 0;
BikeMbAudioSelfTestCommand g_pendingCommand = BIKE_MB_AUDIO_SELF_TEST_COMMAND_NONE;

bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
  return !I2C_Write(address, reg, &value, 1);
}

bool initSpeakerCodec() {
  bool ok = true;
  ok &= writeRegister(kEs8311Address, 0x00, 0x1F);
  delay(20);
  ok &= writeRegister(kEs8311Address, 0x00, 0x00);
  ok &= writeRegister(kEs8311Address, 0x00, 0x80);

  ok &= writeRegister(kEs8311Address, 0x01, 0x3F);
  ok &= writeRegister(kEs8311Address, 0x02, 0x00);
  ok &= writeRegister(kEs8311Address, 0x03, 0x10);
  ok &= writeRegister(kEs8311Address, 0x04, 0x10);
  ok &= writeRegister(kEs8311Address, 0x05, 0x00);
  ok &= writeRegister(kEs8311Address, 0x06, 0x03);
  ok &= writeRegister(kEs8311Address, 0x07, 0x00);
  ok &= writeRegister(kEs8311Address, 0x08, 0xFF);
  ok &= writeRegister(kEs8311Address, 0x09, 0x0C);
  ok &= writeRegister(kEs8311Address, 0x0A, 0x0C);

  ok &= writeRegister(kEs8311Address, 0x0D, 0x01);
  ok &= writeRegister(kEs8311Address, 0x0E, 0x02);
  ok &= writeRegister(kEs8311Address, 0x12, 0x00);
  ok &= writeRegister(kEs8311Address, 0x13, 0x10);
  ok &= writeRegister(kEs8311Address, 0x14, 0x1A);
  ok &= writeRegister(kEs8311Address, 0x17, 0xC8);
  ok &= writeRegister(kEs8311Address, 0x1C, 0x6A);
  ok &= writeRegister(kEs8311Address, 0x32, 0x7F);
  ok &= writeRegister(kEs8311Address, 0x37, 0x08);
  return ok;
}

bool initMicrophoneCodec() {
  bool ok = true;
  ok &= writeRegister(kEs7210Address, 0x00, 0xFF);
  ok &= writeRegister(kEs7210Address, 0x00, 0x32);
  ok &= writeRegister(kEs7210Address, 0x09, 0x30);
  ok &= writeRegister(kEs7210Address, 0x0A, 0x30);
  ok &= writeRegister(kEs7210Address, 0x23, 0x2A);
  ok &= writeRegister(kEs7210Address, 0x22, 0x0A);
  ok &= writeRegister(kEs7210Address, 0x21, 0x2A);
  ok &= writeRegister(kEs7210Address, 0x20, 0x0A);
  ok &= writeRegister(kEs7210Address, 0x11, 0x60);
  ok &= writeRegister(kEs7210Address, 0x12, 0x00);
  ok &= writeRegister(kEs7210Address, 0x40, 0xC3);
  ok &= writeRegister(kEs7210Address, 0x41, 0x70);
  ok &= writeRegister(kEs7210Address, 0x42, 0x70);
  ok &= writeRegister(kEs7210Address, 0x43, 0x1D);
  ok &= writeRegister(kEs7210Address, 0x44, 0x1D);
  ok &= writeRegister(kEs7210Address, 0x45, 0x1D);
  ok &= writeRegister(kEs7210Address, 0x46, 0x1D);
  ok &= writeRegister(kEs7210Address, 0x47, 0x08);
  ok &= writeRegister(kEs7210Address, 0x48, 0x08);
  ok &= writeRegister(kEs7210Address, 0x49, 0x08);
  ok &= writeRegister(kEs7210Address, 0x4A, 0x08);
  ok &= writeRegister(kEs7210Address, 0x07, 0x20);
  ok &= writeRegister(kEs7210Address, 0x02, 0xC1);
  ok &= writeRegister(kEs7210Address, 0x04, 0x01);
  ok &= writeRegister(kEs7210Address, 0x05, 0x00);
  ok &= writeRegister(kEs7210Address, 0x06, 0x04);
  ok &= writeRegister(kEs7210Address, 0x4B, 0x0F);
  ok &= writeRegister(kEs7210Address, 0x4C, 0x0F);
  ok &= writeRegister(kEs7210Address, 0x00, 0x71);
  ok &= writeRegister(kEs7210Address, 0x00, 0x41);
  ok &= writeRegister(kEs7210Address, 0x1B, 0xFF);
  ok &= writeRegister(kEs7210Address, 0x1C, 0xFF);
  ok &= writeRegister(kEs7210Address, 0x1D, 0xFF);
  ok &= writeRegister(kEs7210Address, 0x1E, 0xFF);
  return ok;
}

void writeTone(uint16_t frequencyHz, uint16_t durationMs) {
  if (!g_audioReady) {
    return;
  }

  int16_t samples[kSamplesPerChunk * 2];
  const uint32_t totalSamples = (kSampleRate * durationMs) / 1000;
  const uint32_t halfPeriod = kSampleRate / (frequencyHz * 2);
  uint32_t writtenSamples = 0;

  while (writtenSamples < totalSamples) {
    const uint16_t chunkSamples =
        static_cast<uint16_t>(min<uint32_t>(kSamplesPerChunk, totalSamples - writtenSamples));
    for (uint16_t i = 0; i < chunkSamples; ++i) {
      const uint32_t phase = ((writtenSamples + i) / halfPeriod) & 1;
      samples[i * 2] = phase ? 7000 : -7000;
      samples[i * 2 + 1] = samples[i * 2];
    }
    g_i2s.write(reinterpret_cast<const uint8_t *>(samples), chunkSamples * 2 * sizeof(samples[0]));
    writtenSamples += chunkSamples;
  }
}

void reportMicLevel() {
  if (!g_audioReady) {
    return;
  }

  int16_t samples[128] = {};
  const size_t bytesRead = g_i2s.readBytes(reinterpret_cast<char *>(samples), sizeof(samples));
  if (bytesRead == 0) {
    Serial.println("[BikeMB][audio] mic rms unavailable");
    return;
  }

  uint64_t sumSquares = 0;
  const size_t sampleCount = bytesRead / sizeof(samples[0]);
  for (size_t i = 0; i < sampleCount; ++i) {
    const int32_t sample = samples[i];
    sumSquares += static_cast<uint64_t>(sample * sample);
  }

  const uint32_t meanSquare = static_cast<uint32_t>(sumSquares / sampleCount);
  Serial.print("[BikeMB][audio] mic mean-square=");
  Serial.println(meanSquare);
}

void readSerialCommand() {
  while (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());
    if (command == 'n' || command == 'N') {
      g_pendingCommand = BIKE_MB_AUDIO_SELF_TEST_COMMAND_NEXT_PAGE;
      Serial.println("[BikeMB][audio] simulated command: next page");
    } else if (command == 'p' || command == 'P') {
      g_pendingCommand = BIKE_MB_AUDIO_SELF_TEST_COMMAND_PREVIOUS_PAGE;
      Serial.println("[BikeMB][audio] simulated command: previous page");
    }
  }
}

}  // namespace

void BikeMbAudioSelfTest_Init(void) {
  Serial.println("[BikeMB][audio] self-test enabled");

  pinMode(GPIO_NUM_15, OUTPUT);
  digitalWrite(GPIO_NUM_15, HIGH);

  if (!initSpeakerCodec()) {
    Serial.println("[BikeMB][audio] ES8311 init failed");
    return;
  }
  if (!initMicrophoneCodec()) {
    Serial.println("[BikeMB][audio] ES7210 init failed");
    return;
  }

  g_i2s.setPins(GPIO_NUM_48, GPIO_NUM_38, GPIO_NUM_47, GPIO_NUM_39, GPIO_NUM_2);
  g_audioReady =
      g_i2s.begin(I2S_MODE_STD, kSampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  if (!g_audioReady) {
    Serial.println("[BikeMB][audio] I2S init failed");
    return;
  }

  Serial.println("[BikeMB][audio] I2S ready");
  writeTone(880, 120);
}

void BikeMbAudioSelfTest_Tick(uint32_t nowMs) {
  readSerialCommand();
  if (nowMs - g_lastMicReportMs < kMicReportIntervalMs) {
    return;
  }
  g_lastMicReportMs = nowMs;
  reportMicLevel();
}

void BikeMbAudioSelfTest_PlayPageTone(bool nextPage) {
  writeTone(nextPage ? 1040 : 660, 70);
}

BikeMbAudioSelfTestCommand BikeMbAudioSelfTest_ConsumeCommand(void) {
  const BikeMbAudioSelfTestCommand command = g_pendingCommand;
  g_pendingCommand = BIKE_MB_AUDIO_SELF_TEST_COMMAND_NONE;
  return command;
}

#else

void BikeMbAudioSelfTest_Init(void) {}
void BikeMbAudioSelfTest_Tick(uint32_t nowMs) {
  (void)nowMs;
}
void BikeMbAudioSelfTest_PlayPageTone(bool nextPage) {
  (void)nextPage;
}
BikeMbAudioSelfTestCommand BikeMbAudioSelfTest_ConsumeCommand(void) {
  return BIKE_MB_AUDIO_SELF_TEST_COMMAND_NONE;
}

#endif
