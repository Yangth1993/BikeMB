#include "ui_service.h"

#include "esp_log.h"
#include "freertos/task.h"

#include "app/dashboard_app.h"
#include "platform/bike_platform.h"
#include "platform/lvgl_port.h"
#include "runtime/bike_event.h"
#include "runtime/bike_runtime_plan.h"

namespace {

constexpr const char *TAG = "BikeMB.UiService";
constexpr uint32_t kUiQueueWaitMs = 5;
constexpr uint32_t kUiTaskStackWords = 8192;
constexpr UBaseType_t kUiTaskPriority = 5;
constexpr BaseType_t kUiTaskCore = BIKE_RUNTIME_CORE_UI;

QueueHandle_t g_eventQueue = nullptr;
TaskHandle_t g_uiTask = nullptr;

void HandleEvent(const BikeEvent &event) {
  switch (event.type) {
    case BikeEventType::DashboardTick:
      DashboardApp_Tick(event.timestampMs);
      break;
    case BikeEventType::RenderStatsUpdate:
      DashboardApp_SetRenderWorkMs(event.value);
      break;
    case BikeEventType::ShowAiPage:
      DashboardApp_ShowAiPage();
      break;
    case BikeEventType::SystemTick:
    case BikeEventType::DiagnosticRequest:
    default:
      break;
  }
}

void UiTask(void *arg) {
  g_eventQueue = static_cast<QueueHandle_t>(arg);

  LvglPort_Init();
  DashboardApp_Init();
  ESP_LOGI(TAG, "LVGL UI service ready");

  uint32_t lastLvglTickMs = BikePlatform_Millis();

  while (true) {
    const uint32_t nowMs = BikePlatform_Millis();
    const uint32_t deltaMs = nowMs - lastLvglTickMs;
    lastLvglTickMs = nowMs;
    LvglPort_Tick(deltaMs);

    BikeEvent event = {};
    if (xQueueReceive(g_eventQueue, &event, pdMS_TO_TICKS(kUiQueueWaitMs)) == pdTRUE) {
      do {
        HandleEvent(event);
      } while (xQueueReceive(g_eventQueue, &event, 0) == pdTRUE);
    }

    const uint32_t renderWorkMs = LvglPort_Run();
    const BikeEvent renderStats = {
        .type = BikeEventType::RenderStatsUpdate,
        .timestampMs = BikePlatform_Millis(),
        .value = renderWorkMs,
    };
    HandleEvent(renderStats);
  }
}

}  // namespace

void UiService_Start(QueueHandle_t eventQueue) {
  if (g_uiTask != nullptr || eventQueue == nullptr) {
    return;
  }

  xTaskCreatePinnedToCore(
      UiTask,
      "bike_ui",
      kUiTaskStackWords,
      eventQueue,
      kUiTaskPriority,
      &g_uiTask,
      kUiTaskCore);
}
