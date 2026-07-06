#include "demo_metrics.h"

#include <Arduino.h>

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
  g_lastFpsAt = millis();
}

DemoMetrics DemoMetrics_Update(uint32_t elapsedMs, uint32_t targetFrameMs) {
  ++g_frameCount;
  UpdateOrb();

  const uint32_t now = millis();
  if (now - g_lastFpsAt >= 1000) {
    g_fps = g_frameCount * 1000.0f / (now - g_lastFpsAt);
    g_frameCount = 0;
    g_lastFpsAt = now;
  }

  const float frameLoad = min(100.0f, elapsedMs * 100.0f / max<uint32_t>(1, targetFrameMs));
  g_cpuLoad = 0.85f * g_cpuLoad + 0.15f * frameLoad;

  DemoMetrics metrics{};
  metrics.fps = g_fps;
  metrics.cpuLoad = g_cpuLoad;
  metrics.heapFree = ESP.getFreeHeap();
  metrics.heapTotal = ESP.getHeapSize();
  metrics.psramFree = ESP.getFreePsram();
  metrics.psramTotal = ESP.getPsramSize();
  metrics.orbX = g_orbX;
  metrics.orbY = g_orbY;
  return metrics;
}
