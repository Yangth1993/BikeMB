#include <cassert>
#include <cstring>

#include "../../src/firmware/bikemb/src/runtime/bike_runtime_plan.h"

int main() {
  assert(BIKE_RUNTIME_CORE_RUNTIME == 0);
  assert(BIKE_RUNTIME_CORE_UI == 1);
  assert(BikeRuntime_GetServicePlanCount() == 7);

  const BikeRuntimeServicePlan *runtime =
      BikeRuntime_FindServicePlan(BIKE_RUNTIME_SERVICE_RUNTIME_TICK);
  assert(runtime != nullptr);
  assert(std::strcmp(runtime->taskName, "bike_runtime") == 0);
  assert(runtime->core == BIKE_RUNTIME_CORE_RUNTIME);
  assert(runtime->autoStart);
  assert(!runtime->ownsLvgl);

  const BikeRuntimeServicePlan *ui =
      BikeRuntime_FindServicePlan(BIKE_RUNTIME_SERVICE_UI);
  assert(ui != nullptr);
  assert(ui->core == BIKE_RUNTIME_CORE_UI);
  assert(ui->autoStart);
  assert(ui->ownsLvgl);

  const BikeRuntimeServicePlan *assistant =
      BikeRuntime_FindServicePlan(BIKE_RUNTIME_SERVICE_AI_ASSISTANT);
  assert(assistant != nullptr);
  assert(assistant->core == BIKE_RUNTIME_CORE_RUNTIME);
  assert(assistant->autoStart);
  assert(!assistant->ownsLvgl);

  const BikeRuntimeServicePlan *unknown =
      BikeRuntime_FindServicePlan(static_cast<BikeRuntimeServiceId>(255));
  assert(unknown == nullptr);

  return 0;
}
