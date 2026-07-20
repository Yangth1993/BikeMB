#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "../audio/audio_session.h"

enum BikeMbCloudStage {
  BIKE_MB_CLOUD_STAGE_STT = 0,
  BIKE_MB_CLOUD_STAGE_LLM,
  BIKE_MB_CLOUD_STAGE_TTS,
};

struct BikeMbCloudJob {
  BikeMbCloudStage stage;
  uint32_t requestId;
  uint32_t deadlineMs;
};

typedef void (*BikeMbCloudResultSink)(
    BikeMbCloudStage stage,
    uint32_t requestId,
    bool success,
    const char *detail);

bool BikeMbCloudWorker_Init(BikeMbCloudResultSink sink);
bool BikeMbCloudWorker_Submit(const BikeMbCloudJob &job);
bool BikeMbCloudWorker_SetCaptureClip(uint32_t requestId, BikeMbAudioClip *clip);
void BikeMbCloudWorker_CancelBefore(uint32_t validRequestId);
