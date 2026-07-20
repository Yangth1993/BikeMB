#include "deepseek_adapter.h"

#include "ai_config.h"

namespace {

bool writeText(BikeMbDeepSeekJsonSink sink, void *context, const char *text) {
  if (sink == nullptr || text == nullptr) {
    return false;
  }

  size_t length = 0;
  while (text[length] != '\0') {
    ++length;
  }
  return sink(text, length, context);
}

bool writeJsonEscaped(
    const char *text, size_t textLength, BikeMbDeepSeekJsonSink sink, void *context) {
  if (text == nullptr || sink == nullptr) {
    return false;
  }

  for (size_t i = 0; i < textLength; ++i) {
    const char c = text[i];
    if (c == '"' || c == '\\') {
      const char escaped[2] = {'\\', c};
      if (!sink(escaped, sizeof(escaped), context)) {
        return false;
      }
    } else if (c == '\n') {
      if (!writeText(sink, context, "\\n")) {
        return false;
      }
    } else if (c == '\r') {
      if (!writeText(sink, context, "\\r")) {
        return false;
      }
    } else if (c == '\t') {
      if (!writeText(sink, context, "\\t")) {
        return false;
      }
    } else {
      if (!sink(&c, 1, context)) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace

bool BikeMbDeepSeek_WriteRequestJson(
    const char *text, size_t textLength, BikeMbDeepSeekJsonSink sink, void *context) {
  if (text == nullptr || textLength == 0 || sink == nullptr) {
    return false;
  }

  return writeText(sink, context, "{\"model\":\"") &&
         writeText(sink, context, BikeMbAiConfig::kDeepSeekModel) &&
         writeText(
             sink,
             context,
             "\",\"messages\":[{\"role\":\"system\",\"content\":\"Answer briefly for spoken playback.\"},"
             "{\"role\":\"user\",\"content\":\"") &&
         writeJsonEscaped(text, textLength, sink, context) &&
         writeText(sink, context, "\"}],\"max_tokens\":256,\"stream\":false}");
}

size_t BikeMbDeepSeek_CopyBoundedAnswer(const char *text, char *out, size_t outCapacity) {
  if (text == nullptr || out == nullptr || outCapacity == 0) {
    return 0;
  }

  const size_t limit =
      BikeMbAiConfig::kMaxAnswerBytes < (outCapacity - 1U)
          ? BikeMbAiConfig::kMaxAnswerBytes
          : (outCapacity - 1U);
  size_t written = 0;
  while (written < limit && text[written] != '\0') {
    out[written] = text[written];
    ++written;
  }
  out[written] = '\0';
  return written;
}

const char *BikeMbDeepSeek_RedactedLogLabel(bool success) {
  return success ? "deepseek ready" : "deepseek failed";
}
