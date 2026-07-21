#include "ui_service.h"

#include "esp_heap_caps.h"
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
constexpr uint32_t kUiDiagnosticsIntervalMs = 10000;
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

void LogUiDiagnostics(uint32_t nowMs, uint32_t renderWorkMs) {
  static uint32_t s_lastDiagnosticsMs = 0;
  if (nowMs - s_lastDiagnosticsMs < kUiDiagnosticsIntervalMs) {
    return;
  }
  s_lastDiagnosticsMs = nowMs;

  const LvglPortPerfStats stats = LvglPort_GetPerfStats();
  const uint32_t handlerAvgUs =
      stats.handlerRunCount == 0
          ? 0
          : static_cast<uint32_t>(stats.handlerTotalUs / stats.handlerRunCount);
  const size_t internalFree =
      heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  const size_t internalLargest =
      heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  const size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const size_t psramLargest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  const UBaseType_t stackHighWaterWords = uxTaskGetStackHighWaterMark(nullptr);
  ESP_LOGI(
      TAG,
      "diag core=%d task=bike_ui heap_internal_free=%u heap_internal_largest=%u "
      "psram_free=%u psram_largest=%u stack_hwm_words=%u render_ms=%u "
      "handler_max_us=%u handler_avg_us=%u flush_count=%u flush_pixels=%llu",
      xPortGetCoreID(),
      static_cast<unsigned>(internalFree),
      static_cast<unsigned>(internalLargest),
      static_cast<unsigned>(psramFree),
      static_cast<unsigned>(psramLargest),
      static_cast<unsigned>(stackHighWaterWords),
      static_cast<unsigned>(renderWorkMs),
      static_cast<unsigned>(stats.handlerMaxUs),
      static_cast<unsigned>(handlerAvgUs),
      static_cast<unsigned>(stats.flushCount),
      static_cast<unsigned long long>(stats.flushPixelCount));
  LvglPort_ResetPerfStats();
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
    LogUiDiagnostics(BikePlatform_Millis(), renderWorkMs);
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
