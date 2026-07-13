#pragma once

#include <stdint.h>

typedef struct LvglPortPerfStats {
  uint32_t handlerRunCount;
  uint64_t handlerTotalUs;
  uint32_t handlerMaxUs;
  uint32_t flushCount;
  uint64_t flushPixelCount;
} LvglPortPerfStats;

void LvglPort_Init();
void LvglPort_Tick(unsigned long deltaMs);
uint32_t LvglPort_Run();
LvglPortPerfStats LvglPort_GetPerfStats();
void LvglPort_ResetPerfStats();
