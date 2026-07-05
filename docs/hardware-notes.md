# 硬件记录

用于记录 BikeMB 项目的实际硬件信息。当前资料基于 Waveshare ESP32-S3-Touch-LCD-1.85C V2 官方文档、V2 原理图和 Arduino 示例代码整理。

## 开发板

- 型号：Waveshare ESP32-S3-Touch-LCD-1.85C
- 版本：V2 / Rev2.0
- SKU：30683 ESP32-S3-Touch-LCD-1.85C
- 主控：ESP32-S3R8
- CPU：Xtensa 32-bit LX7 双核，最高 240 MHz
- 无线：2.4 GHz Wi-Fi 802.11 b/g/n，Bluetooth 5 LE
- Flash：16 MB
- PSRAM：8 MB
- SRAM / ROM：512 KB SRAM，384 KB ROM
- 天线：板载贴片陶瓷天线，支持通过拆焊电阻切换到 IPEX 外部天线
- USB：USB Type-C，使用 ESP32-S3 原生 USB
- USB 引脚：GPIO19 = USB DN，GPIO20 = USB DP
- 本地资料目录：`D:\MyProject\BikeMB\ESP32_Datas`

## 屏幕

- 尺寸：1.85 寸圆形 LCD
- 分辨率：360 x 360
- 色彩：262K 色，示例代码使用 16-bit color
- 驱动芯片：ST77916
- 接口：SPI/QSPI，官方示例使用 `SPI2_HOST`
- SPI 模式：0
- SPI 时钟：官方示例配置为 80 MHz
- LCD TE：GPIO18
- LCD SCK：GPIO40
- LCD DATA0：GPIO46
- LCD DATA1：GPIO45
- LCD DATA2：GPIO42
- LCD DATA3：GPIO41
- LCD CS：GPIO21
- LCD RST：TCA9554 扩展 IO 的 EXIO2
- 背光控制：GPIO5，PWM
- 背光 PWM：官方示例使用 20 kHz，10-bit resolution

## 触摸

- 触摸芯片：CST816
- 触摸接口：I2C
- I2C 地址：`0x15`
- 最大触点：1 点
- Touch INT：GPIO4
- Touch RST：TCA9554 扩展 IO 的 EXIO1
- 支持手势：上滑、下滑、左滑、右滑、单击、双击、长按

## I2C 与扩展 IO

- I2C SCL：GPIO10
- I2C SDA：GPIO11
- I2C 频率：官方触摸示例使用 400 kHz
- 重要限制：Waveshare 文档说明板载 I2C 已连接内部芯片，外接 I2C 设备可以使用，但 GPIO10/GPIO11 不应改作普通 GPIO
- GPIO 扩展芯片：TCA9554PWR
- TCA9554 I2C 地址：`0x20`
- 已知扩展 IO：
  - EXIO1：Touch RST
  - EXIO2：LCD RST

## 板载外设

- RTC：PCF85063
- 音频解码：ES8311
- 音频编码：ES7210
- 功放：NS4150B
- 麦克风：双模拟麦克风
- 存储：Micro SD 卡座
- 电源模块：MP1605GTF-Z，3.3V 输出能力最高 2A
- 充电管理：板载电池充电管理芯片
- 指示灯：电源指示灯、充电指示灯

## 外部接口

- I2C 接口：
  - GND
  - 3V3
  - SCL = GPIO10
  - SDA = GPIO11
- UART 接口：
  - GND
  - 3V3
  - TXD = GPIO43
  - RXD = GPIO44
- USB 接口：
  - 5V
  - GND
  - DN = GPIO19
  - DP = GPIO20
- 预留接口：1.27 mm 28PIN 排座，引出部分 GPIO

## 输入

- 板载按键：
  - RESET 按键
  - BOOT 按键
- 码表交互按键：待确认，需要后续选择外接 GPIO
- 建议：BikeMB 正式码表功能使用额外实体按键，不依赖 BOOT/RESET 作为日常操作按键

## 速度传感器

- 第一版建议：霍尔传感器 + 轮圈磁铁
- 传感器型号：待确认
- 信号引脚：待确认，需要从可用扩展 GPIO 中选择
- 是否需要上拉：待确认，常见霍尔模块可用内部上拉或模块自带上拉
- 注意：不要占用 LCD、Touch、I2C、USB、UART 调试和电池 ADC 已使用的关键引脚

## 供电

- USB 供电：USB Type-C
- 电池接口：MX1.25 2PIN 系统电池接口
- 电池类型：3.7V 锂电池，板载支持充放电
- 系统电池开关：板载
- 充电指示：连接系统电池时，充电常亮，充满熄灭；未接电池时状态可能不确定
- 电池电压采样：GPIO8 ADC
- ADC 示例：官方 `06_AnalogRead` 使用 12-bit ADC，`analogRead(8)` 和 `analogReadMilliVolts(8)`
- 电压换算：官方示例打印 `analogReadMilliVolts(8) * 3` 作为 BAT mV，表示硬件分压约为 1/3

## 开发资料

- 官方仓库本地副本：`D:\MyProject\BikeMB\ESP32_Datas\waveshare-ESP32-S3-Touch-LCD-1.85C`
- V2 原理图：`D:\MyProject\BikeMB\ESP32_Datas\ESP32-S3-Touch-LCD-1.85C_V2_schematic.pdf`
- 结构图纸：`D:\MyProject\BikeMB\ESP32_Datas\ESP32-S3-Touch-LCD-1.85C_dimension.zip`
- Arduino LVGL 示例：`Arduino\examples\01_lvgl_example`
- 电池 ADC 示例：`Arduino\examples\06_AnalogRead`
- 官方支持开发方式：Arduino IDE 和 ESP-IDF
- BikeMB 第一轮计划：VS Code + PlatformIO + Arduino framework，先做自建亮屏 bring-up

## 待确认

- 实物是否为基础版还是 BOX 版本
- 开发板是否已通过 USB 在 Windows 上识别
- PlatformIO 下采用的具体 board 配置
- 码表用外接按键的数量和引脚
- 轮速霍尔传感器型号、供电电压和信号引脚
- 是否接入系统锂电池，以及电池容量

