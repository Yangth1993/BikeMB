#include "audio_session.h"

#ifndef BIKE_MB_ENABLE_AUDIO_SESSION
#define BIKE_MB_ENABLE_AUDIO_SESSION 0
#endif

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && BIKE_MB_ENABLE_AUDIO_SESSION

#include <Arduino.h>
#include "ESP_I2S.h"
#include "driver/gpio.h"

#include "../drivers/I2C_Driver.h"
#include "audio_capture_core.h"

namespace {

constexpr uint8_t kEs8311Address = 0x18;
constexpr uint8_t kEs7210Address = 0x40;
constexpr uint32_t kSampleRate = 16000;
constexpr uint32_t kAudioSessionReadTimeoutMs = 10;
constexpr uint32_t kAudioSessionWriteTimeoutMs = 250;
constexpr size_t kCaptureStereoFramesPerRead = 128;

I2SClass g_i2s(I2S_NUM_0);
BikeMbAudioSessionState g_state;
bool g_ready = false;
BikeMbAudioClip g_capture = {};
uint32_t g_captureRequestId = 0;
bool g_captureActive = false;

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

}  // namespace

void BikeMbAudioSession_Init(void) {
  BikeMbAudioSessionCore_Init(&g_state);
  Serial.println("[BikeMB][audio] session enabled");

  pinMode(GPIO_NUM_15, OUTPUT);
  digitalWrite(GPIO_NUM_15, HIGH);

  if (!initSpeakerCodec()) {
    Serial.println("[BikeMB][audio] ES8311 session init failed");
    return;
  }
  if (!initMicrophoneCodec()) {
    Serial.println("[BikeMB][audio] ES7210 session init failed");
    return;
  }

  g_i2s.setPins(GPIO_NUM_48, GPIO_NUM_38, GPIO_NUM_47, GPIO_NUM_39, GPIO_NUM_2);
  g_ready = g_i2s.begin(I2S_MODE_STD, kSampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  if (!g_ready) {
    Serial.println("[BikeMB][audio] session I2S init failed");
    return;
  }
  g_i2s.setTimeout(kAudioSessionReadTimeoutMs);

  Serial.println("[BikeMB][audio] session ready");
}

bool BikeMbAudioSession_Acquire(BikeMbAudioSessionOwner owner, uint32_t requestId) {
  if (!g_ready) {
    return false;
  }

  return BikeMbAudioSessionCore_Acquire(&g_state, owner, requestId);
}

void BikeMbAudioSession_Release(BikeMbAudioSessionOwner owner, uint32_t requestId) {
  BikeMbAudioSessionCore_Release(&g_state, owner, requestId);
}

void BikeMbAudioSession_ReleaseAll(void) {
  BikeMbAudioSessionCore_ReleaseAll(&g_state);
}

BikeMbAudioSessionOwner BikeMbAudioSession_GetOwner(void) {
  return BikeMbAudioSessionCore_GetOwner(&g_state);
}

uint32_t BikeMbAudioSession_GetRequestId(void) {
  return BikeMbAudioSessionCore_GetRequestId(&g_state);
}

size_t BikeMbAudioSession_WriteStereoPcm(
    BikeMbAudioSessionOwner owner, uint32_t requestId, const int16_t *samples, size_t frameCount) {
  if (!g_ready || samples == nullptr || frameCount == 0) {
    return 0;
  }
  if (BikeMbAudioSessionCore_GetOwner(&g_state) != owner ||
      BikeMbAudioSessionCore_GetRequestId(&g_state) != requestId) {
    return 0;
  }

  const size_t byteCount = frameCount * 2 * sizeof(samples[0]);
  g_i2s.setTimeout(kAudioSessionWriteTimeoutMs);
  const size_t bytesWritten = g_i2s.write(reinterpret_cast<const uint8_t *>(samples), byteCount);
  return bytesWritten / (2 * sizeof(samples[0]));
}

size_t BikeMbAudioSession_ReadMicBytes(
    BikeMbAudioSessionOwner owner, uint32_t requestId, void *buffer, size_t byteCount) {
  if (!g_ready || buffer == nullptr || byteCount == 0) {
    return 0;
  }
  if (BikeMbAudioSessionCore_GetOwner(&g_state) != owner ||
      BikeMbAudioSessionCore_GetRequestId(&g_state) != requestId) {
    return 0;
  }

  g_i2s.setTimeout(kAudioSessionReadTimeoutMs);
  return g_i2s.readBytes(reinterpret_cast<char *>(buffer), byteCount);
}

bool BikeMbAudioSession_StartCapture(uint32_t requestId, uint32_t maxMs) {
  if (g_capture.samples != nullptr) {
    return false;
  }

  const uint32_t capacitySamples = BikeMbAudioCaptureCore_MaxSamplesForMs(maxMs);
  if (capacitySamples == 0 ||
      !BikeMbAudioSession_Acquire(BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE, requestId)) {
    return false;
  }

  int16_t *samples = static_cast<int16_t *>(ps_malloc(capacitySamples * sizeof(int16_t)));
  if (samples == nullptr) {
    BikeMbAudioSession_Release(BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE, requestId);
    Serial.println("[BikeMB][audio] capture PSRAM allocation failed");
    return false;
  }

  g_capture.samples = samples;
  g_capture.sampleCount = 0;
  g_capture.sampleRateHz = BIKE_MB_AUDIO_CAPTURE_SAMPLE_RATE_HZ;
  g_capture.capacitySamples = capacitySamples;
  g_capture.hitLimit = false;
  g_captureRequestId = requestId;
  g_captureActive = true;
  return true;
}

bool BikeMbAudioSession_PollCapture(uint32_t requestId) {
  if (!g_captureActive || g_capture.samples == nullptr || g_captureRequestId != requestId) {
    return false;
  }

  if (g_capture.sampleCount >= g_capture.capacitySamples) {
    g_capture.hitLimit = true;
    g_captureActive = false;
    BikeMbAudioSession_Release(BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE, requestId);
    return false;
  }

  int16_t stereo[kCaptureStereoFramesPerRead * 2] = {};
  const size_t bytesRead = BikeMbAudioSession_ReadMicBytes(
      BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE, requestId, stereo, sizeof(stereo));
  if (bytesRead == 0) {
    return true;
  }

  const size_t framesRead = bytesRead / (2 * sizeof(stereo[0]));
  const size_t remainingSamples = g_capture.capacitySamples - g_capture.sampleCount;
  const size_t writtenSamples = BikeMbAudioCaptureCore_DownmixStereoToMono(
      stereo, framesRead, g_capture.samples + g_capture.sampleCount, remainingSamples);
  g_capture.sampleCount += static_cast<uint32_t>(writtenSamples);

  if (g_capture.sampleCount >= g_capture.capacitySamples) {
    g_capture.hitLimit = true;
    g_captureActive = false;
    BikeMbAudioSession_Release(BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE, requestId);
    return false;
  }
  return true;
}

bool BikeMbAudioSession_FinishCapture(uint32_t requestId, BikeMbAudioClip *outClip) {
  if (outClip == nullptr || g_capture.samples == nullptr || g_captureRequestId != requestId) {
    return false;
  }

  if (g_captureActive) {
    g_captureActive = false;
    BikeMbAudioSession_Release(BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE, requestId);
  }

  *outClip = g_capture;
  g_capture = {};
  g_captureRequestId = 0;
  return true;
}

void BikeMbAudioSession_ReleaseClip(BikeMbAudioClip *clip) {
  if (clip == nullptr || clip->samples == nullptr) {
    return;
  }
  free(clip->samples);
  *clip = {};
}

#else

namespace {
BikeMbAudioSessionState g_state;
}

void BikeMbAudioSession_Init(void) {
  BikeMbAudioSessionCore_Init(&g_state);
}

bool BikeMbAudioSession_Acquire(BikeMbAudioSessionOwner owner, uint32_t requestId) {
  (void)owner;
  (void)requestId;
  return false;
}

void BikeMbAudioSession_Release(BikeMbAudioSessionOwner owner, uint32_t requestId) {
  (void)owner;
  (void)requestId;
}

void BikeMbAudioSession_ReleaseAll(void) {}

BikeMbAudioSessionOwner BikeMbAudioSession_GetOwner(void) {
  return BIKE_MB_AUDIO_SESSION_OWNER_NONE;
}

uint32_t BikeMbAudioSession_GetRequestId(void) {
  return 0;
}

size_t BikeMbAudioSession_WriteStereoPcm(
    BikeMbAudioSessionOwner owner, uint32_t requestId, const int16_t *samples, size_t frameCount) {
  (void)owner;
  (void)requestId;
  (void)samples;
  (void)frameCount;
  return 0;
}

size_t BikeMbAudioSession_ReadMicBytes(
    BikeMbAudioSessionOwner owner, uint32_t requestId, void *buffer, size_t byteCount) {
  (void)owner;
  (void)requestId;
  (void)buffer;
  (void)byteCount;
  return 0;
}

bool BikeMbAudioSession_StartCapture(uint32_t requestId, uint32_t maxMs) {
  (void)requestId;
  (void)maxMs;
  return false;
}

bool BikeMbAudioSession_PollCapture(uint32_t requestId) {
  (void)requestId;
  return false;
}

bool BikeMbAudioSession_FinishCapture(uint32_t requestId, BikeMbAudioClip *outClip) {
  (void)requestId;
  (void)outClip;
  return false;
}

void BikeMbAudioSession_ReleaseClip(BikeMbAudioClip *clip) {
  (void)clip;
}

#endif
