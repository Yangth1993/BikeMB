#include "ai_assistant.h"

#include <stdio.h>
#include <string.h>

#include "ai_config.h"
#include "ai_state_machine.h"
#include "audio/audio_session.h"
#include "cloud_worker.h"
#include "runtime/bike_runtime_plan.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
#include <Arduino.h>
#else
#include "esp_log.h"
#include "esp_timer.h"
#endif

namespace {
constexpr uint32_t kCommandQueueLength = 8;
constexpr TickType_t kTickInterval = pdMS_TO_TICKS(20);
constexpr uint32_t kMockReplySampleRateHz = 16000;
constexpr uint32_t kMockReplyToneHz = 740;
constexpr uint32_t kMockReplyToneMs = 180;
constexpr int16_t kMockReplyAmplitude = 9000;
constexpr size_t kMockReplyChunkFrames = 96;

QueueHandle_t s_commandQueue = nullptr;
BikeMbAiStateMachine s_machine;
BikeMbAiSnapshot s_snapshot;
portMUX_TYPE s_snapshotMux = portMUX_INITIALIZER_UNLOCKED;
bool s_captureActive = false;
uint32_t s_captureRequestId = 0;

uint32_t nowMs() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  return millis();
#else
  return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
#endif
}

void diag(const char *message) {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  Serial.println(message);
#else
  ESP_LOGI("BikeMbAi", "%s", message);
#endif
}

void publishSnapshot() {
  portENTER_CRITICAL(&s_snapshotMux);
  s_snapshot = s_machine.snapshot;
  portEXIT_CRITICAL(&s_snapshotMux);
}

void enqueueEvent(BikeMbAiEvent event) {
  if (s_commandQueue == nullptr) {
    return;
  }
  event.nowMs = event.nowMs == 0 ? nowMs() : event.nowMs;
  xQueueSend(s_commandQueue, &event, 0);
}

void submitCloudJob(BikeMbCloudStage stage) {
  BikeMbCloudJob job = {};
  job.stage = stage;
  job.requestId = s_machine.snapshot.requestId;
  job.deadlineMs = s_machine.deadlineMs;
  BikeMbCloudWorker_Submit(job);
}

void logClipSummary(const char *prefix, const BikeMbAudioClip &clip) {
  char message[96];
  snprintf(
      message,
      sizeof(message),
      "%s samples=%lu limit=%u",
      prefix,
      static_cast<unsigned long>(clip.sampleCount),
      clip.hitLimit ? 1U : 0U);
  diag(message);
}

void finishActiveCapture(const char *prefix, bool handoffToCloud) {
  if (!s_captureActive) {
    return;
  }

  BikeMbAudioClip clip = {};
  const uint32_t requestId = s_captureRequestId;
  s_captureActive = false;
  s_captureRequestId = 0;
  if (BikeMbAudioSession_FinishCapture(requestId, &clip)) {
    logClipSummary(prefix, clip);
    if (handoffToCloud && BikeMbCloudWorker_SetCaptureClip(requestId, &clip)) {
      return;
    }
    BikeMbAudioSession_ReleaseClip(&clip);
    return;
  }
  diag("ai capture finish unavailable");
}

void startCapture(uint32_t requestId) {
  finishActiveCapture("ai capture replaced", false);
  if (BikeMbAudioSession_StartCapture(requestId, BikeMbAiConfig::kMaxRecordingMs)) {
    s_captureActive = true;
    s_captureRequestId = requestId;
    diag("ai capture start");
    return;
  }
  diag("ai capture unavailable");
}

void pollCaptureIfActive() {
  if (!s_captureActive) {
    return;
  }
  if (!BikeMbAudioSession_PollCapture(s_captureRequestId)) {
    finishActiveCapture("ai capture stopped", false);
  }
}

void cancelAudio() {
  finishActiveCapture("ai capture cancel", false);
  if (BikeMbAudioSession_GetOwner() == BIKE_MB_AUDIO_SESSION_OWNER_AI_PLAYBACK) {
    BikeMbAudioSession_ReleaseAll();
  }
  diag("ai audio cancel");
}

void playMockAssistantReply(uint32_t requestId) {
  if (!BikeMbAudioSession_Acquire(BIKE_MB_AUDIO_SESSION_OWNER_AI_PLAYBACK, requestId)) {
    diag("ai mock reply audio unavailable");
    return;
  }

  int16_t chunk[kMockReplyChunkFrames * 2] = {};
  const uint32_t totalFrames =
      (kMockReplySampleRateHz * kMockReplyToneMs) / 1000U;
  const uint32_t halfPeriod =
      (kMockReplySampleRateHz / kMockReplyToneHz) / 2U;
  uint32_t writtenFrames = 0;
  while (writtenFrames < totalFrames) {
    const size_t framesThisChunk =
        (totalFrames - writtenFrames) < kMockReplyChunkFrames
            ? static_cast<size_t>(totalFrames - writtenFrames)
            : kMockReplyChunkFrames;
    for (size_t frame = 0; frame < framesThisChunk; ++frame) {
      const uint32_t sampleIndex = writtenFrames + static_cast<uint32_t>(frame);
      const bool high =
          halfPeriod == 0 ? true : ((sampleIndex / halfPeriod) % 2U) == 0;
      const int16_t sample = high ? kMockReplyAmplitude : -kMockReplyAmplitude;
      chunk[frame * 2] = sample;
      chunk[frame * 2 + 1] = sample;
    }
    const size_t framesWritten = BikeMbAudioSession_WriteStereoPcm(
        BIKE_MB_AUDIO_SESSION_OWNER_AI_PLAYBACK,
        requestId,
        chunk,
        framesThisChunk);
    if (framesWritten == 0) {
      break;
    }
    writtenFrames += static_cast<uint32_t>(framesWritten);
  }
  BikeMbAudioSession_Release(BIKE_MB_AUDIO_SESSION_OWNER_AI_PLAYBACK, requestId);
}

void executeEffects(uint32_t effects) {
  if ((effects & BIKE_MB_AI_EFFECT_CANCEL_CLOUD) != 0) {
    BikeMbCloudWorker_CancelBefore(s_machine.snapshot.requestId);
  }
  if ((effects & BIKE_MB_AI_EFFECT_CANCEL_AUDIO) != 0) {
    cancelAudio();
  }
  if ((effects & BIKE_MB_AI_EFFECT_START_CAPTURE) != 0) {
    startCapture(s_machine.snapshot.requestId);
  }
  if ((effects & BIKE_MB_AI_EFFECT_FINISH_CAPTURE) != 0) {
    finishActiveCapture("ai capture finish", true);
  }
  if ((effects & BIKE_MB_AI_EFFECT_SUBMIT_STT) != 0) {
    submitCloudJob(BIKE_MB_CLOUD_STAGE_STT);
  }
  if ((effects & BIKE_MB_AI_EFFECT_SUBMIT_LLM) != 0) {
    submitCloudJob(BIKE_MB_CLOUD_STAGE_LLM);
  }
  if ((effects & BIKE_MB_AI_EFFECT_SUBMIT_TTS) != 0) {
    submitCloudJob(BIKE_MB_CLOUD_STAGE_TTS);
  }
}

void processEvent(const BikeMbAiEvent &event) {
  const uint32_t effects = BikeMbAiStateMachine_Dispatch(&s_machine, event);
  publishSnapshot();
  executeEffects(effects);
}

void assistantTask(void *) {
  BikeMbAiEvent event = {};
  for (;;) {
    pollCaptureIfActive();
    if (xQueueReceive(s_commandQueue, &event, kTickInterval) == pdTRUE) {
      processEvent(event);
      continue;
    }
    BikeMbAiEvent tick = {};
    tick.type = BIKE_MB_AI_EVENT_TICK;
    tick.nowMs = nowMs();
    processEvent(tick);
  }
}

void onCloudResult(
    BikeMbCloudStage stage,
    uint32_t requestId,
    bool success,
    const char *detail) {
  BikeMbAiEvent event = {};
  event.nowMs = nowMs();
  event.requestId = requestId;
  event.detail = detail;
  if (!success) {
    event.type = BIKE_MB_AI_EVENT_FAILURE;
    enqueueEvent(event);
    return;
  }
  switch (stage) {
    case BIKE_MB_CLOUD_STAGE_STT:
      event.type = BIKE_MB_AI_EVENT_STT_READY;
      enqueueEvent(event);
      break;
    case BIKE_MB_CLOUD_STAGE_LLM:
      event.type = BIKE_MB_AI_EVENT_LLM_READY;
      enqueueEvent(event);
      break;
    case BIKE_MB_CLOUD_STAGE_TTS:
      event.type = BIKE_MB_AI_EVENT_TTS_STARTED;
      enqueueEvent(event);
#if BIKE_MB_AI_USE_MOCK_PROVIDERS
      playMockAssistantReply(requestId);
#endif
      event.type = BIKE_MB_AI_EVENT_PLAYBACK_DONE;
      enqueueEvent(event);
      break;
  }
}

void enqueueSimple(BikeMbAiEventType type) {
  BikeMbAiEvent event = {};
  event.type = type;
  enqueueEvent(event);
}
}

bool BikeMbAiAssistant_Init(void) {
  BikeMbAiStateMachine_Init(
      &s_machine,
      BIKE_MB_ENABLE_AI_ASSISTANT != 0,
      nowMs());
  publishSnapshot();

#if !BIKE_MB_ENABLE_AI_ASSISTANT
  return true;
#else
  if (s_commandQueue == nullptr) {
    s_commandQueue = xQueueCreate(kCommandQueueLength, sizeof(BikeMbAiEvent));
    if (s_commandQueue == nullptr) {
      return false;
    }
  }
  if (!BikeMbCloudWorker_Init(onCloudResult)) {
    return false;
  }
  BaseType_t created = xTaskCreatePinnedToCore(
      assistantTask,
      "bikemb_ai",
      BikeMbAiConfig::kAssistantStackBytes,
      nullptr,
      2,
      nullptr,
      BIKE_RUNTIME_CORE_RUNTIME);
  return created == pdPASS;
#endif
}

void BikeMbAiAssistant_OnButtonPressed(void) {
  enqueueSimple(BIKE_MB_AI_EVENT_BUTTON_PRESSED);
}

void BikeMbAiAssistant_OnButtonReleased(void) {
  enqueueSimple(BIKE_MB_AI_EVENT_BUTTON_RELEASED);
}

void BikeMbAiAssistant_Cancel(void) {
  enqueueSimple(BIKE_MB_AI_EVENT_CANCEL);
}

void BikeMbAiAssistant_SetWifiConnected(bool connected) {
  BikeMbAiEvent event = {};
  event.type = BIKE_MB_AI_EVENT_SET_WIFI;
  event.value = connected;
  enqueueEvent(event);
}

void BikeMbAiAssistant_GetSnapshot(BikeMbAiSnapshot *out) {
  if (out == nullptr) {
    return;
  }
  portENTER_CRITICAL(&s_snapshotMux);
  *out = s_snapshot;
  portEXIT_CRITICAL(&s_snapshotMux);
}
