#include "audio_self_test.h"

#ifndef BIKE_MB_ENABLE_AUDIO_SELF_TEST
#define BIKE_MB_ENABLE_AUDIO_SELF_TEST 0
#endif

#ifndef BIKE_MB_AUDIO_SELF_TEST_DISABLE_MIC
#define BIKE_MB_AUDIO_SELF_TEST_DISABLE_MIC 0
#endif

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && BIKE_MB_ENABLE_AUDIO_SELF_TEST

#include <Arduino.h>

#include "audio_session.h"

namespace {

constexpr uint32_t kSampleRate = 16000;
constexpr uint16_t kSamplesPerChunk = 96;
constexpr uint32_t kMicReportIntervalMs = 1000;
constexpr uint32_t kSelfTestRequestId = 1;
constexpr uint32_t kMicTaskStackBytes = 4096;
constexpr UBaseType_t kMicTaskPriority = 1;
constexpr BaseType_t kMicTaskCore = 0;

bool g_audioReady = false;
BikeMbAudioSelfTestCommand g_pendingCommand = BIKE_MB_AUDIO_SELF_TEST_COMMAND_NONE;
TaskHandle_t g_micTask = nullptr;

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
    BikeMbAudioSession_WriteStereoPcm(
        BIKE_MB_AUDIO_SESSION_OWNER_SELF_TEST, kSelfTestRequestId, samples, chunkSamples);
    writtenSamples += chunkSamples;
  }
}

void reportMicLevel() {
  if (!g_audioReady) {
    return;
  }

  int16_t samples[128] = {};
  const size_t bytesRead = BikeMbAudioSession_ReadMicBytes(
      BIKE_MB_AUDIO_SESSION_OWNER_SELF_TEST, kSelfTestRequestId, samples, sizeof(samples));
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

void BikeMbAudioSelfTest_MicTask(void *param) {
  (void)param;
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(kMicReportIntervalMs));
    reportMicLevel();
  }
}

}  // namespace

void BikeMbAudioSelfTest_Init(void) {
  Serial.println("[BikeMB][audio] self-test enabled");

  g_audioReady =
      BikeMbAudioSession_Acquire(BIKE_MB_AUDIO_SESSION_OWNER_SELF_TEST, kSelfTestRequestId);
  if (!g_audioReady) {
    Serial.println("[BikeMB][audio] self-test audio session unavailable");
    return;
  }

  Serial.println("[BikeMB][audio] self-test audio session ready");
  writeTone(880, 120);
#if !BIKE_MB_AUDIO_SELF_TEST_DISABLE_MIC
  if (xTaskCreatePinnedToCore(
          BikeMbAudioSelfTest_MicTask,
          "bikemb-audio-mic",
          kMicTaskStackBytes,
          nullptr,
          kMicTaskPriority,
          &g_micTask,
          kMicTaskCore) != pdPASS) {
    Serial.println("[BikeMB][audio] mic task create failed");
  }
#endif
}

void BikeMbAudioSelfTest_Tick(uint32_t nowMs) {
  (void)nowMs;
  readSerialCommand();
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
