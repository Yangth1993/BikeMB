#include "bike_runtime.h"

#include "freertos/task.h"

#include "ai/ai_assistant.h"
#include "audio/audio_capture_self_test.h"
#include "audio/audio_prompts.h"
#include "audio/audio_self_test.h"
#include "audio/audio_session.h"
#include "input/ai_button.h"
#include "network/wifi_service.h"
#include "platform/bike_platform.h"
#include "platform/board_support.h"
#include "runtime/bike_runtime_plan.h"
#include "services/ui_service.h"
#include "voice/voice_commands.h"

#ifndef BIKE_MB_ENABLE_AI_ASSISTANT
#define BIKE_MB_ENABLE_AI_ASSISTANT 0
#endif

#ifndef BIKE_MB_ENABLE_AUDIO_SESSION
#define BIKE_MB_ENABLE_AUDIO_SESSION 0
#endif

namespace {

constexpr uint32_t kDashboardIntervalMs = 33;
constexpr uint32_t kRuntimePollMs = 5;
constexpr uint32_t kEventQueueLength = 16;
constexpr uint32_t kRuntimeTaskStackWords = 4096;
constexpr UBaseType_t kRuntimeTaskPriority = 4;
constexpr BaseType_t kRuntimeTaskCore = BIKE_RUNTIME_CORE_RUNTIME;

QueueHandle_t g_eventQueue = nullptr;
TaskHandle_t g_runtimeTask = nullptr;
uint32_t g_droppedLowPriorityEvents = 0;

bool IsLowPriorityEvent(BikeEventType type) {
  return type == BikeEventType::SystemTick || type == BikeEventType::DashboardTick;
}

void RuntimeTickTask(void *arg) {
  (void)arg;
  uint32_t lastDashboardMs = BikePlatform_Millis();

  while (true) {
    const uint32_t nowMs = BikePlatform_Millis();

#if BIKE_MB_ENABLE_AI_ASSISTANT
    BikeMbAiButton_Tick(nowMs);
#endif

    const BikeEvent systemTick = {
        .type = BikeEventType::SystemTick,
        .timestampMs = nowMs,
        .value = 0,
    };
    BikeRuntime_PostEvent(&systemTick, 0);

    if (nowMs - lastDashboardMs >= kDashboardIntervalMs) {
      lastDashboardMs = nowMs;
      const BikeEvent dashboardTick = {
          .type = BikeEventType::DashboardTick,
          .timestampMs = nowMs,
          .value = 0,
      };
      BikeRuntime_PostEvent(&dashboardTick, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(kRuntimePollMs));
  }
}

}  // namespace

void BikeRuntime_Init() {
  if (g_eventQueue == nullptr) {
    g_eventQueue = xQueueCreate(kEventQueueLength, sizeof(BikeEvent));
  }

  BoardSupport_Init();
}

void BikeRuntime_Start() {
  if (g_eventQueue == nullptr) {
    BikeRuntime_Init();
  }

  UiService_Start(g_eventQueue);

#if BIKE_MB_ENABLE_AUDIO_SESSION
  BikeMbAudioSession_Init();
#endif

#if BIKE_MB_ENABLE_AI_ASSISTANT
  BikeMbAiAssistant_Init();
  BikeMbWifiService_Init();
  BikeMbAiButton_Init();
#endif

  BikeMbAudioCaptureSelfTest_Init();
  BikeMbAudioPrompts_Init();
  BikeMbAudioSelfTest_Init();
  BikeMbVoiceCommands_Init();

  if (g_runtimeTask == nullptr) {
    xTaskCreatePinnedToCore(
        RuntimeTickTask,
        "bike_runtime",
        kRuntimeTaskStackWords,
        nullptr,
        kRuntimeTaskPriority,
        &g_runtimeTask,
        kRuntimeTaskCore);
  }
}

bool BikeRuntime_PostEvent(const BikeEvent *event, TickType_t timeoutTicks) {
  if (g_eventQueue == nullptr || event == nullptr) {
    return false;
  }

  if (xQueueSend(g_eventQueue, event, timeoutTicks) == pdTRUE) {
    return true;
  }

  if (IsLowPriorityEvent(event->type)) {
    ++g_droppedLowPriorityEvents;
  }
  return false;
}

QueueHandle_t BikeRuntime_GetEventQueue() {
  return g_eventQueue;
}

uint32_t BikeRuntime_GetDroppedLowPriorityEvents() {
  return g_droppedLowPriorityEvents;
}
