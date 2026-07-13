#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void UiService_Start(QueueHandle_t eventQueue);
