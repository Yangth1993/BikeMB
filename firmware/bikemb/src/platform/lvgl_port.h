#pragma once

#include <stdint.h>

void LvglPort_Init();
void LvglPort_Tick(unsigned long deltaMs);
uint32_t LvglPort_Run();
