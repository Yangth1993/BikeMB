#pragma once

#include <stdint.h>

struct DemoMetrics {
  float fps;
  float cpuLoad;
  uint32_t heapFree;
  uint32_t heapTotal;
  uint32_t psramFree;
  uint32_t psramTotal;
  int16_t orbX;
  int16_t orbY;
  uint32_t uptimeMs;
  uint32_t rideSeconds;
  float speedKmh;
  float tripKm;
  float totalKm;
  float averageSpeedKmh;
  float assistPowerW;
  float temperatureC;
  float gradePercent;
  uint8_t batteryPercent;
  uint8_t wavePhase;
};

void DemoMetrics_Init();
DemoMetrics DemoMetrics_Update(uint32_t elapsedMs, uint32_t targetFrameMs, uint32_t renderWorkMs);
