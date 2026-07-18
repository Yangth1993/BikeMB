#include <assert.h>

#include "../../src/firmware/bikemb/src/input/ai_button_logic.h"

int main() {
  BikeMbAiButtonLogic logic = {};
  BikeMbAiButtonLogic_Init(&logic);

  assert(BikeMbAiButtonLogic_Update(&logic, 0, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 2999, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3000, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3050, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(!logic.armed);

  assert(BikeMbAiButtonLogic_Update(&logic, 3060, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3109, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3110, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(logic.armed);

  assert(BikeMbAiButtonLogic_Update(&logic, 3120, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3149, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3150, true) == BIKE_MB_AI_BUTTON_EVENT_PRESSED);
  assert(BikeMbAiButtonLogic_Update(&logic, 3151, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3152, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3182, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);

  assert(BikeMbAiButtonLogic_Update(&logic, 3200, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3230, false) == BIKE_MB_AI_BUTTON_EVENT_RELEASED);
  assert(BikeMbAiButtonLogic_Update(&logic, 3240, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  return 0;
}
