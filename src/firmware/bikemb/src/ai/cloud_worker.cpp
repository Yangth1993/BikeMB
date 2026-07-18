#include "cloud_worker.h"

#include "ai_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace {
constexpr uint32_t kQueueLength = 4;

QueueHandle_t s_queue = nullptr;
BikeMbCloudResultSink s_sink = nullptr;
portMUX_TYPE s_requestMux = portMUX_INITIALIZER_UNLOCKED;
uint32_t s_minValidRequestId = 0;

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

bool isRequestValid(uint32_t requestId) {
  portENTER_CRITICAL(&s_requestMux);
  const bool valid = requestId >= s_minValidRequestId;
  portEXIT_CRITICAL(&s_requestMux);
  return valid;
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
    vTaskDelay(pdMS_TO_TICKS(delayForStage(job.stage)));
    if (!isRequestValid(job.requestId)) {
      continue;
    }
    if (s_sink != nullptr) {
      s_sink(job.stage, job.requestId, true, labelForStage(job.stage));
    }
  }
}
}

bool BikeMbCloudWorker_Init(BikeMbCloudResultSink sink) {
  s_sink = sink;
#if BIKE_MB_AI_USE_MOCK_PROVIDERS
  if (s_queue == nullptr) {
    s_queue = xQueueCreate(kQueueLength, sizeof(BikeMbCloudJob));
    if (s_queue == nullptr) {
      return false;
    }
    BaseType_t created = xTaskCreate(
        cloudTask,
        "bikemb_cloud",
        BikeMbAiConfig::kCloudWorkerStackBytes,
        nullptr,
        1,
        nullptr);
    if (created != pdPASS) {
      return false;
    }
  }
  return true;
#else
  return true;
#endif
}

bool BikeMbCloudWorker_Submit(const BikeMbCloudJob &job) {
#if BIKE_MB_AI_USE_MOCK_PROVIDERS
  if (s_queue == nullptr) {
    return false;
  }
  return xQueueSend(s_queue, &job, 0) == pdTRUE;
#else
  if (s_sink != nullptr) {
    s_sink(job.stage, job.requestId, false, "provider unavailable");
  }
  return false;
#endif
}

void BikeMbCloudWorker_CancelBefore(uint32_t validRequestId) {
  portENTER_CRITICAL(&s_requestMux);
  if (validRequestId > s_minValidRequestId) {
    s_minValidRequestId = validRequestId;
  }
  portEXIT_CRITICAL(&s_requestMux);
}
