#include "qwen_asr_adapter.h"

#include <stddef.h>
#include <stdint.h>

#include "ai_config.h"

namespace {

constexpr uint16_t kPcmFormat = 1;
constexpr uint16_t kChannels = 1;
constexpr uint16_t kBitsPerSample = 16;

struct BikeMbQwenAsrBase64State {
  uint8_t carry[3];
  size_t carryCount;
};

bool writeText(BikeMbQwenAsrJsonSink sink, void *context, const char *text) {
  if (sink == nullptr || text == nullptr) {
    return false;
  }

  size_t length = 0;
  while (text[length] != '\0') {
    ++length;
  }
  return sink(text, length, context);
}

bool writeByte(BikeMbQwenAsrJsonSink sink, void *context, uint8_t value) {
  const char byte = static_cast<char>(value);
  return sink(&byte, 1, context);
}

bool writeLe16(BikeMbQwenAsrJsonSink sink, void *context, uint16_t value) {
  return writeByte(sink, context, static_cast<uint8_t>(value & 0xFFU)) &&
         writeByte(sink, context, static_cast<uint8_t>((value >> 8U) & 0xFFU));
}

bool writeLe32(BikeMbQwenAsrJsonSink sink, void *context, uint32_t value) {
  return writeByte(sink, context, static_cast<uint8_t>(value & 0xFFU)) &&
         writeByte(sink, context, static_cast<uint8_t>((value >> 8U) & 0xFFU)) &&
         writeByte(sink, context, static_cast<uint8_t>((value >> 16U) & 0xFFU)) &&
         writeByte(sink, context, static_cast<uint8_t>((value >> 24U) & 0xFFU));
}

bool writeBase64Chars(BikeMbQwenAsrJsonSink sink, void *context, const char *chars, size_t length) {
  return sink != nullptr && chars != nullptr && sink(chars, length, context);
}

bool writeBase64Triplet(BikeMbQwenAsrJsonSink sink, void *context, const uint8_t *bytes) {
  static constexpr char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  char out[4] = {};
  out[0] = kAlphabet[bytes[0] >> 2];
  out[1] = kAlphabet[((bytes[0] & 0x03U) << 4) | (bytes[1] >> 4)];
  out[2] = kAlphabet[((bytes[1] & 0x0FU) << 2) | (bytes[2] >> 6)];
  out[3] = kAlphabet[bytes[2] & 0x3FU];
  return writeBase64Chars(sink, context, out, sizeof(out));
}

bool BikeMbQwenAsr_WriteBase64Chunk(
    BikeMbQwenAsrBase64State *state,
    BikeMbQwenAsrJsonSink sink,
    void *context,
    const uint8_t *bytes,
    size_t byteCount) {
  if (sink == nullptr || bytes == nullptr) {
    return false;
  }

  for (size_t i = 0; i < byteCount; ++i) {
    state->carry[state->carryCount++] = bytes[i];
    if (state->carryCount == sizeof(state->carry)) {
      if (!writeBase64Triplet(sink, context, state->carry)) {
        return false;
      }
      state->carryCount = 0;
    }
  }
  return true;
}

bool BikeMbQwenAsr_WriteBase64Final(
    BikeMbQwenAsrBase64State *state, BikeMbQwenAsrJsonSink sink, void *context) {
  static constexpr char kAlphabet[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  if (state == nullptr || sink == nullptr) {
    return false;
  }
  if (state->carryCount == 0) {
    return true;
  }

  const uint8_t b0 = state->carry[0];
  const uint8_t b1 = state->carryCount > 1 ? state->carry[1] : 0;
  char out[4] = {};
  out[0] = kAlphabet[b0 >> 2];
  out[1] = kAlphabet[((b0 & 0x03U) << 4) | (b1 >> 4)];
  out[2] = state->carryCount > 1 ? kAlphabet[(b1 & 0x0FU) << 2] : '=';
  out[3] = '=';
  state->carryCount = 0;
  return writeBase64Chars(sink, context, out, sizeof(out));
}

bool BikeMbQwenAsr_WriteWavHeader(
    BikeMbQwenAsrJsonSink sink, void *context, uint32_t sampleRateHz, uint32_t dataBytes) {
  const uint32_t byteRate = sampleRateHz * kChannels * (kBitsPerSample / 8U);
  const uint16_t blockAlign = kChannels * (kBitsPerSample / 8U);
  return writeText(sink, context, "RIFF") &&
         writeLe32(sink, context, 36U + dataBytes) &&
         writeText(sink, context, "WAVEfmt ") &&
         writeLe32(sink, context, 16U) &&
         writeLe16(sink, context, kPcmFormat) &&
         writeLe16(sink, context, kChannels) &&
         writeLe32(sink, context, sampleRateHz) &&
         writeLe32(sink, context, byteRate) &&
         writeLe16(sink, context, blockAlign) &&
         writeLe16(sink, context, kBitsPerSample) &&
         writeText(sink, context, "data") &&
         writeLe32(sink, context, dataBytes);
}

bool BikeMbQwenAsr_WriteWavHeaderBase64(
    BikeMbQwenAsrBase64State *state,
    BikeMbQwenAsrJsonSink sink,
    void *context,
    uint32_t sampleRateHz,
    uint32_t dataBytes) {
  uint8_t wavHeader[44] = {};
  size_t offset = 0;
  auto putText = [&](const char *text) {
    while (*text != '\0') {
      wavHeader[offset++] = static_cast<uint8_t>(*text++);
    }
  };
  auto putLe16 = [&](uint16_t value) {
    wavHeader[offset++] = static_cast<uint8_t>(value & 0xFFU);
    wavHeader[offset++] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
  };
  auto putLe32 = [&](uint32_t value) {
    wavHeader[offset++] = static_cast<uint8_t>(value & 0xFFU);
    wavHeader[offset++] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    wavHeader[offset++] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    wavHeader[offset++] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
  };

  const uint32_t byteRate = sampleRateHz * kChannels * (kBitsPerSample / 8U);
  const uint16_t blockAlign = kChannels * (kBitsPerSample / 8U);
  putText("RIFF");
  putLe32(36U + dataBytes);
  putText("WAVEfmt ");
  putLe32(16U);
  putLe16(kPcmFormat);
  putLe16(kChannels);
  putLe32(sampleRateHz);
  putLe32(byteRate);
  putLe16(blockAlign);
  putLe16(kBitsPerSample);
  putText("data");
  putLe32(dataBytes);
  return BikeMbQwenAsr_WriteBase64Chunk(state, sink, context, wavHeader, sizeof(wavHeader));
}

bool writeClipPcmAsBase64(
    const BikeMbAudioClip *clip,
    BikeMbQwenAsrBase64State *state,
    BikeMbQwenAsrJsonSink sink,
    void *context) {
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(clip->samples);
  const size_t byteCount = clip->sampleCount * sizeof(int16_t);
  return BikeMbQwenAsr_WriteBase64Chunk(state, sink, context, bytes, byteCount);
}

}  // namespace

bool BikeMbQwenAsr_WriteRequestJson(
    const BikeMbAudioClip *clip, BikeMbQwenAsrJsonSink sink, void *context) {
  if (clip == nullptr || clip->samples == nullptr || clip->sampleCount == 0 || sink == nullptr) {
    return false;
  }

  const uint32_t dataBytes = clip->sampleCount * sizeof(int16_t);
  BikeMbQwenAsrBase64State base64 = {};
  return writeText(sink, context, "{\"model\":\"") &&
         writeText(sink, context, BikeMbAiConfig::kQwenAsrModel) &&
         writeText(
             sink,
             context,
             "\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"input_audio\",\"input_audio\":{\"data\":\"data:audio/wav;base64,") &&
         BikeMbQwenAsr_WriteWavHeaderBase64(
             &base64, sink, context, clip->sampleRateHz, dataBytes) &&
         writeClipPcmAsBase64(clip, &base64, sink, context) &&
         BikeMbQwenAsr_WriteBase64Final(&base64, sink, context) &&
         writeText(
             sink,
             context,
             "\"}}]}],\"stream\":false,\"asr_options\":{\"language\":\"zh\",\"enable_itn\":false}}");
}
