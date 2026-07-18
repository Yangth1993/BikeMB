#pragma once

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "bike_event.h"

void BikeRuntime_Init();
void BikeRuntime_Start();
bool BikeRuntime_PostEvent(const BikeEvent *event, TickType_t timeoutTicks = 0);
QueueHandle_t BikeRuntime_GetEventQueue();
uint32_t BikeRuntime_GetDroppedLowPriorityEvents();
