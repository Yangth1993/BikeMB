#pragma once

#include <stdbool.h>

#include "ai_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool BikeMbAiAssistant_Init(void);
void BikeMbAiAssistant_OnButtonPressed(void);
void BikeMbAiAssistant_OnButtonReleased(void);
void BikeMbAiAssistant_Cancel(void);
void BikeMbAiAssistant_SetWifiConnected(bool connected);
void BikeMbAiAssistant_GetSnapshot(BikeMbAiSnapshot *out);

#ifdef __cplusplus
}
#endif
