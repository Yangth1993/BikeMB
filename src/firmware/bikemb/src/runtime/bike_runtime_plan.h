#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum BikeRuntimeCore : int8_t {
  BIKE_RUNTIME_CORE_RUNTIME = 0,
  BIKE_RUNTIME_CORE_UI = 1,
};

enum BikeRuntimeServiceId : uint8_t {
  BIKE_RUNTIME_SERVICE_RUNTIME_TICK = 0,
  BIKE_RUNTIME_SERVICE_UI,
  BIKE_RUNTIME_SERVICE_AI_ASSISTANT,
  BIKE_RUNTIME_SERVICE_CLOUD_WORKER,
  BIKE_RUNTIME_SERVICE_WIFI,
  BIKE_RUNTIME_SERVICE_AI_BUTTON_POLL,
  BIKE_RUNTIME_SERVICE_AUDIO_SESSION,
};

struct BikeRuntimeServicePlan {
  BikeRuntimeServiceId id;
  const char *taskName;
  int8_t core;
  bool autoStart;
  bool ownsLvgl;
};

const BikeRuntimeServicePlan *BikeRuntime_GetServicePlans();
size_t BikeRuntime_GetServicePlanCount();
const BikeRuntimeServicePlan *BikeRuntime_FindServicePlan(BikeRuntimeServiceId id);
