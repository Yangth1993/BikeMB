#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BikeMbDashboardMetrics {
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
  uint8_t activePage;
  uint8_t wavePhase;
} BikeMbDashboardMetrics;

void BikeMbDashboardView_Create(void);
void BikeMbDashboardView_Update(const BikeMbDashboardMetrics *metrics);

#ifdef __cplusplus
}
#endif
