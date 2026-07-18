#include "board_support.h"

#include "../drivers/Display_ST77916.h"
#include "../drivers/I2C_Driver.h"
#include "../drivers/TCA9554PWR.h"

void BoardSupport_Init() {
  I2C_Init();
  TCA9554PWR_Init(0x00);
  Backlight_Init();
  Set_Backlight(80);
  LCD_Init();
}
