#include "cloud_worker.h"

#include "ai_config.h"
#include "cosyvoice_tts_adapter.h"
#include "qwen_asr_adapter.h"
#include "qwen_chat_adapter.h"
#include "runtime/bike_runtime_plan.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#if defined(BIKE_MB_USE_ESPIDF_RUNTIME)
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#endif

#if __has_include("ai_secrets.local.h")
#include "ai_secrets.local.h"
#endif

#ifndef BIKE_MB_AI_STT_TOKEN
#define BIKE_MB_AI_STT_TOKEN ""
#endif

#ifndef BIKE_MB_AI_DASHSCOPE_TOKEN
#define BIKE_MB_AI_DASHSCOPE_TOKEN BIKE_MB_AI_STT_TOKEN
#endif

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
#include <Arduino.h>
#include <WiFiClientSecure.h>
#endif

namespace {
constexpr uint32_t kQueueLength = 4;
constexpr uint32_t kHttpsTimeoutMs = 15000;
constexpr unsigned long kTlsHandshakeTimeoutSeconds = 8;
constexpr size_t kMaxJsonResponseBytes = 4096;
constexpr size_t kMaxSseLineBytes = 12 * 1024;
constexpr size_t kHttpWriteBufferBytes = 1024;
constexpr size_t kMaxTtsPcmSamples = 320000;
constexpr size_t kMaxTtsPlaybackSamples = 96000;
constexpr uint8_t kTtsMaxAttempts = 2;

QueueHandle_t s_queue = nullptr;
BikeMbCloudResultSink s_sink = nullptr;
portMUX_TYPE s_requestMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE s_clipMux = portMUX_INITIALIZER_UNLOCKED;
uint32_t s_minValidRequestId = 0;
BikeMbAudioClip s_captureClip = {};
uint32_t s_captureClipRequestId = 0;
char s_sttText[BikeMbAiConfig::kMaxSttTextBytes] = {};
char s_answerText[BikeMbAiConfig::kMaxAnswerBytes] = {};

uint32_t delayForStage(BikeMbCloudStage stage) {
  switch (stage) {
    case BIKE_MB_CLOUD_STAGE_STT: return 250;
    case BIKE_MB_CLOUD_STAGE_LLM: return 400;
    case BIKE_MB_CLOUD_STAGE_TTS: return 250;
  }
  return 250;
}

const char *labelForStage(BikeMbCloudStage stage) {
  switch (stage) {
    case BIKE_MB_CLOUD_STAGE_STT: return "mock stt ready";
    case BIKE_MB_CLOUD_STAGE_LLM: return "mock llm ready";
    case BIKE_MB_CLOUD_STAGE_TTS: return "mock tts ready";
  }
  return "mock stage ready";
}

bool hasDashScopeToken() {
  return BIKE_MB_AI_DASHSCOPE_TOKEN[0] != '\0' &&
         strcmp(BIKE_MB_AI_DASHSCOPE_TOKEN, "CHANGE_ME_DASHSCOPE_TOKEN") != 0 &&
         strcmp(BIKE_MB_AI_DASHSCOPE_TOKEN, "CHANGE_ME_STT_TOKEN") != 0;
}

uint32_t cloudNowMs() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  return millis();
#else
  return static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS);
#endif
}

void logCloudStatus(const char *label, int statusCode, bool success) {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  Serial.print("[BikeMB][ai] ");
  Serial.print(label);
  Serial.print(" http status=");
  Serial.print(statusCode);
  Serial.print(" ok=");
  Serial.println(success ? 1 : 0);
#else
  ESP_LOGI("BikeMbCloud", "%s http status=%d ok=%d", label, statusCode, success ? 1 : 0);
#endif
}

void logCloudDuration(const char *label, const char *phase, uint32_t startMs) {
  const uint32_t elapsedMs = cloudNowMs() - startMs;
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  Serial.print("[BikeMB][ai] ");
  Serial.print(label);
  Serial.print(" ");
  Serial.print(phase);
  Serial.print(" elapsed_ms=");
  Serial.println(elapsedMs);
#else
  ESP_LOGI("BikeMbCloud", "%s %s elapsed_ms=%u", label, phase, elapsedMs);
#endif
}

void logCloudLabel(const char *label) {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  Serial.print("[BikeMB][ai] ");
  Serial.println(label);
#else
  ESP_LOGI("BikeMbCloud", "%s", label);
#endif
}

void logCloudText(const char *label, const char *text) {
  if (label == nullptr || text == nullptr) {
    return;
  }
  static constexpr size_t kMaxLogTextBytes = 160;
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  Serial.print("[BikeMB][ai] ");
  Serial.print(label);
  Serial.print("=");
  for (size_t i = 0; text[i] != '\0' && i < kMaxLogTextBytes; ++i) {
    const char c = text[i];
    Serial.print(c == '\n' || c == '\r' ? ' ' : c);
  }
  if (strlen(text) > kMaxLogTextBytes) {
    Serial.print("...");
  }
  Serial.println();
#else
  char bounded[kMaxLogTextBytes + 1] = {};
  size_t i = 0;
  for (; text[i] != '\0' && i < kMaxLogTextBytes; ++i) {
    const char c = text[i];
    bounded[i] = c == '\n' || c == '\r' ? ' ' : c;
  }
  bounded[i] = '\0';
  ESP_LOGI("BikeMbCloud", "%s=%s%s", label, bounded, text[i] != '\0' ? "..." : "");
#endif
}

bool extractJsonString(const char *json, const char *key, char *out, size_t outCapacity);

void logCloudError(const char *label, const char *body) {
  if (label == nullptr || body == nullptr || body[0] == '\0') {
    return;
  }
  char code[48] = {};
  char message[160] = {};
  char requestId[80] = {};
  const bool hasCode = extractJsonString(body, "\"code\":\"", code, sizeof(code));
  const bool hasMessage = extractJsonString(body, "\"message\":\"", message, sizeof(message));
  const bool hasRequestId =
      extractJsonString(body, "\"request_id\":\"", requestId, sizeof(requestId));
  if (!hasCode && !hasMessage && !hasRequestId) {
    return;
  }
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  Serial.print("[BikeMB][ai] ");
  Serial.print(label);
  Serial.print(" error code=");
  Serial.print(hasCode ? code : "");
  Serial.print(" request_id=");
  Serial.print(hasRequestId ? requestId : "");
  Serial.print(" message=");
  Serial.println(hasMessage ? message : "");
#else
  ESP_LOGI(
      "BikeMbCloud",
      "%s error code=%s request_id=%s message=%s",
      label,
      hasCode ? code : "",
      hasRequestId ? requestId : "",
      hasMessage ? message : "");
#endif
}

void logCloudMemory(const char *label) {
  const size_t internalFree =
      heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  const size_t internalLargest =
      heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  const size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const UBaseType_t stackHighWaterWords = uxTaskGetStackHighWaterMark(nullptr);
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  Serial.print("[BikeMB][ai] ");
  Serial.print(label);
  Serial.print(" heap internal_free=");
  Serial.print(internalFree);
  Serial.print(" internal_largest=");
  Serial.print(internalLargest);
  Serial.print(" psram_free=");
  Serial.print(psramFree);
  Serial.print(" stack_hwm_words=");
  Serial.println(static_cast<uint32_t>(stackHighWaterWords));
#else
  ESP_LOGI(
      "BikeMbCloud",
      "%s heap internal_free=%u internal_largest=%u psram_free=%u stack_hwm_words=%u",
      label,
      static_cast<unsigned>(internalFree),
      static_cast<unsigned>(internalLargest),
      static_cast<unsigned>(psramFree),
      static_cast<unsigned>(stackHighWaterWords));
#endif
}

bool isRequestValid(uint32_t requestId) {
  portENTER_CRITICAL(&s_requestMux);
  const bool valid = requestId >= s_minValidRequestId;
  portEXIT_CRITICAL(&s_requestMux);
  return valid;
}

void releaseStoredClip() {
  portENTER_CRITICAL(&s_clipMux);
  BikeMbAudioClip clip = s_captureClip;
  s_captureClip = {};
  s_captureClipRequestId = 0;
  portEXIT_CRITICAL(&s_clipMux);
  BikeMbAudioSession_ReleaseClip(&clip);
}

bool takeStoredClip(uint32_t requestId, BikeMbAudioClip *outClip) {
  if (outClip == nullptr) {
    return false;
  }
  portENTER_CRITICAL(&s_clipMux);
  const bool matches =
      s_captureClip.samples != nullptr && s_captureClipRequestId == requestId;
  if (matches) {
    *outClip = s_captureClip;
    s_captureClip = {};
    s_captureClipRequestId = 0;
  }
  portEXIT_CRITICAL(&s_clipMux);
  return matches;
}

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
struct UrlParts {
  char host[96];
  char path[192];
  uint16_t port;
};

struct CountSinkContext {
  size_t length;
};

struct BufferedClientSinkContext {
  WiFiClientSecure *client;
  char buffer[kHttpWriteBufferBytes];
  size_t length;
};

struct TtsPcmBufferContext {
  int16_t *samples;
  size_t sampleCount;
  size_t capacitySamples;
  bool overflow;
};

struct TtsSseReadStats {
  size_t lineCount;
  size_t audioLineCount;
  const char *failure;
  char lastLine[128];
};

bool parseHttpsUrl(const char *url, UrlParts *out) {
  if (url == nullptr || out == nullptr) {
    return false;
  }
  static constexpr const char *kPrefix = "https://";
  const size_t prefixLength = 8;
  if (strncmp(url, kPrefix, prefixLength) != 0) {
    return false;
  }
  const char *hostStart = url + prefixLength;
  const char *pathStart = strchr(hostStart, '/');
  if (pathStart == nullptr) {
    return false;
  }
  const size_t hostLength = static_cast<size_t>(pathStart - hostStart);
  const size_t pathLength = strlen(pathStart);
  if (hostLength == 0 || hostLength >= sizeof(out->host) ||
      pathLength == 0 || pathLength >= sizeof(out->path)) {
    return false;
  }
  memcpy(out->host, hostStart, hostLength);
  out->host[hostLength] = '\0';
  memcpy(out->path, pathStart, pathLength + 1U);
  out->port = 443;
  return true;
}

bool countSink(const char *, size_t length, void *context) {
  CountSinkContext *count = static_cast<CountSinkContext *>(context);
  if (count == nullptr) {
    return false;
  }
  count->length += length;
  return true;
}

bool flushBufferedClientSink(BufferedClientSinkContext *out) {
  if (out == nullptr || out->client == nullptr) {
    return false;
  }
  if (out->length == 0) {
    return true;
  }
  const size_t written =
      out->client->write(reinterpret_cast<const uint8_t *>(out->buffer), out->length);
  const bool ok = written == out->length;
  out->length = 0;
  return ok;
}

bool bufferedClientSink(const char *data, size_t length, void *context) {
  BufferedClientSinkContext *out = static_cast<BufferedClientSinkContext *>(context);
  if (out == nullptr || out->client == nullptr || data == nullptr) {
    return false;
  }
  size_t offset = 0;
  while (offset < length) {
    if (out->length == sizeof(out->buffer)) {
      if (!flushBufferedClientSink(out)) {
        return false;
      }
    }
    const size_t available = sizeof(out->buffer) - out->length;
    const size_t remaining = length - offset;
    const size_t chunk = remaining < available ? remaining : available;
    memcpy(out->buffer + out->length, data + offset, chunk);
    out->length += chunk;
    offset += chunk;
  }
  return true;
}

bool ttsPcmBufferSink(const int16_t *samples, size_t sampleCount, void *context) {
  TtsPcmBufferContext *tts = static_cast<TtsPcmBufferContext *>(context);
  if (samples == nullptr || tts == nullptr || tts->samples == nullptr) {
    return false;
  }
  if (sampleCount > tts->capacitySamples - tts->sampleCount) {
    tts->overflow = true;
    return false;
  }
  memcpy(tts->samples + tts->sampleCount, samples, sampleCount * sizeof(samples[0]));
  tts->sampleCount += sampleCount;
  return true;
}

bool playTtsPcmBuffer(const TtsPcmBufferContext *tts, uint32_t requestId) {
  if (tts == nullptr || tts->samples == nullptr || tts->sampleCount == 0) {
    return false;
  }
  if (!BikeMbAudioSession_Acquire(BIKE_MB_AUDIO_SESSION_OWNER_AI_PLAYBACK, requestId)) {
    char message[96] = {};
    snprintf(
        message,
        sizeof(message),
        "cosyvoice playback acquire failed owner=%d request=%lu",
        static_cast<int>(BikeMbAudioSession_GetOwner()),
        static_cast<unsigned long>(BikeMbAudioSession_GetRequestId()));
    logCloudLabel(message);
    return false;
  }

  int16_t stereo[128 * 2] = {};
  const size_t playableSamples =
      tts->sampleCount < kMaxTtsPlaybackSamples ? tts->sampleCount : kMaxTtsPlaybackSamples;
  if (playableSamples < tts->sampleCount) {
    char message[96] = {};
    snprintf(
        message,
        sizeof(message),
        "cosyvoice playback clipped samples=%lu limit=%lu",
        static_cast<unsigned long>(tts->sampleCount),
        static_cast<unsigned long>(kMaxTtsPlaybackSamples));
    logCloudLabel(message);
  }
  size_t offset = 0;
  bool ok = true;
  while (offset < playableSamples) {
    const size_t chunkSamples =
        (playableSamples - offset) < 128 ? (playableSamples - offset) : 128;
    for (size_t i = 0; i < chunkSamples; ++i) {
      stereo[i * 2] = tts->samples[offset + i];
      stereo[i * 2 + 1] = tts->samples[offset + i];
    }
    const size_t written = BikeMbAudioSession_WriteStereoPcm(
        BIKE_MB_AUDIO_SESSION_OWNER_AI_PLAYBACK,
        requestId,
        stereo,
        chunkSamples);
    if (written == 0) {
      char message[96] = {};
      snprintf(
          message,
          sizeof(message),
          "cosyvoice playback write failed offset=%lu samples=%lu",
          static_cast<unsigned long>(offset),
          static_cast<unsigned long>(tts->sampleCount));
      logCloudLabel(message);
      ok = false;
      break;
    }
    offset += written;
  }
  BikeMbAudioSession_Release(BIKE_MB_AUDIO_SESSION_OWNER_AI_PLAYBACK, requestId);
  return ok;
}

template <typename Writer>
bool writeHttpJsonRequest(
    WiFiClientSecure *client,
    const char *label,
    const char *endpoint,
    size_t contentLength,
    bool enableSse,
    Writer writer) {
  UrlParts url = {};
  if (client == nullptr || !parseHttpsUrl(endpoint, &url)) {
    char message[48] = {};
    snprintf(message, sizeof(message), "%s url parse failed", label);
    logCloudLabel(message);
    return false;
  }

#if BIKE_MB_AI_TLS_INSECURE_TEST_ONLY
  client->setInsecure();
#endif
  client->setTimeout(kHttpsTimeoutMs);
  client->setHandshakeTimeout(kTlsHandshakeTimeoutSeconds);
  logCloudMemory(label);
  char message[48] = {};
  snprintf(message, sizeof(message), "%s connect start", label);
  logCloudLabel(message);
  if (!client->connect(url.host, url.port)) {
    snprintf(message, sizeof(message), "%s connect failed", label);
    logCloudLabel(message);
    return false;
  }
  snprintf(message, sizeof(message), "%s connect ok", label);
  logCloudLabel(message);
  client->print("POST ");
  client->print(url.path);
  client->print(" HTTP/1.1\r\nHost: ");
  client->print(url.host);
  client->print("\r\nAuthorization: Bearer ");
  client->print(BIKE_MB_AI_DASHSCOPE_TOKEN);
  client->print("\r\nContent-Type: application/json\r\nAccept: ");
  client->print(enableSse ? "text/event-stream" : "application/json");
  client->print("\r\n");
  if (enableSse) {
    client->print("X-DashScope-SSE: enable\r\n");
  }
  client->print("Connection: close\r\nContent-Length: ");
  client->print(contentLength);
  client->print("\r\n\r\n");
  if (!writer()) {
    char message[48] = {};
    snprintf(message, sizeof(message), "%s post failed", label);
    logCloudLabel(message);
    return false;
  }
  return true;
}

bool readStatusAndHeaders(WiFiClientSecure *client, int *statusCode) {
  if (client == nullptr || statusCode == nullptr) {
    return false;
  }
  const String statusLine = client->readStringUntil('\n');
  const int firstSpace = statusLine.indexOf(' ');
  *statusCode = firstSpace >= 0 ? statusLine.substring(firstSpace + 1).toInt() : 0;
  while (client->connected()) {
    const String header = client->readStringUntil('\n');
    if (header == "\r" || header.length() == 0) {
      break;
    }
  }
  return *statusCode >= 200 && *statusCode < 300;
}

bool readJsonBody(WiFiClientSecure *client, char *out, size_t outCapacity) {
  if (client == nullptr || out == nullptr || outCapacity == 0) {
    return false;
  }
  size_t length = 0;
  const uint32_t deadline = millis() + kHttpsTimeoutMs;
  while ((client->connected() || client->available()) &&
         static_cast<int32_t>(millis() - deadline) < 0) {
    while (client->available()) {
      const char c = static_cast<char>(client->read());
      if (length + 1U < outCapacity) {
        out[length++] = c;
      }
    }
    delay(1);
  }
  out[length] = '\0';
  return length > 0;
}

void rememberSseLine(TtsSseReadStats *stats, const char *line) {
  if (stats == nullptr || line == nullptr || line[0] == '\0') {
    return;
  }
  size_t i = 0;
  while (line[i] != '\0' && i + 1U < sizeof(stats->lastLine)) {
    const char c = line[i];
    stats->lastLine[i] = (c >= 32 && c <= 126) ? c : ' ';
    ++i;
  }
  stats->lastLine[i] = '\0';
}

bool handleSseLine(
    const char *line,
    BikeMbCosyVoicePcmStream *stream,
    BikeMbCosyVoicePcmSink sink,
    void *context,
    TtsSseReadStats *stats) {
  if (stats != nullptr) {
    ++stats->lineCount;
    rememberSseLine(stats, line);
    if (strstr(line, "\"data\":\"") != nullptr) {
      ++stats->audioLineCount;
    }
  }
  return BikeMbCosyVoice_HandleSseLineWithStream(line, stream, sink, context);
}

bool readSseBody(
    WiFiClientSecure *client,
    BikeMbCosyVoicePcmSink sink,
    void *context,
    TtsSseReadStats *stats) {
  if (client == nullptr || sink == nullptr) {
    return false;
  }

  char *line = static_cast<char *>(ps_malloc(kMaxSseLineBytes));
  if (line == nullptr) {
    logCloudLabel("cosyvoice sse buffer allocation failed");
    return false;
  }

  BikeMbCosyVoicePcmStream stream = {};
  BikeMbCosyVoice_PcmStreamInit(&stream);
  size_t length = 0;
  bool ok = true;
  const uint32_t deadline = millis() + kHttpsTimeoutMs;
  while ((client->connected() || client->available()) &&
         static_cast<int32_t>(millis() - deadline) < 0 && ok) {
    while (client->available()) {
      const char c = static_cast<char>(client->read());
      if (c == '\n') {
        line[length] = '\0';
        ok = handleSseLine(line, &stream, sink, context, stats);
        length = 0;
        if (!ok) {
          if (stats != nullptr) {
            stats->failure = "cosyvoice sse handler failed";
          }
          break;
        }
      } else if (c != '\r' && length + 1U < kMaxSseLineBytes) {
        line[length++] = c;
      } else if (c != '\r') {
        logCloudLabel("cosyvoice sse line too large");
        if (stats != nullptr) {
          stats->failure = "cosyvoice sse line too large";
        }
        ok = false;
        break;
      }
    }
    delay(1);
  }

  if (ok && length > 0) {
    line[length] = '\0';
    ok = handleSseLine(line, &stream, sink, context, stats);
    if (!ok && stats != nullptr) {
      stats->failure = "cosyvoice sse handler failed";
    }
  }
  if (ok) {
    ok = BikeMbCosyVoice_PcmStreamFinish(&stream, sink, context);
    if (!ok && stats != nullptr) {
      stats->failure = "cosyvoice sse pcm finish failed";
    }
  }
  if (ok && stats != nullptr && stats->audioLineCount == 0) {
    stats->failure = "cosyvoice sse no audio data";
  }
  free(line);
  return ok;
}

bool extractJsonString(const char *json, const char *key, char *out, size_t outCapacity) {
  if (json == nullptr || key == nullptr || out == nullptr || outCapacity == 0) {
    return false;
  }
  const char *start = strstr(json, key);
  if (start == nullptr) {
    return false;
  }
  start += strlen(key);
  size_t written = 0;
  bool escaping = false;
  while (*start != '\0') {
    const char c = *start++;
    if (escaping) {
      if (written + 1U < outCapacity) {
        out[written++] = c == 'n' ? '\n' : c;
      }
      escaping = false;
      continue;
    }
    if (c == '\\') {
      escaping = true;
      continue;
    }
    if (c == '"') {
      out[written] = '\0';
      return written > 0;
    }
    if (written + 1U < outCapacity) {
      out[written++] = c;
    }
  }
  out[written] = '\0';
  return written > 0;
}

bool postQwenAsr(const BikeMbAudioClip *clip, char *out, size_t outCapacity) {
  logCloudLabel("qwen asr stage start");
  const uint32_t stageStartMs = cloudNowMs();
  CountSinkContext count = {};
  if (!BikeMbQwenAsr_WriteRequestJson(clip, countSink, &count)) {
    return false;
  }
  WiFiClientSecure client;
  BufferedClientSinkContext sink = {&client};
  if (!writeHttpJsonRequest(
          &client,
          "qwen asr",
          BikeMbAiConfig::kQwenAsrEndpoint,
          count.length,
          false,
          [&]() {
            return BikeMbQwenAsr_WriteRequestJson(clip, bufferedClientSink, &sink) &&
                   flushBufferedClientSink(&sink);
          })) {
    return false;
  }
  logCloudDuration("qwen asr", "request sent", stageStartMs);
  int status = 0;
  char body[kMaxJsonResponseBytes] = {};
  const bool httpOk = readStatusAndHeaders(&client, &status);
  logCloudDuration("qwen asr", "headers", stageStartMs);
  const bool bodyOk = readJsonBody(&client, body, sizeof(body));
  logCloudDuration("qwen asr", "body", stageStartMs);
  const bool ok =
      httpOk && bodyOk &&
      (extractJsonString(body, "\"content\":\"", out, outCapacity) ||
       extractJsonString(body, "\"text\":\"", out, outCapacity) ||
       extractJsonString(body, "\"transcription\":\"", out, outCapacity));
  if (!ok) {
    logCloudError("qwen asr", body);
  } else {
    logCloudText("qwen asr text", out);
  }
  logCloudStatus("qwen asr", status, ok);
  return ok;
}

bool postQwenChat(const char *text, char *out, size_t outCapacity) {
  const uint32_t stageStartMs = cloudNowMs();
  const size_t textLength = strlen(text);
  CountSinkContext count = {};
  if (!BikeMbQwenChat_WriteRequestJson(text, textLength, countSink, &count)) {
    return false;
  }
  WiFiClientSecure client;
  BufferedClientSinkContext sink = {&client};
  if (!writeHttpJsonRequest(
          &client,
          "qwen chat",
          BikeMbAiConfig::kQwenChatEndpoint,
          count.length,
          false,
          [&]() {
            return BikeMbQwenChat_WriteRequestJson(
                       text, textLength, bufferedClientSink, &sink) &&
                   flushBufferedClientSink(&sink);
          })) {
    return false;
  }
  logCloudDuration("qwen chat", "request sent", stageStartMs);
  int status = 0;
  char body[kMaxJsonResponseBytes] = {};
  char rawAnswer[BikeMbAiConfig::kMaxAnswerBytes] = {};
  const bool httpOk = readStatusAndHeaders(&client, &status);
  logCloudDuration("qwen chat", "headers", stageStartMs);
  const bool bodyOk = readJsonBody(&client, body, sizeof(body));
  logCloudDuration("qwen chat", "body", stageStartMs);
  const bool ok =
      httpOk && bodyOk &&
      extractJsonString(body, "\"content\":\"", rawAnswer, sizeof(rawAnswer)) &&
      BikeMbQwenChat_CopyBoundedAnswer(rawAnswer, out, outCapacity) > 0;
  if (!ok) {
    logCloudError("qwen chat", body);
  } else {
    logCloudText("qwen chat answer", out);
  }
  logCloudStatus("qwen chat", status, ok);
  return ok;
}

void logTtsSseFailure(const TtsSseReadStats &stats, const TtsPcmBufferContext &tts) {
  if (tts.overflow) {
    logCloudLabel("cosyvoice sse pcm overflow");
  } else if (stats.failure != nullptr) {
    logCloudLabel(stats.failure);
  } else if (stats.audioLineCount == 0) {
    logCloudLabel("cosyvoice sse no audio data");
  } else {
    logCloudLabel("cosyvoice sse pcm missing");
  }
  if (stats.lastLine[0] != '\0') {
    char message[176] = {};
    snprintf(
        message,
        sizeof(message),
        "cosyvoice sse last line=%s",
        stats.lastLine);
    logCloudLabel(message);
  }
}

bool postCosyVoiceTts(const char *text, uint32_t requestId) {
  const size_t textLength = strlen(text);
  CountSinkContext count = {};
  if (!BikeMbCosyVoice_WriteRequestJson(text, textLength, countSink, &count)) {
    return false;
  }

  TtsPcmBufferContext tts = {};
  tts.samples = static_cast<int16_t *>(ps_malloc(kMaxTtsPcmSamples * sizeof(int16_t)));
  tts.capacitySamples = kMaxTtsPcmSamples;
  if (tts.samples == nullptr) {
    logCloudLabel("cosyvoice pcm buffer allocation failed");
    return false;
  }

  int status = 0;
  bool readyToPlay = false;
  for (uint8_t attempt = 1; attempt <= kTtsMaxAttempts && !readyToPlay; ++attempt) {
    if (!isRequestValid(requestId)) {
      break;
    }
    tts.sampleCount = 0;
    tts.overflow = false;
    const uint32_t stageStartMs = cloudNowMs();
    WiFiClientSecure client;
    BufferedClientSinkContext sink = {&client};
    const bool posted = writeHttpJsonRequest(
        &client,
        "cosyvoice",
        BikeMbAiConfig::kCosyVoiceTtsEndpoint,
        count.length,
        true,
        [&]() {
          return BikeMbCosyVoice_WriteRequestJson(text, textLength, bufferedClientSink, &sink) &&
                 flushBufferedClientSink(&sink);
        });
    if (posted) {
      logCloudDuration("cosyvoice", "request sent", stageStartMs);
    }
    const bool httpOk = posted && readStatusAndHeaders(&client, &status);
    if (posted) {
      logCloudDuration("cosyvoice", "headers", stageStartMs);
    }
    char body[kMaxJsonResponseBytes] = {};
    if (posted && !httpOk && readJsonBody(&client, body, sizeof(body))) {
      logCloudDuration("cosyvoice", "body", stageStartMs);
      logCloudError("cosyvoice", body);
    }
    TtsSseReadStats stats = {};
    const bool streamed = httpOk && readSseBody(&client, ttsPcmBufferSink, &tts, &stats);
    if (httpOk) {
      logCloudDuration("cosyvoice", "body", stageStartMs);
    }
    readyToPlay = streamed && tts.sampleCount > 0;
    if (readyToPlay) {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
      Serial.print("[BikeMB][ai] cosyvoice pcm samples=");
      Serial.println(tts.sampleCount);
#else
      ESP_LOGI("BikeMbCloud", "cosyvoice pcm samples=%u", static_cast<unsigned>(tts.sampleCount));
#endif
      break;
    }
    if (httpOk) {
      logCloudLabel("cosyvoice pcm missing");
      logTtsSseFailure(stats, tts);
    }
    if (attempt < kTtsMaxAttempts && isRequestValid(requestId)) {
      logCloudLabel("cosyvoice retry");
      delay(250);
    }
  }
  const bool played = readyToPlay && playTtsPcmBuffer(&tts, requestId);
  logCloudStatus("cosyvoice", status, played);
  free(tts.samples);
  return played;
}
#endif

#if defined(BIKE_MB_USE_ESPIDF_RUNTIME)
struct IdfCountSinkContext {
  size_t length;
};

struct IdfBufferedClientSinkContext {
  esp_http_client_handle_t client;
  char buffer[kHttpWriteBufferBytes];
  size_t length;
};

bool idfCountSink(const char *, size_t length, void *context) {
  IdfCountSinkContext *count = static_cast<IdfCountSinkContext *>(context);
  if (count == nullptr) {
    return false;
  }
  count->length += length;
  return true;
}

bool flushIdfBufferedClientSink(IdfBufferedClientSinkContext *out) {
  if (out == nullptr || out->client == nullptr) {
    return false;
  }
  if (out->length == 0) {
    return true;
  }
  const int written = esp_http_client_write(out->client, out->buffer, out->length);
  const bool ok = written == static_cast<int>(out->length);
  out->length = 0;
  return ok;
}

bool idfBufferedClientSink(const char *data, size_t length, void *context) {
  IdfBufferedClientSinkContext *out = static_cast<IdfBufferedClientSinkContext *>(context);
  if (out == nullptr || out->client == nullptr || data == nullptr) {
    return false;
  }
  size_t offset = 0;
  while (offset < length) {
    if (out->length == sizeof(out->buffer)) {
      if (!flushIdfBufferedClientSink(out)) {
        return false;
      }
    }
    const size_t available = sizeof(out->buffer) - out->length;
    const size_t remaining = length - offset;
    const size_t chunk = remaining < available ? remaining : available;
    memcpy(out->buffer + out->length, data + offset, chunk);
    out->length += chunk;
    offset += chunk;
  }
  return true;
}

bool extractJsonStringIdf(const char *json, const char *key, char *out, size_t outCapacity) {
  if (json == nullptr || key == nullptr || out == nullptr || outCapacity == 0) {
    return false;
  }
  const char *start = strstr(json, key);
  if (start == nullptr) {
    return false;
  }
  start += strlen(key);
  size_t written = 0;
  bool escaping = false;
  while (*start != '\0') {
    const char c = *start++;
    if (escaping) {
      if (written + 1U < outCapacity) {
        out[written++] = c == 'n' ? '\n' : c;
      }
      escaping = false;
      continue;
    }
    if (c == '\\') {
      escaping = true;
      continue;
    }
    if (c == '"') {
      out[written] = '\0';
      return written > 0;
    }
    if (written + 1U < outCapacity) {
      out[written++] = c;
    }
  }
  out[written] = '\0';
  return written > 0;
}

bool readIdfJsonBody(esp_http_client_handle_t client, char *out, size_t outCapacity) {
  if (client == nullptr || out == nullptr || outCapacity == 0) {
    return false;
  }
  size_t length = 0;
  while (length + 1U < outCapacity) {
    const int read = esp_http_client_read(
        client, out + length, static_cast<int>(outCapacity - length - 1U));
    if (read <= 0) {
      break;
    }
    length += static_cast<size_t>(read);
  }
  out[length] = '\0';
  return length > 0;
}

template <typename Writer>
bool postIdfJson(
    const char *label,
    const char *endpoint,
    size_t contentLength,
    char *body,
    size_t bodyCapacity,
    int *status,
    Writer writer) {
  if (label == nullptr || endpoint == nullptr || body == nullptr || status == nullptr) {
    return false;
  }

  esp_http_client_config_t config = {};
  config.url = endpoint;
  config.timeout_ms = kHttpsTimeoutMs;
  config.crt_bundle_attach = esp_crt_bundle_attach;

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) {
    char message[64] = {};
    snprintf(message, sizeof(message), "%s init failed", label);
    logCloudLabel(message);
    return false;
  }

  bool ok = false;
  do {
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Authorization", "Bearer " BIKE_MB_AI_DASHSCOPE_TOKEN);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "Connection", "close");

    logCloudMemory(label);
    char message[64] = {};
    snprintf(message, sizeof(message), "%s connect start", label);
    logCloudLabel(message);
    esp_err_t err = esp_http_client_open(client, static_cast<int>(contentLength));
    if (err != ESP_OK) {
      snprintf(message, sizeof(message), "%s connect failed", label);
      logCloudLabel(message);
      break;
    }
    snprintf(message, sizeof(message), "%s connect ok", label);
    logCloudLabel(message);

    IdfBufferedClientSinkContext sink = {client};
    if (!writer(&sink) || !flushIdfBufferedClientSink(&sink)) {
      snprintf(message, sizeof(message), "%s post failed", label);
      logCloudLabel(message);
      break;
    }

    const int64_t headerLength = esp_http_client_fetch_headers(client);
    (void)headerLength;
    *status = esp_http_client_get_status_code(client);
    const bool httpOk = *status >= 200 && *status < 300;
    const bool bodyOk = readIdfJsonBody(client, body, bodyCapacity);
    ok = httpOk && bodyOk;
  } while (false);

  esp_http_client_cleanup(client);
  return ok;
}

bool postQwenAsrIdf(const BikeMbAudioClip *clip, char *out, size_t outCapacity) {
  logCloudLabel("qwen asr stage start");
  const uint32_t stageStartMs = cloudNowMs();
  IdfCountSinkContext count = {};
  if (!BikeMbQwenAsr_WriteRequestJson(clip, idfCountSink, &count)) {
    return false;
  }

  int status = 0;
  char body[kMaxJsonResponseBytes] = {};
  const bool posted = postIdfJson(
      "qwen asr",
      BikeMbAiConfig::kQwenAsrEndpoint,
      count.length,
      body,
      sizeof(body),
      &status,
      [&](IdfBufferedClientSinkContext *sink) {
        return BikeMbQwenAsr_WriteRequestJson(clip, idfBufferedClientSink, sink);
      });
  logCloudDuration("qwen asr", "body", stageStartMs);
  const bool ok =
      posted &&
      (extractJsonStringIdf(body, "\"content\":\"", out, outCapacity) ||
       extractJsonStringIdf(body, "\"text\":\"", out, outCapacity) ||
       extractJsonStringIdf(body, "\"transcription\":\"", out, outCapacity));
  if (ok) {
    logCloudText("qwen asr text", out);
  }
  logCloudStatus("qwen asr", status, ok);
  return ok;
}

bool postQwenChatIdf(const char *text, char *out, size_t outCapacity) {
  const uint32_t stageStartMs = cloudNowMs();
  const size_t textLength = strlen(text);
  IdfCountSinkContext count = {};
  if (!BikeMbQwenChat_WriteRequestJson(text, textLength, idfCountSink, &count)) {
    return false;
  }

  int status = 0;
  char body[kMaxJsonResponseBytes] = {};
  char rawAnswer[BikeMbAiConfig::kMaxAnswerBytes] = {};
  const bool posted = postIdfJson(
      "qwen chat",
      BikeMbAiConfig::kQwenChatEndpoint,
      count.length,
      body,
      sizeof(body),
      &status,
      [&](IdfBufferedClientSinkContext *sink) {
        return BikeMbQwenChat_WriteRequestJson(
            text, textLength, idfBufferedClientSink, sink);
      });
  logCloudDuration("qwen chat", "body", stageStartMs);
  const bool ok =
      posted &&
      extractJsonStringIdf(body, "\"content\":\"", rawAnswer, sizeof(rawAnswer)) &&
      BikeMbQwenChat_CopyBoundedAnswer(rawAnswer, out, outCapacity) > 0;
  if (ok) {
    logCloudText("qwen chat answer", out);
  }
  logCloudStatus("qwen chat", status, ok);
  return ok;
}
#endif

bool runRealStage(const BikeMbCloudJob &job, const char **detail) {
  if (detail == nullptr) {
    return false;
  }
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  if (!hasDashScopeToken()) {
    *detail = "Bailian token unavailable";
    logCloudLabel("bailian token unavailable");
    return false;
  }
  switch (job.stage) {
    case BIKE_MB_CLOUD_STAGE_STT: {
      BikeMbAudioClip clip = {};
      if (!takeStoredClip(job.requestId, &clip)) {
        *detail = "audio clip unavailable";
        return false;
      }
      const bool ok = postQwenAsr(&clip, s_sttText, sizeof(s_sttText));
      BikeMbAudioSession_ReleaseClip(&clip);
      *detail = ok ? "qwen asr ready" : "qwen asr failed";
      return ok;
    }
    case BIKE_MB_CLOUD_STAGE_LLM: {
      const bool ok = postQwenChat(s_sttText, s_answerText, sizeof(s_answerText));
      *detail = BikeMbQwenChat_RedactedLogLabel(ok);
      return ok;
    }
    case BIKE_MB_CLOUD_STAGE_TTS: {
      const bool ok = postCosyVoiceTts(s_answerText, job.requestId);
      *detail = ok ? "cosyvoice ready" : "cosyvoice failed";
      return ok;
    }
  }
#elif defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  if (!hasDashScopeToken()) {
    *detail = "Bailian token unavailable";
    logCloudLabel("bailian token unavailable");
    return false;
  }
  switch (job.stage) {
    case BIKE_MB_CLOUD_STAGE_STT: {
      BikeMbAudioClip clip = {};
      if (!takeStoredClip(job.requestId, &clip)) {
        *detail = "audio clip unavailable";
        return false;
      }
      const bool ok = postQwenAsrIdf(&clip, s_sttText, sizeof(s_sttText));
      BikeMbAudioSession_ReleaseClip(&clip);
      *detail = ok ? "qwen asr ready" : "qwen asr failed";
      return ok;
    }
    case BIKE_MB_CLOUD_STAGE_LLM: {
      const bool ok = postQwenChatIdf(s_sttText, s_answerText, sizeof(s_answerText));
      *detail = BikeMbQwenChat_RedactedLogLabel(ok);
      return ok;
    }
    case BIKE_MB_CLOUD_STAGE_TTS:
      *detail = "idf cosyvoice playback unavailable";
      logCloudLabel("idf cosyvoice playback unavailable");
      return false;
  }
#else
  (void)job;
  *detail = "provider unavailable";
#endif
  return false;
}

void cloudTask(void *) {
  BikeMbCloudJob job = {};
  for (;;) {
    if (xQueueReceive(s_queue, &job, portMAX_DELAY) != pdTRUE) {
      continue;
    }
    if (!isRequestValid(job.requestId)) {
      continue;
    }
    const char *detail = nullptr;
    bool success = false;
#if BIKE_MB_AI_USE_MOCK_PROVIDERS
    vTaskDelay(pdMS_TO_TICKS(delayForStage(job.stage)));
    success = isRequestValid(job.requestId);
    detail = labelForStage(job.stage);
#else
    success = runRealStage(job, &detail);
#endif
    if (isRequestValid(job.requestId) && s_sink != nullptr) {
      s_sink(job.stage, job.requestId, success, detail);
    }
  }
}
}

bool BikeMbCloudWorker_Init(BikeMbCloudResultSink sink) {
  s_sink = sink;
  if (s_queue == nullptr) {
    s_queue = xQueueCreate(kQueueLength, sizeof(BikeMbCloudJob));
    if (s_queue == nullptr) {
      return false;
    }
    BaseType_t created = xTaskCreatePinnedToCore(
        cloudTask,
        "bikemb_cloud",
        BikeMbAiConfig::kCloudWorkerStackBytes,
        nullptr,
        1,
        nullptr,
        BIKE_RUNTIME_CORE_RUNTIME);
    if (created != pdPASS) {
      return false;
    }
  }
  return true;
}

bool BikeMbCloudWorker_Submit(const BikeMbCloudJob &job) {
  if (s_queue == nullptr) {
    return false;
  }
  return xQueueSend(s_queue, &job, 0) == pdTRUE;
}

void BikeMbCloudWorker_CancelBefore(uint32_t validRequestId) {
  portENTER_CRITICAL(&s_requestMux);
  if (validRequestId > s_minValidRequestId) {
    s_minValidRequestId = validRequestId;
  }
  portEXIT_CRITICAL(&s_requestMux);
  releaseStoredClip();
}

bool BikeMbCloudWorker_SetCaptureClip(uint32_t requestId, BikeMbAudioClip *clip) {
  if (clip == nullptr || clip->samples == nullptr || clip->sampleCount == 0) {
    return false;
  }

  releaseStoredClip();
  portENTER_CRITICAL(&s_clipMux);
  s_captureClip = *clip;
  s_captureClipRequestId = requestId;
  *clip = {};
  portEXIT_CRITICAL(&s_clipMux);
  return true;
}
