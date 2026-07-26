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
constexpr uint32_t kCloudDeadlineMs = 60000;
constexpr uint32_t kErrorDisplayMs = 1500;
constexpr size_t kMaxSttTextBytes = 512;
constexpr size_t kMaxAnswerBytes = 192;
constexpr uint32_t kAssistantStackBytes = 12 * 1024;
constexpr uint32_t kCloudWorkerStackBytes = 12 * 1024;
constexpr const char *kQwenAsrEndpoint = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";
constexpr const char *kQwenAsrModel = "qwen3-asr-flash";
constexpr const char *kQwenChatEndpoint = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";
constexpr const char *kQwenChatModel = "qwen-plus";
constexpr const char *kDeepSeekEndpoint = "https://api.deepseek.com/chat/completions";
constexpr const char *kDeepSeekModel = "deepseek-chat";
constexpr const char *kCosyVoiceTtsEndpoint = "https://dashscope.aliyuncs.com/api/v1/services/audio/tts/SpeechSynthesizer";
constexpr const char *kCosyVoiceTtsModel = "cosyvoice-v3-flash";
}
