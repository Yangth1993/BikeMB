#include "I2C_Driver.h"

#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)

void I2C_Init(void) {
  Wire.begin( I2C_SDA_PIN, I2C_SCL_PIN);                       
}


bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr); 
  if ( Wire.endTransmission(true)){
    printf("The I2C transmission fails. - I2C Read\r\n");
    return -1;
  }
  Wire.requestFrom(Driver_addr, Length);
  for (int i = 0; i < Length; i++) {
    *Reg_data++ = Wire.read();
  }
  return 0;
}

bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr);       
  for (int i = 0; i < Length; i++) {
    Wire.write(*Reg_data++);
  }
  if ( Wire.endTransmission(true))
  {
    printf("The I2C transmission fails. - I2C Write\r\n");
    return -1;
  }
  return 0;
}
#else
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"

namespace {

constexpr i2c_port_t kI2cPort = I2C_NUM_0;
constexpr uint32_t kI2cClockHz = 400000;
constexpr TickType_t kI2cTimeout = pdMS_TO_TICKS(100);
constexpr size_t kMaxWriteBytes = 64;

}  // namespace

void I2C_Init(void) {
  i2c_config_t config = {};
  config.mode = I2C_MODE_MASTER;
  config.sda_io_num = I2C_SDA_PIN;
  config.scl_io_num = I2C_SCL_PIN;
  config.sda_pullup_en = GPIO_PULLUP_ENABLE;
  config.scl_pullup_en = GPIO_PULLUP_ENABLE;
  config.master.clk_speed = kI2cClockHz;

  i2c_param_config(kI2cPort, &config);
  i2c_driver_install(kI2cPort, config.mode, 0, 0, 0);
}

bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length) {
  const esp_err_t ret = i2c_master_write_read_device(
      kI2cPort, Driver_addr, &Reg_addr, 1, Reg_data, Length, kI2cTimeout);
  if (ret != ESP_OK) {
    printf("The I2C transmission fails. - I2C Read\r\n");
    return true;
  }
  return false;
}

bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length) {
  if (Length + 1 > kMaxWriteBytes) {
    printf("The I2C write is too large.\r\n");
    return true;
  }

  uint8_t buffer[kMaxWriteBytes] = {};
  buffer[0] = Reg_addr;
  memcpy(&buffer[1], Reg_data, Length);

  const esp_err_t ret = i2c_master_write_to_device(kI2cPort, Driver_addr, buffer, Length + 1, kI2cTimeout);
  if (ret != ESP_OK) {
    printf("The I2C transmission fails. - I2C Write\r\n");
    return true;
  }
  return false;
}
#endif
