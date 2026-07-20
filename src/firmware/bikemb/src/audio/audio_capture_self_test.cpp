#include "audio_capture_self_test.h"

#ifndef BIKE_MB_ENABLE_AUDIO_CAPTURE_SELF_TEST
#define BIKE_MB_ENABLE_AUDIO_CAPTURE_SELF_TEST 0
#endif

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME) && BIKE_MB_ENABLE_AUDIO_CAPTURE_SELF_TEST

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "audio_capture_core.h"
#include "audio_session.h"

namespace {

constexpr uint32_t kCaptureRequestId = 1;
constexpr uint32_t kCaptureTaskStackBytes = 4096;
constexpr UBaseType_t kCaptureTaskPriority = 1;
TaskHandle_t g_captureTask = nullptr;

void captureTask(void *param) {
  (void)param;
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println("[BikeMB][audio] capture self-test start");
  if (!BikeMbAudioSession_StartCapture(kCaptureRequestId, BIKE_MB_AUDIO_CAPTURE_MAX_MS)) {
    Serial.println("[BikeMB][audio] capture self-test start failed");
    vTaskDelete(nullptr);
    return;
  }

  while (BikeMbAudioSession_PollCapture(kCaptureRequestId)) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  BikeMbAudioClip clip = {};
  if (!BikeMbAudioSession_FinishCapture(kCaptureRequestId, &clip)) {
    Serial.println("[BikeMB][audio] capture self-test finish failed");
    vTaskDelete(nullptr);
    return;
  }

  Serial.print("[BikeMB][audio] capture samples=");
  Serial.print(clip.sampleCount);
  Serial.print(" bytes=");
  Serial.print(clip.sampleCount * sizeof(int16_t));
  Serial.print(" hit-limit=");
  Serial.println(clip.hitLimit ? "yes" : "no");
  BikeMbAudioSession_ReleaseClip(&clip);
  vTaskDelete(nullptr);
}

}  // namespace

void BikeMbAudioCaptureSelfTest_Init(void) {
  if (xTaskCreate(
          captureTask,
          "bikemb-audio-capture",
          kCaptureTaskStackBytes,
          nullptr,
          kCaptureTaskPriority,
          &g_captureTask) != pdPASS) {
    g_captureTask = nullptr;
    Serial.println("[BikeMB][audio] capture self-test task create failed");
  }
}

#else

void BikeMbAudioCaptureSelfTest_Init(void) {}

#endif
