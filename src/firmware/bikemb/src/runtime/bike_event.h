#pragma once

#include <stdint.h>

enum class BikeEventType : uint8_t {
  SystemTick,
  DashboardTick,
  RenderStatsUpdate,
  ShowAiPage,
  DiagnosticRequest,
};

struct BikeEvent {
  BikeEventType type;
  uint32_t timestampMs;
  uint32_t value;
};
