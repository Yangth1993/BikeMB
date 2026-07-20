#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*BikeMbCosyVoiceJsonSink)(const char *data, size_t length, void *context);
typedef bool (*BikeMbCosyVoicePcmSink)(const int16_t *samples, size_t sampleCount, void *context);

typedef struct {
  uint8_t pendingByte;
  bool hasPendingByte;
  int16_t samples[96];
  size_t sampleCount;
} BikeMbCosyVoicePcmStream;

bool BikeMbCosyVoice_WriteRequestJson(
    const char *text, size_t textLength, BikeMbCosyVoiceJsonSink sink, void *context);
void BikeMbCosyVoice_PcmStreamInit(BikeMbCosyVoicePcmStream *stream);
bool BikeMbCosyVoice_PcmStreamFinish(
    BikeMbCosyVoicePcmStream *stream, BikeMbCosyVoicePcmSink sink, void *context);
bool BikeMbCosyVoice_HandleSseLine(
    const char *line, BikeMbCosyVoicePcmSink sink, void *context);
bool BikeMbCosyVoice_HandleSseLineWithStream(
    const char *line,
    BikeMbCosyVoicePcmStream *stream,
    BikeMbCosyVoicePcmSink sink,
    void *context);

#ifdef __cplusplus
}
#endif
