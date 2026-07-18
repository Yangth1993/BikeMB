#include "ai_assistant.h"

#include <string.h>

#include "ai_config.h"
#include "ai_state_machine.h"
#include "cloud_worker.h"

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

QueueHandle_t s_commandQueue = nullptr;
BikeMbAiStateMachine s_machine;
BikeMbAiSnapshot s_snapshot;
portMUX_TYPE s_snapshotMux = portMUX_INITIALIZER_UNLOCKED;

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

void executeEffects(uint32_t effects) {
  if ((effects & BIKE_MB_AI_EFFECT_CANCEL_CLOUD) != 0) {
    BikeMbCloudWorker_CancelBefore(s_machine.snapshot.requestId);
  }
  if ((effects & BIKE_MB_AI_EFFECT_START_CAPTURE) != 0) {
    diag("ai mock capture start");
  }
  if ((effects & BIKE_MB_AI_EFFECT_FINISH_CAPTURE) != 0) {
    diag("ai mock capture finish");
  }
  if ((effects & BIKE_MB_AI_EFFECT_CANCEL_AUDIO) != 0) {
    diag("ai mock audio cancel");
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
  BaseType_t created = xTaskCreate(
      assistantTask,
      "bikemb_ai",
      BikeMbAiConfig::kAssistantStackBytes,
      nullptr,
      2,
      nullptr);
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
