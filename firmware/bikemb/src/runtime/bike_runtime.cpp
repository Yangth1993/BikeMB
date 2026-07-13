#include "bike_runtime.h"

#include "freertos/task.h"

#include "platform/bike_platform.h"
#include "platform/board_support.h"
#include "services/ui_service.h"

namespace {

constexpr uint32_t kDashboardIntervalMs = 33;
constexpr uint32_t kRuntimePollMs = 5;
constexpr uint32_t kEventQueueLength = 16;
constexpr uint32_t kRuntimeTaskStackWords = 4096;
constexpr UBaseType_t kRuntimeTaskPriority = 4;
constexpr BaseType_t kRuntimeTaskCore = 0;

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
