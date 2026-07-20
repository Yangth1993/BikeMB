#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*BikeMbQwenChatJsonSink)(const char *data, size_t length, void *context);

bool BikeMbQwenChat_WriteRequestJson(
    const char *text, size_t textLength, BikeMbQwenChatJsonSink sink, void *context);
size_t BikeMbQwenChat_CopyBoundedAnswer(const char *text, char *out, size_t outCapacity);
const char *BikeMbQwenChat_RedactedLogLabel(bool success);

#ifdef __cplusplus
}
#endif
