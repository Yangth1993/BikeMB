#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../audio/audio_session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*BikeMbQwenAsrJsonSink)(const char *data, size_t length, void *context);

bool BikeMbQwenAsr_WriteRequestJson(
    const BikeMbAudioClip *clip, BikeMbQwenAsrJsonSink sink, void *context);

#ifdef __cplusplus
}
#endif
