#include "ai_button.h"

#include "../ai/ai_assistant.h"
#include "../ai/ai_config.h"
#include "../runtime/bike_event.h"
#include "../runtime/bike_runtime.h"
#include "ai_button_logic.h"

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
#include <Arduino.h>
#else
#include "driver/gpio.h"
#include "esp_log.h"
#endif

namespace {
constexpr const char *TAG = "BikeMbAiButton";
constexpr uint32_t kButtonDiagnosticsIntervalMs = 10000;
BikeMbAiButtonLogic s_logic;
bool s_rawInitialized = false;
bool s_lastRawPressed = false;
uint32_t s_lastDiagnosticsMs = 0;

bool readRawPressed() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  const bool rawPressed = digitalRead(BikeMbAiConfig::kButtonGpio) == LOW;
#else
  const bool rawPressed =
      gpio_get_level(static_cast<gpio_num_t>(BikeMbAiConfig::kButtonGpio)) == 0;
#endif
  return rawPressed;
}

void logButtonEvent(const char *message) {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  Serial.println(message);
#else
  ESP_LOGI(TAG, "%s", message);
#endif
}

void logButtonInit() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  Serial.print("ai button init gpio=");
  Serial.print(BikeMbAiConfig::kButtonGpio);
  Serial.print(" guard_ms=");
  Serial.print(BikeMbAiConfig::kStartupGuardMs);
  Serial.print(" release_to_arm_ms=");
  Serial.print(BikeMbAiConfig::kReleaseToArmMs);
  Serial.print(" debounce_ms=");
  Serial.println(BikeMbAiConfig::kDebounceMs);
#else
  ESP_LOGI(
      TAG,
      "ai button init gpio=%u guard_ms=%u release_to_arm_ms=%u debounce_ms=%u",
      static_cast<unsigned>(BikeMbAiConfig::kButtonGpio),
      static_cast<unsigned>(BikeMbAiConfig::kStartupGuardMs),
      static_cast<unsigned>(BikeMbAiConfig::kReleaseToArmMs),
      static_cast<unsigned>(BikeMbAiConfig::kDebounceMs));
#endif
}

void logButtonDiagnostics(uint32_t nowMs, bool rawPressed) {
  if (nowMs - s_lastDiagnosticsMs < kButtonDiagnosticsIntervalMs) {
    return;
  }
  s_lastDiagnosticsMs = nowMs;
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  Serial.print("ai button diag raw=");
  Serial.print(rawPressed ? 1 : 0);
  Serial.print(" armed=");
  Serial.print(s_logic.armed ? 1 : 0);
  Serial.print(" stable=");
  Serial.print(s_logic.stablePressed ? 1 : 0);
  Serial.print(" candidate=");
  Serial.println(s_logic.candidatePressed ? 1 : 0);
#else
  ESP_LOGI(
      TAG,
      "ai button diag raw=%d armed=%d stable=%d candidate=%d",
      rawPressed ? 1 : 0,
      s_logic.armed ? 1 : 0,
      s_logic.stablePressed ? 1 : 0,
      s_logic.candidatePressed ? 1 : 0);
#endif
}

void logRawButtonChange(bool rawPressed) {
  if (!s_rawInitialized) {
    s_rawInitialized = true;
    s_lastRawPressed = rawPressed;
    return;
  }
  if (rawPressed == s_lastRawPressed) {
    return;
  }
  s_lastRawPressed = rawPressed;
  logButtonEvent(rawPressed ? "ai button raw pressed"
                            : "ai button raw released");
}
}

void BikeMbAiButton_Init(void) {
  BikeMbAiButtonLogic_Init(&s_logic);
  s_rawInitialized = false;
  s_lastRawPressed = false;
  s_lastDiagnosticsMs = 0;
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  pinMode(BikeMbAiConfig::kButtonGpio, INPUT_PULLUP);
#else
  gpio_config_t config = {};
  config.pin_bit_mask = 1ULL << BikeMbAiConfig::kButtonGpio;
  config.mode = GPIO_MODE_INPUT;
  config.pull_up_en = GPIO_PULLUP_ENABLE;
  config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&config);
#endif
  logButtonInit();
}

void BikeMbAiButton_Tick(uint32_t nowMs) {
  const bool rawPressed = readRawPressed();
  logRawButtonChange(rawPressed);
  const BikeMbAiButtonEvent event =
      BikeMbAiButtonLogic_Update(&s_logic, nowMs, rawPressed);
  logButtonDiagnostics(nowMs, rawPressed);
  if (event == BIKE_MB_AI_BUTTON_EVENT_PRESSED) {
    logButtonEvent("ai button pressed");
    const BikeEvent showAiPage = {
        .type = BikeEventType::ShowAiPage,
        .timestampMs = nowMs,
        .value = 0,
    };
    BikeRuntime_PostEvent(&showAiPage, 0);
    BikeMbAiAssistant_OnButtonPressed();
  } else if (event == BIKE_MB_AI_BUTTON_EVENT_RELEASED) {
    logButtonEvent("ai button released");
    BikeMbAiAssistant_OnButtonReleased();
  }
}
