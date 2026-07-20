#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../../src/firmware/bikemb/src/ai/cosyvoice_tts_adapter.h"

struct PcmCapture {
  int16_t samples[8];
  size_t sampleCount;
};

struct TextCapture {
  char text[256];
  size_t length;
};

static bool captureSink(const int16_t *samples, size_t sampleCount, void *context) {
  PcmCapture *capture = static_cast<PcmCapture *>(context);
  if (samples == nullptr || capture == nullptr) {
    return false;
  }
  for (size_t i = 0; i < sampleCount; ++i) {
    capture->samples[capture->sampleCount++] = samples[i];
  }
  return true;
}

static bool textSink(const char *data, size_t length, void *context) {
  TextCapture *capture = static_cast<TextCapture *>(context);
  if (data == nullptr || capture == nullptr || capture->length + length >= sizeof(capture->text)) {
    return false;
  }
  memcpy(capture->text + capture->length, data, length);
  capture->length += length;
  capture->text[capture->length] = '\0';
  return true;
}

int main() {
  PcmCapture capture = {};
  const char *line = "data: {\"output\":{\"audio\":{\"data\":\"AQACAAMA\"}}}";
  assert(BikeMbCosyVoice_HandleSseLine(line, captureSink, &capture));
  assert(capture.sampleCount == 3);
  assert(capture.samples[0] == 1);
  assert(capture.samples[1] == 2);
  assert(capture.samples[2] == 3);

  PcmCapture streamed = {};
  BikeMbCosyVoicePcmStream stream = {};
  BikeMbCosyVoice_PcmStreamInit(&stream);
  assert(BikeMbCosyVoice_HandleSseLineWithStream(
      "data: {\"output\":{\"audio\":{\"data\":\"AQAC\"}}}", &stream, captureSink, &streamed));
  assert(BikeMbCosyVoice_HandleSseLineWithStream(
      "data: {\"output\":{\"audio\":{\"data\":\"AAMA\"}}}", &stream, captureSink, &streamed));
  assert(BikeMbCosyVoice_PcmStreamFinish(&stream, captureSink, &streamed));
  assert(streamed.sampleCount == 3);
  assert(streamed.samples[0] == 1);
  assert(streamed.samples[1] == 2);
  assert(streamed.samples[2] == 3);

  TextCapture request = {};
  assert(BikeMbCosyVoice_WriteRequestJson("OK", 2, textSink, &request));
  assert(strstr(request.text, "\"input\":{\"text\":\"OK\",\"voice\":\"longanyang\",\"format\":\"pcm\",\"sample_rate\":16000}") != nullptr);
  return 0;
}
