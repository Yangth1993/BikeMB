#include "wifi_service.h"

#include <string.h>

#include "../ai/ai_assistant.h"
#include "../ai/ai_config.h"
#include "../runtime/bike_runtime_plan.h"
#include "wifi_service_core.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
#include <Arduino.h>
#if BIKE_MB_ENABLE_AI_ASSISTANT
#include <WiFi.h>
#include <WiFiClient.h>
#endif
#else
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#endif

#if __has_include("ai_secrets.local.h")
#include "ai_secrets.local.h"
#define BIKE_MB_AI_WIFI_HAS_LOCAL_SECRETS 1
#else
#define BIKE_MB_AI_WIFI_HAS_LOCAL_SECRETS 0
#endif

namespace {
constexpr uint32_t kPollIntervalMs = 1000;
constexpr uint32_t kWifiServiceStackBytes = 6144;
constexpr const char *kBailianProbeHost = "dashscope.aliyuncs.com";
constexpr const char *TAG = "BikeMbWifi";
BikeMbWifiServiceCore s_core;
TaskHandle_t s_task = nullptr;

#if !defined(ARDUINO) || defined(BIKE_MB_USE_ESPIDF_RUNTIME)
bool s_idfWifiInitialized = false;
bool s_idfConnected = false;
esp_netif_t *s_idfStaNetif = nullptr;
#endif

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
#elif BIKE_MB_ENABLE_AI_ASSISTANT
  return s_idfConnected;
#else
  return false;
#endif
}

#if !defined(ARDUINO) || defined(BIKE_MB_USE_ESPIDF_RUNTIME)
void idfWifiEventHandler(void *, esp_event_base_t eventBase, int32_t eventId, void *eventData) {
  if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
    s_idfConnected = false;
    ESP_LOGI(TAG, "disconnected");
    return;
  }
  if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
    s_idfConnected = true;
    const ip_event_got_ip_t *event = static_cast<const ip_event_got_ip_t *>(eventData);
    ESP_LOGI(TAG, "connected ip=" IPSTR, IP2STR(&event->ip_info.ip));
  }
}

bool initIdfWifi() {
#if !BIKE_MB_ENABLE_AI_ASSISTANT
  return true;
#else
  if (s_idfWifiInitialized) {
    return true;
  }

  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK && err != ESP_ERR_NVS_NO_FREE_PAGES &&
      err != ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGE(TAG, "nvs init failed err=0x%x", static_cast<unsigned>(err));
    return false;
  }
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGW(TAG, "nvs unavailable err=0x%x", static_cast<unsigned>(err));
    return false;
  }

  err = esp_netif_init();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "netif init failed err=0x%x", static_cast<unsigned>(err));
    return false;
  }

  err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    ESP_LOGE(TAG, "event loop init failed err=0x%x", static_cast<unsigned>(err));
    return false;
  }

  if (s_idfStaNetif == nullptr) {
    s_idfStaNetif = esp_netif_create_default_wifi_sta();
    if (s_idfStaNetif == nullptr) {
      ESP_LOGE(TAG, "sta netif init failed");
      return false;
    }
  }

  wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
  err = esp_wifi_init(&initConfig);
  if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
    ESP_LOGE(TAG, "wifi init failed err=0x%x", static_cast<unsigned>(err));
    return false;
  }

  esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &idfWifiEventHandler, nullptr, nullptr);
  esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &idfWifiEventHandler, nullptr, nullptr);

  wifi_config_t wifiConfig = {};
  strncpy(
      reinterpret_cast<char *>(wifiConfig.sta.ssid),
      BIKE_MB_AI_WIFI_SSID,
      sizeof(wifiConfig.sta.ssid) - 1);
  strncpy(
      reinterpret_cast<char *>(wifiConfig.sta.password),
      BIKE_MB_AI_WIFI_PASSWORD,
      sizeof(wifiConfig.sta.password) - 1);
  wifiConfig.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
  err = esp_wifi_start();
  if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
    ESP_LOGE(TAG, "wifi start failed err=0x%x", static_cast<unsigned>(err));
    return false;
  }

  s_idfWifiInitialized = true;
  return true;
#endif
}
#endif

void startConnect() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && \
    BIKE_MB_AI_WIFI_HAS_LOCAL_SECRETS
  WiFi.mode(WIFI_STA);
  WiFi.begin(BIKE_MB_AI_WIFI_SSID, BIKE_MB_AI_WIFI_PASSWORD);
#elif BIKE_MB_ENABLE_AI_ASSISTANT && BIKE_MB_AI_WIFI_HAS_LOCAL_SECRETS
  if (initIdfWifi()) {
    ESP_LOGI(TAG, "connect start");
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_wifi_connect());
  }
#endif
}

void logWifiConnected() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && \
    BIKE_MB_ENABLE_AI_ASSISTANT
  Serial.print("[BikeMB][wifi] connected ip=");
  Serial.print(WiFi.localIP());
  Serial.print(" rssi=");
  Serial.println(WiFi.RSSI());

  IPAddress ip;
  const bool dnsOk = WiFi.hostByName(kBailianProbeHost, ip) == 1;
  Serial.print("[BikeMB][wifi] dns dashscope ok=");
  Serial.print(dnsOk ? 1 : 0);
  if (dnsOk) {
    Serial.print(" ip=");
    Serial.print(ip);
  }
  Serial.println();

  WiFiClient client;
  client.setTimeout(3000);
  const bool tcpOk = dnsOk && client.connect(ip, 443);
  Serial.print("[BikeMB][wifi] tcp dashscope:443 ok=");
  Serial.println(tcpOk ? 1 : 0);
  client.stop();
#elif BIKE_MB_ENABLE_AI_ASSISTANT
  ESP_LOGI(TAG, "online");
#endif
}

void logWifiDisconnected() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && \
    BIKE_MB_ENABLE_AI_ASSISTANT
  Serial.println("[BikeMB][wifi] disconnected");
#elif BIKE_MB_ENABLE_AI_ASSISTANT
  ESP_LOGI(TAG, "offline");
#endif
}

void applyActions(uint32_t actions) {
  if ((actions & BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_CONNECTED) != 0) {
    logWifiConnected();
    BikeMbAiAssistant_SetWifiConnected(true);
  }
  if ((actions & BIKE_MB_WIFI_SERVICE_ACTION_PUBLISH_DISCONNECTED) != 0) {
    logWifiDisconnected();
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
#if !defined(ARDUINO) || defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  if (BIKE_MB_ENABLE_AI_ASSISTANT && hasCredentials() && !initIdfWifi()) {
    ESP_LOGW(TAG, "idf wifi init failed; service will publish offline");
  }
#endif

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
  const BaseType_t created = xTaskCreatePinnedToCore(
      wifiTask,
      "bikemb_wifi",
      kWifiServiceStackBytes,
      nullptr,
      1,
      &s_task,
      BIKE_RUNTIME_CORE_RUNTIME);
  return created == pdPASS;
#endif
}
