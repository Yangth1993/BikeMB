#include "Touch_CST816.h"

#include "I2C_Driver.h"
#include "TCA9554PWR.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr uint8_t kCst816Address = 0x15;
constexpr uint8_t kGestureReg = 0x01;
constexpr uint8_t kVersionReg = 0x15;
constexpr uint8_t kChipIdReg = 0xA7;
constexpr uint8_t kDisableAutoSleepReg = 0xFE;
constexpr uint8_t kMaxTouchPoints = 1;

Cst816Gesture g_lastGesture = CST816_GESTURE_NONE;
bool g_gestureLatched = false;

}  // namespace

bool TouchCst816_Init() {
  Set_EXIO(EXIO_PIN1, Low);
  vTaskDelay(pdMS_TO_TICKS(10));
  Set_EXIO(EXIO_PIN1, High);
  vTaskDelay(pdMS_TO_TICKS(50));

  uint8_t version = 0;
  uint8_t ids[3] = {};
  const bool version_failed = I2C_Read(kCst816Address, kVersionReg, &version, 1);
  const bool id_failed = I2C_Read(kCst816Address, kChipIdReg, ids, 3);

  uint8_t disable_auto_sleep = 10;
  I2C_Write(kCst816Address, kDisableAutoSleepReg, &disable_auto_sleep, 1);

  return !version_failed && !id_failed;
}

bool TouchCst816_Read(Cst816TouchPoint *point) {
  if (point == nullptr) {
    return false;
  }

  uint8_t buf[6] = {};
  if (I2C_Read(kCst816Address, kGestureReg, buf, sizeof(buf))) {
    point->pressed = false;
    point->gesture = CST816_GESTURE_NONE;
    return false;
  }

  uint8_t touch_count = buf[1];
  if (touch_count > kMaxTouchPoints) {
    touch_count = kMaxTouchPoints;
  }

  point->gesture = static_cast<Cst816Gesture>(buf[0]);
  point->pressed = touch_count > 0;

  if (!point->pressed) {
    g_gestureLatched = false;
  }

  if (point->pressed && !g_gestureLatched && point->gesture != CST816_GESTURE_NONE) {
    g_lastGesture = point->gesture;
    g_gestureLatched = true;
  }

  if (point->pressed) {
    point->x = ((buf[2] & 0x0F) << 8) | buf[3];
    point->y = ((buf[4] & 0x0F) << 8) | buf[5];
  }

  return true;
}

Cst816Gesture TouchCst816_ConsumeGesture() {
  const Cst816Gesture gesture = g_lastGesture;
  g_lastGesture = CST816_GESTURE_NONE;
  return gesture;
}
