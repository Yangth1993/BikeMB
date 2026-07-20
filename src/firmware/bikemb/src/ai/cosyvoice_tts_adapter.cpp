#include "cosyvoice_tts_adapter.h"

#include "ai_config.h"

namespace {

bool writeText(BikeMbCosyVoiceJsonSink sink, void *context, const char *text) {
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
    const char *text, size_t textLength, BikeMbCosyVoiceJsonSink sink, void *context) {
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
    } else {
      if (!sink(&c, 1, context)) {
        return false;
      }
    }
  }
  return true;
}

int decodeBase64Char(char c) {
  if (c >= 'A' && c <= 'Z') {
    return c - 'A';
  }
  if (c >= 'a' && c <= 'z') {
    return c - 'a' + 26;
  }
  if (c >= '0' && c <= '9') {
    return c - '0' + 52;
  }
  if (c == '+') {
    return 62;
  }
  if (c == '/') {
    return 63;
  }
  return -1;
}

bool flushPcmSamples(
    BikeMbCosyVoicePcmStream *state, BikeMbCosyVoicePcmSink sink, void *context) {
  if (state == nullptr || sink == nullptr || state->sampleCount == 0) {
    return true;
  }
  const bool ok = sink(state->samples, state->sampleCount, context);
  state->sampleCount = 0;
  return ok;
}

bool emitPcmByte(
    BikeMbCosyVoicePcmStream *state,
    uint8_t byte,
    BikeMbCosyVoicePcmSink sink,
    void *context) {
  if (state == nullptr || sink == nullptr) {
    return false;
  }

  if (!state->hasPendingByte) {
    state->pendingByte = byte;
    state->hasPendingByte = true;
    return true;
  }

  state->samples[state->sampleCount++] =
      static_cast<int16_t>(state->pendingByte | (static_cast<uint16_t>(byte) << 8U));
  state->hasPendingByte = false;
  if (state->sampleCount == 96) {
    if (!flushPcmSamples(state, sink, context)) {
      return false;
    }
  }
  return true;
}

bool emitPcmBytes(
    BikeMbCosyVoicePcmStream *state,
    const uint8_t *bytes,
    size_t byteCount,
    BikeMbCosyVoicePcmSink sink,
    void *context) {
  if (state == nullptr || sink == nullptr || bytes == nullptr) {
    return false;
  }
  for (size_t i = 0; i < byteCount; ++i) {
    if (!emitPcmByte(state, bytes[i], sink, context)) {
      return false;
    }
  }
  return true;
}

bool BikeMbCosyVoice_DecodeBase64Chunk(
    const char *base64,
    size_t length,
    BikeMbCosyVoicePcmStream *stream,
    BikeMbCosyVoicePcmSink sink,
    void *context) {
  if (base64 == nullptr || stream == nullptr || sink == nullptr) {
    return false;
  }

  uint8_t out[3] = {};
  int quartet[4] = {};
  size_t quartetCount = 0;
  for (size_t i = 0; i < length; ++i) {
    const char c = base64[i];
    if (c == '=') {
      quartet[quartetCount++] = -2;
    } else {
      const int value = decodeBase64Char(c);
      if (value < 0) {
        continue;
      }
      quartet[quartetCount++] = value;
    }

    if (quartetCount != 4) {
      continue;
    }

    out[0] = static_cast<uint8_t>((quartet[0] << 2) | (quartet[1] >> 4));
    size_t outCount = 1;
    if (quartet[2] != -2) {
      out[1] = static_cast<uint8_t>(((quartet[1] & 0x0F) << 4) | (quartet[2] >> 2));
      outCount = 2;
    }
    if (quartet[3] != -2) {
      out[2] = static_cast<uint8_t>(((quartet[2] & 0x03) << 6) | quartet[3]);
      outCount = 3;
    }
    if (!emitPcmBytes(stream, out, outCount, sink, context)) {
      return false;
    }
    quartetCount = 0;
  }
  return flushPcmSamples(stream, sink, context);
}

const char *findAudioValue(const char *line) {
  const char *p = line;
  static constexpr const char *kAudioKey = "\"data\":\"";
  while (*p != '\0') {
    const char *candidate = p;
    const char *key = kAudioKey;
    while (*candidate == *key && *key != '\0') {
      ++candidate;
      ++key;
    }
    if (*key == '\0') {
      return candidate;
    }
    ++p;
  }
  return nullptr;
}

}  // namespace

bool BikeMbCosyVoice_WriteRequestJson(
    const char *text, size_t textLength, BikeMbCosyVoiceJsonSink sink, void *context) {
  if (text == nullptr || textLength == 0 || sink == nullptr) {
    return false;
  }

  return writeText(sink, context, "{\"model\":\"") &&
         writeText(sink, context, BikeMbAiConfig::kCosyVoiceTtsModel) &&
         writeText(
             sink,
             context,
             "\",\"input\":{\"text\":\"") &&
         writeJsonEscaped(text, textLength, sink, context) &&
         writeText(
             sink,
             context,
             "\",\"voice\":\"longanyang\",\"format\":\"pcm\",\"sample_rate\":16000}}");
}

void BikeMbCosyVoice_PcmStreamInit(BikeMbCosyVoicePcmStream *stream) {
  if (stream == nullptr) {
    return;
  }
  stream->pendingByte = 0;
  stream->hasPendingByte = false;
  stream->sampleCount = 0;
}

bool BikeMbCosyVoice_PcmStreamFinish(
    BikeMbCosyVoicePcmStream *stream, BikeMbCosyVoicePcmSink sink, void *context) {
  if (stream == nullptr || sink == nullptr) {
    return false;
  }
  if (stream->hasPendingByte) {
    return false;
  }
  return flushPcmSamples(stream, sink, context);
}

bool BikeMbCosyVoice_HandleSseLine(
    const char *line, BikeMbCosyVoicePcmSink sink, void *context) {
  BikeMbCosyVoicePcmStream stream = {};
  BikeMbCosyVoice_PcmStreamInit(&stream);
  return BikeMbCosyVoice_HandleSseLineWithStream(line, &stream, sink, context) &&
         BikeMbCosyVoice_PcmStreamFinish(&stream, sink, context);
}

bool BikeMbCosyVoice_HandleSseLineWithStream(
    const char *line,
    BikeMbCosyVoicePcmStream *stream,
    BikeMbCosyVoicePcmSink sink,
    void *context) {
  if (line == nullptr || sink == nullptr) {
    return false;
  }

  const char *audio = findAudioValue(line);
  if (audio == nullptr) {
    return true;
  }

  size_t length = 0;
  while (audio[length] != '\0' && audio[length] != '"') {
    ++length;
  }
  return BikeMbCosyVoice_DecodeBase64Chunk(audio, length, stream, sink, context);
}
