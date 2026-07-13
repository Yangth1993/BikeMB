#pragma once

#include <stdint.h>
#include <stdio.h>

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
#include <Arduino.h>
#else
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

inline uint32_t BikePlatform_Millis() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  return millis();
#else
  return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
#endif
}

inline uint32_t BikePlatform_Micros() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  return micros();
#else
  return static_cast<uint32_t>(esp_timer_get_time());
#endif
}

inline void BikePlatform_DelayMs(uint32_t delayMs) {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  delay(delayMs);
#else
  vTaskDelay(pdMS_TO_TICKS(delayMs));
#endif
}

inline uint32_t BikePlatform_GetHeapFree() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  return ESP.getFreeHeap();
#else
  return static_cast<uint32_t>(heap_caps_get_free_size(MALLOC_CAP_8BIT));
#endif
}

inline uint32_t BikePlatform_GetHeapTotal() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  return ESP.getHeapSize();
#else
  return static_cast<uint32_t>(heap_caps_get_total_size(MALLOC_CAP_8BIT));
#endif
}

inline uint32_t BikePlatform_GetPsramFree() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  return ESP.getFreePsram();
#else
  return static_cast<uint32_t>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#endif
}

inline uint32_t BikePlatform_GetPsramTotal() {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  return ESP.getPsramSize();
#else
  return static_cast<uint32_t>(heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
#endif
}

inline void BikePlatform_LogInfo(const char *tag, const char *message) {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  (void)tag;
  Serial.println(message);
#else
  ESP_LOGI(tag, "%s", message);
#endif
}

inline void BikePlatform_SetOutputPin(uint8_t pin) {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  pinMode(pin, OUTPUT);
#else
  gpio_set_direction(static_cast<gpio_num_t>(pin), GPIO_MODE_OUTPUT);
#endif
}

inline void BikePlatform_LedcAttach(uint8_t pin, uint32_t frequency, uint8_t resolution, uint8_t channel) {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  (void)channel;
  ledcAttach(pin, frequency, resolution);
#else
  ledc_timer_config_t timerConfig = {};
  timerConfig.speed_mode = LEDC_LOW_SPEED_MODE;
  timerConfig.duty_resolution = static_cast<ledc_timer_bit_t>(resolution);
  timerConfig.timer_num = LEDC_TIMER_0;
  timerConfig.freq_hz = frequency;
  timerConfig.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&timerConfig);

  ledc_channel_config_t channelConfig = {};
  channelConfig.gpio_num = pin;
  channelConfig.speed_mode = LEDC_LOW_SPEED_MODE;
  channelConfig.channel = static_cast<ledc_channel_t>(channel);
  channelConfig.intr_type = LEDC_INTR_DISABLE;
  channelConfig.timer_sel = LEDC_TIMER_0;
  channelConfig.duty = 0;
  channelConfig.hpoint = 0;
  ledc_channel_config(&channelConfig);
#endif
}

inline void BikePlatform_LedcWrite(uint8_t pin, uint8_t channel, uint32_t duty) {
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
  (void)channel;
  ledcWrite(pin, duty);
#else
  ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(channel), duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(channel));
#endif
}
