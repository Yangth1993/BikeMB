#include "wifi_service.h"

#include <string.h>

#include "../ai/ai_assistant.h"
#include "../ai/ai_config.h"
#include "wifi_service_core.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
#include <Arduino.h>
#if BIKE_MB_ENABLE_AI_ASSISTANT
#include <WiFi.h>
#endif
#else
#include "esp_log.h"
#include "esp_timer.h"
#endif

#if __has_include("ai_secrets.local.h")
#include "ai_secrets.local.h"
#define BIKE_MB_AI_WIFI_HAS_LOCAL_SECRETS 1
#else
#define BIKE_MB_AI_WIFI_HAS_LOCAL_SECRETS 0
#endif

namespace {
constexpr uint32_t kPollIntervalMs = 1000;
constexpr uint32_t kWifiServiceStackBytes = 4096;
BikeMbWifiServiceCore s_core;
TaskHandle_t s_task = nullptr;

uint32_t nowMs() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  return millis();
#else
  return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
#endif
}

bool hasCredentials() {
#if BIKE_MB_AI_WIFI_HAS_LOCAL_SECRETS
  return strlen(BIKE_MB_AI_WIFI_SSID) > 0 &&
         strcmp(BIKE_MB_AI_WIFI_SSID, "CHANGE_ME_WIFI_SSID") != 0 &&
         strcmp(BIKE_MB_AI_WIFI_PASSWORD, "CHANGE_ME_WIFI_PASSWORD") != 0;
#else
  return false;
#endif
}

bool isConnected() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && \
    BIKE_MB_ENABLE_AI_ASSISTANT
  return WiFi.status() == WL_CONNECTED;
#else
  return false;
#endif
}

void startConnect() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && \
    BIKE_MB_AI_WIFI_HAS_LOCAL_SECRETS
  WiFi.mode(WIFI_STA);
  WiFi.begin(BIKE_MB_AI_WIFI_SSID, BIKE_MB_AI_WIFI_PASSWORD);
#endif
}

void applyActions(uint32_t actions) {
  if ((actions & BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_CONNECTED) != 0) {
    BikeMbAiAssistant_SetWifiConnected(true);
  }
  if ((actions & BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_DISCONNECTED) != 0) {
    BikeMbAiAssistant_SetWifiConnected(false);
  }
  if ((actions & BIKE_MB_WIFI_SERVICE_ACTION_START_CONNECT) != 0) {
    startConnect();
  }
}

void wifiTask(void *) {
  for (;;) {
    const uint32_t actions =
        BikeMbWifiServiceCore_Update(&s_core, nowMs(), isConnected());
    applyActions(actions);
    vTaskDelay(pdMS_TO_TICKS(kPollIntervalMs));
  }
}
}

bool BikeMbWifiService_Init(void) {
  BikeMbWifiServiceCore_Init(
      &s_core,
      BIKE_MB_ENABLE_AI_ASSISTANT != 0,
      hasCredentials(),
      nowMs());

#if !BIKE_MB_ENABLE_AI_ASSISTANT
  return true;
#else
  if (s_task != nullptr) {
    return true;
  }
  const BaseType_t created = xTaskCreate(
      wifiTask,
      "bikemb_wifi",
      kWifiServiceStackBytes,
      nullptr,
      1,
      &s_task);
  return created == pdPASS;
#endif
}
