#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*BikeMbDeepSeekJsonSink)(const char *data, size_t length, void *context);

bool BikeMbDeepSeek_WriteRequestJson(
    const char *text, size_t textLength, BikeMbDeepSeekJsonSink sink, void *context);
size_t BikeMbDeepSeek_CopyBoundedAnswer(const char *text, char *out, size_t outCapacity);
const char *BikeMbDeepSeek_RedactedLogLabel(bool success);

#ifdef __cplusplus
}
#endif
