#include "audio_capture_core.h"

uint32_t BikeMbAudioCaptureCore_MaxSamplesForMs(uint32_t maxMs) {
  const uint32_t boundedMs = maxMs > BIKE_MB_AUDIO_CAPTURE_MAX_MS ? BIKE_MB_AUDIO_CAPTURE_MAX_MS : maxMs;
  const uint32_t requestedSamples =
      (BIKE_MB_AUDIO_CAPTURE_SAMPLE_RATE_HZ * boundedMs) / 1000U;
  const uint32_t maxSamplesByBytes =
      BIKE_MB_AUDIO_CAPTURE_MAX_BYTES / sizeof(int16_t);
  return requestedSamples > maxSamplesByBytes ? maxSamplesByBytes : requestedSamples;
}

size_t BikeMbAudioCaptureCore_DownmixStereoToMono(
    const int16_t *stereoFrames, size_t frameCount, int16_t *monoSamples, size_t monoCapacity) {
  if (stereoFrames == nullptr || monoSamples == nullptr || frameCount == 0 || monoCapacity == 0) {
    return 0;
  }

  const size_t outputCount = frameCount < monoCapacity ? frameCount : monoCapacity;
  for (size_t i = 0; i < outputCount; ++i) {
    monoSamples[i] = stereoFrames[i * 2];
  }
  return outputCount;
}
