#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef BIKE_MB_ENABLE_AI_ASSISTANT
#define BIKE_MB_ENABLE_AI_ASSISTANT 0
#endif

#ifndef BIKE_MB_AI_USE_MOCK_PROVIDERS
#define BIKE_MB_AI_USE_MOCK_PROVIDERS 0
#endif

namespace BikeMbAiConfig {
constexpr uint8_t kButtonGpio = 0;
constexpr bool kButtonActiveLow = true;
constexpr uint32_t kStartupGuardMs = 3000;
constexpr uint32_t kReleaseToArmMs = 50;
constexpr uint32_t kDebounceMs = 30;
constexpr uint32_t kMinRecordingMs = 300;
constexpr uint32_t kMaxRecordingMs = 10000;
constexpr uint32_t kCloudDeadlineMs = 15000;
constexpr uint32_t kErrorDisplayMs = 1500;
constexpr size_t kMaxSttTextBytes = 512;
constexpr size_t kMaxAnswerBytes = 1024;
constexpr uint32_t kAssistantStackBytes = 12 * 1024;
constexpr uint32_t kCloudWorkerStackBytes = 12 * 1024;
}
