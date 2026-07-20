#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "audio_session_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BikeMbAudioClip {
  int16_t *samples;
  uint32_t sampleCount;
  uint32_t sampleRateHz;
  uint32_t capacitySamples;
  bool hitLimit;
} BikeMbAudioClip;

void BikeMbAudioSession_Init(void);
bool BikeMbAudioSession_Acquire(BikeMbAudioSessionOwner owner, uint32_t requestId);
void BikeMbAudioSession_Release(BikeMbAudioSessionOwner owner, uint32_t requestId);
void BikeMbAudioSession_ReleaseAll(void);
BikeMbAudioSessionOwner BikeMbAudioSession_GetOwner(void);
uint32_t BikeMbAudioSession_GetRequestId(void);
size_t BikeMbAudioSession_WriteStereoPcm(
    BikeMbAudioSessionOwner owner, uint32_t requestId, const int16_t *samples, size_t frameCount);
size_t BikeMbAudioSession_ReadMicBytes(
    BikeMbAudioSessionOwner owner, uint32_t requestId, void *buffer, size_t byteCount);
bool BikeMbAudioSession_StartCapture(uint32_t requestId, uint32_t maxMs);
bool BikeMbAudioSession_PollCapture(uint32_t requestId);
bool BikeMbAudioSession_FinishCapture(uint32_t requestId, BikeMbAudioClip *outClip);
void BikeMbAudioSession_ReleaseClip(BikeMbAudioClip *clip);

#ifdef __cplusplus
}
#endif
