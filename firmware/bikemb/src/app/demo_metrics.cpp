#include "demo_metrics.h"

#include <algorithm>

#include "platform/bike_platform.h"

namespace {

constexpr int16_t kOrbSize = 48;
constexpr int16_t kOrbMinX = 26;
constexpr int16_t kOrbMaxX = 360 - kOrbSize - 26;
constexpr int16_t kOrbMinY = 198;
constexpr int16_t kOrbMaxY = 360 - kOrbSize - 62;

int16_t g_orbX = 52;
int16_t g_orbY = 206;
int8_t g_velocityX = 3;
int8_t g_velocityY = 2;
uint32_t g_frameCount = 0;
uint32_t g_lastFpsAt = 0;
float g_fps = 0.0f;
float g_cpuLoad = 0.0f;

void UpdateOrb() {
  g_orbX += g_velocityX;
  g_orbY += g_velocityY;

  if (g_orbX <= kOrbMinX || g_orbX >= kOrbMaxX) {
    g_velocityX = -g_velocityX;
    g_orbX += g_velocityX;
  }

  if (g_orbY <= kOrbMinY || g_orbY >= kOrbMaxY) {
    g_velocityY = -g_velocityY;
    g_orbY += g_velocityY;
  }
}

}  // namespace

void DemoMetrics_Init() {
  g_lastFpsAt = BikePlatform_Millis();
}

DemoMetrics DemoMetrics_Update(uint32_t elapsedMs, uint32_t targetFrameMs, uint32_t renderWorkMs) {
  (void)elapsedMs;

  ++g_frameCount;
  UpdateOrb();

  const uint32_t now = BikePlatform_Millis();
  if (now - g_lastFpsAt >= 1000) {
    g_fps = g_frameCount * 1000.0f / (now - g_lastFpsAt);
    g_frameCount = 0;
    g_lastFpsAt = now;
  }

  const float frameLoad =
      std::min(100.0f, renderWorkMs * 100.0f / static_cast<float>(std::max<uint32_t>(1, targetFrameMs)));
  g_cpuLoad = 0.85f * g_cpuLoad + 0.15f * frameLoad;

  DemoMetrics metrics{};
  metrics.fps = g_fps;
  metrics.cpuLoad = g_cpuLoad;
  metrics.heapFree = BikePlatform_GetHeapFree();
  metrics.heapTotal = BikePlatform_GetHeapTotal();
  metrics.psramFree = BikePlatform_GetPsramFree();
  metrics.psramTotal = BikePlatform_GetPsramTotal();
  metrics.orbX = g_orbX;
  metrics.orbY = g_orbY;
  metrics.uptimeMs = now;
  metrics.rideSeconds = now / 1000;
  metrics.speedKmh = 24.6f + ((now / 120) % 9) * 0.1f;
  metrics.tripKm = 12.4f + (now / 60000) * 0.1f;
  metrics.totalKm = 1247.0f + metrics.tripKm;
  metrics.averageSpeedKmh = 23.7f;
  metrics.assistPowerW = 180.0f + ((now / 180) % 40);
  metrics.temperatureC = 18.0f;
  metrics.gradePercent = 4.0f;
  metrics.batteryPercent = 78;
  metrics.activePage = (now / 5000) % 3;
  metrics.wavePhase = (now / 180) % 24;
  return metrics;
}
