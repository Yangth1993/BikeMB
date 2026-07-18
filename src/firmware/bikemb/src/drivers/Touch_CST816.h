#pragma once

#include <stdint.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif

typedef enum Cst816Gesture {
  CST816_GESTURE_NONE = 0x00,
  CST816_GESTURE_SWIPE_UP = 0x01,
  CST816_GESTURE_SWIPE_DOWN = 0x02,
  CST816_GESTURE_SWIPE_LEFT = 0x03,
  CST816_GESTURE_SWIPE_RIGHT = 0x04,
  CST816_GESTURE_SINGLE_CLICK = 0x05,
  CST816_GESTURE_DOUBLE_CLICK = 0x0B,
  CST816_GESTURE_LONG_PRESS = 0x0C,
} Cst816Gesture;

typedef struct Cst816TouchPoint {
  bool pressed;
  uint16_t x;
  uint16_t y;
  Cst816Gesture gesture;
} Cst816TouchPoint;

#ifdef __cplusplus
extern "C" {
#endif

bool TouchCst816_Init();
bool TouchCst816_Read(Cst816TouchPoint *point);
Cst816Gesture TouchCst816_ConsumeGesture();

#ifdef __cplusplus
}
#endif
