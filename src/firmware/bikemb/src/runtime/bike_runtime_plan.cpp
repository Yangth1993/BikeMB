#include "bike_runtime_plan.h"

namespace {

constexpr BikeRuntimeServicePlan kPlans[] = {
    {
        BIKE_RUNTIME_SERVICE_RUNTIME_TICK,
        "bike_runtime",
        BIKE_RUNTIME_CORE_RUNTIME,
        true,
        false,
    },
    {
        BIKE_RUNTIME_SERVICE_UI,
        "bike_ui",
        BIKE_RUNTIME_CORE_UI,
        true,
        true,
    },
    {
        BIKE_RUNTIME_SERVICE_AI_ASSISTANT,
        "bikemb_ai",
        BIKE_RUNTIME_CORE_RUNTIME,
        true,
        false,
    },
    {
        BIKE_RUNTIME_SERVICE_CLOUD_WORKER,
        "bikemb_cloud",
        BIKE_RUNTIME_CORE_RUNTIME,
        true,
        false,
    },
    {
        BIKE_RUNTIME_SERVICE_WIFI,
        "bikemb_wifi",
        BIKE_RUNTIME_CORE_RUNTIME,
        true,
        false,
    },
    {
        BIKE_RUNTIME_SERVICE_AI_BUTTON_POLL,
        "ai_button_poll",
        BIKE_RUNTIME_CORE_RUNTIME,
        true,
        false,
    },
    {
        BIKE_RUNTIME_SERVICE_AUDIO_SESSION,
        "audio_session",
        BIKE_RUNTIME_CORE_RUNTIME,
        true,
        false,
    },
};

}  // namespace

const BikeRuntimeServicePlan *BikeRuntime_GetServicePlans() {
  return kPlans;
}

size_t BikeRuntime_GetServicePlanCount() {
  return sizeof(kPlans) / sizeof(kPlans[0]);
}

const BikeRuntimeServicePlan *BikeRuntime_FindServicePlan(BikeRuntimeServiceId id) {
  for (size_t i = 0; i < BikeRuntime_GetServicePlanCount(); ++i) {
    if (kPlans[i].id == id) {
      return &kPlans[i];
    }
  }
  return nullptr;
}
