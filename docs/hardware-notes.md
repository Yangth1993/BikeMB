# 硬件记录

用于记录 BikeMB 项目的实际硬件信息。当前资料来自 Waveshare 官方文档、V2 原理图和 Arduino 示例代码。

## 开发板

- 型号：Waveshare `ESP32-S3-Touch-LCD-1.85C`
- 版本：`V2 / Rev2.0`
- 主控：`ESP32-S3R8`
- CPU：Xtensa LX7 dual-core，最高 `240 MHz`
- Flash：`16 MB`
- PSRAM：`8 MB`
- USB：`USB Type-C`
- USB D- / D+：`GPIO19 / GPIO20`
- 本地资料目录：`D:\MyProject\BikeMB\ESP32_Datas`

## 屏幕

- 尺寸：`1.85"` round LCD
- 分辨率：`360 x 360`
- 驱动芯片：`ST77916`
- 接口：QSPI
- 官方示例主机：`SPI2_HOST`
- LCD TE：`GPIO18`
- LCD SCK：`GPIO40`
- LCD DATA0：`GPIO46`
- LCD DATA1：`GPIO45`
- LCD DATA2：`GPIO42`
- LCD DATA3：`GPIO41`
- LCD CS：`GPIO21`
- LCD RST：`TCA9554 EXIO2`
- 背光控制：`GPIO5`，PWM

## 触摸

- 芯片：`CST816`
- 接口：I2C
- 地址：`0x15`
- 最大触点：1
- INT：`GPIO4`
- RST：`TCA9554 EXIO1`

## I2C 与扩展 IO

- I2C SCL：`GPIO10`
- I2C SDA：`GPIO11`
- 推荐频率：`400 kHz`
- 扩展 IO 芯片：`TCA9554PWR`
- TCA9554 地址：`0x20`
- 已知 EXIO 占用：
  - `EXIO1`：Touch RST
  - `EXIO2`：LCD RST

注意：

- 板载 I2C 已连接内部芯片
- `GPIO10 / GPIO11` 不应当随意改作普通 GPIO

## 电池与供电

- USB 供电：`USB Type-C`
- 电池接口：`MX1.25 2PIN`
- 电池类型：`3.7V` 锂电池
- 电池电压采样：`GPIO8 ADC`
- 官方示例中 `analogReadMilliVolts(8) * 3` 作为电池毫伏值，说明分压约为 `1/3`

## 外部接口

- I2C：
  - `GND`
  - `3V3`
  - `GPIO10`
  - `GPIO11`
- UART：
  - `GND`
  - `3V3`
  - `TXD = GPIO43`
  - `RXD = GPIO44`

## 输入

- 板载按键：
  - `RESET`
  - `BOOT`
- 正式码表建议使用额外实体按键，不依赖 `BOOT/RESET`

## 第一版速度传感器建议

- 霍尔传感器 + 轮圈磁铁
- 信号引脚待后续选定
- 选型时不要占用 LCD、Touch、I2C、USB、UART 和电池 ADC 已使用的关键引脚

## 当前开发约定

- bring-up 与正式 demo 都采用 `VS Code + PlatformIO + Arduino`
- 当前串口默认 `COM5`
- 正式 UI 方向为 `LVGL`
