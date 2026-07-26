#include "qwen_chat_adapter.h"

#include "ai_config.h"

namespace {

bool writeText(BikeMbQwenChatJsonSink sink, void *context, const char *text) {
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
    const char *text, size_t textLength, BikeMbQwenChatJsonSink sink, void *context) {
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

size_t utf8SafePrefixLength(const char *text, size_t limit) {
  if (text == nullptr) {
    return 0;
  }
  size_t safe = 0;
  size_t offset = 0;
  while (offset < limit && text[offset] != '\0') {
    const unsigned char c = static_cast<unsigned char>(text[offset]);
    size_t width = 1;
    if ((c & 0x80U) == 0) {
      width = 1;
    } else if ((c & 0xE0U) == 0xC0U) {
      width = 2;
    } else if ((c & 0xF0U) == 0xE0U) {
      width = 3;
    } else if ((c & 0xF8U) == 0xF0U) {
      width = 4;
    } else {
      break;
    }
    if (offset + width > limit) {
      break;
    }
    for (size_t i = 1; i < width; ++i) {
      const unsigned char next = static_cast<unsigned char>(text[offset + i]);
      if ((next & 0xC0U) != 0x80U) {
        return safe;
      }
    }
    offset += width;
    safe = offset;
  }
  return safe;
}

}  // namespace

bool BikeMbQwenChat_WriteRequestJson(
    const char *text, size_t textLength, BikeMbQwenChatJsonSink sink, void *context) {
  if (text == nullptr || textLength == 0 || sink == nullptr) {
    return false;
  }

  return writeText(sink, context, "{\"model\":\"") &&
         writeText(sink, context, BikeMbAiConfig::kQwenChatModel) &&
         writeText(
             sink,
             context,
             "\",\"messages\":[{\"role\":\"system\",\"content\":\"Answer in Chinese for spoken playback. Use one short sentence and stay within 20 Chinese characters.\"},"
             "{\"role\":\"user\",\"content\":\"") &&
         writeJsonEscaped(text, textLength, sink, context) &&
         writeText(sink, context, "\"}],\"max_tokens\":32,\"stream\":false}");
}

size_t BikeMbQwenChat_CopyBoundedAnswer(const char *text, char *out, size_t outCapacity) {
  if (text == nullptr || out == nullptr || outCapacity == 0) {
    return 0;
  }

  const size_t limit =
      BikeMbAiConfig::kMaxAnswerBytes < (outCapacity - 1U)
          ? BikeMbAiConfig::kMaxAnswerBytes
          : (outCapacity - 1U);
  const size_t safeLimit = utf8SafePrefixLength(text, limit);
  size_t written = 0;
  while (written < safeLimit && text[written] != '\0') {
    out[written] = text[written];
    ++written;
  }
  out[written] = '\0';
  return written;
}

const char *BikeMbQwenChat_RedactedLogLabel(bool success) {
  return success ? "qwen chat ready" : "qwen chat failed";
}
