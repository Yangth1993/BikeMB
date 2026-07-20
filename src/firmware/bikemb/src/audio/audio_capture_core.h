#pragma once

#include <stddef.h>
#include <stdint.h>

#define BIKE_MB_AUDIO_CAPTURE_SAMPLE_RATE_HZ 16000U
#define BIKE_MB_AUDIO_CAPTURE_MAX_MS 10000U
#define BIKE_MB_AUDIO_CAPTURE_MAX_BYTES (384U * 1024U)

#ifdef __cplusplus
extern "C" {
#endif

uint32_t BikeMbAudioCaptureCore_MaxSamplesForMs(uint32_t maxMs);
size_t BikeMbAudioCaptureCore_DownmixStereoToMono(
    const int16_t *stereoFrames, size_t frameCount, int16_t *monoSamples, size_t monoCapacity);

#ifdef __cplusplus
}
#endif
