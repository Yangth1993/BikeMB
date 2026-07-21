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
#endif

namespace {
BikeMbAiButtonLogic s_logic;

bool readRawPressed() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  const bool rawPressed = digitalRead(BikeMbAiConfig::kButtonGpio) == LOW;
#else
  const bool rawPressed =
      gpio_get_level(static_cast<gpio_num_t>(BikeMbAiConfig::kButtonGpio)) == 0;
#endif
  return rawPressed;
}
}

void BikeMbAiButton_Init(void) {
  BikeMbAiButtonLogic_Init(&s_logic);
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
}

void BikeMbAiButton_Tick(uint32_t nowMs) {
  const BikeMbAiButtonEvent event =
      BikeMbAiButtonLogic_Update(&s_logic, nowMs, readRawPressed());
  if (event == BIKE_MB_AI_BUTTON_EVENT_PRESSED) {
    const BikeEvent showAiPage = {
        .type = BikeEventType::ShowAiPage,
        .timestampMs = nowMs,
        .value = 0,
    };
    BikeRuntime_PostEvent(&showAiPage, 0);
    BikeMbAiAssistant_OnButtonPressed();
  } else if (event == BIKE_MB_AI_BUTTON_EVENT_RELEASED) {
    BikeMbAiAssistant_OnButtonReleased();
  }
}
