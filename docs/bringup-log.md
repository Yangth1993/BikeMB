# Bring-up Log

## 2026-07-05

### 目标

点亮 Waveshare `ESP32-S3-Touch-LCD-1.85C V2` 的 `1.85"` 圆形屏幕，验证：

- PlatformIO
- USB 串口
- LCD
- 背光
- 基础显示链路

### 环境

- Board：Waveshare `ESP32-S3-Touch-LCD-1.85C V2`
- Serial port：`COM5`
- Platform：`pioarduino/platform-espressif32#55.03.39`
- Arduino core：`3.3.9`
- Framework：Arduino
- PlatformIO board：`esp32-s3-devkitc1-n16r8`

### Bring-up 工程

- 路径：`firmware/bringup`
- 显示驱动来源：Waveshare 官方 `Arduino/examples/01_lvgl_example`
- 当前 UI：未接入 `LVGL`，使用手写 dashboard 做硬件验证

### 结果

- PlatformIO Core 可用
- `pio run` 可成功完成
- `COM5` 上传成功
- LCD 点亮成功
- 背光可控
- 串口日志正常
- PSRAM 识别正确

### 已解决问题

- 默认 `espressif32` 平台使用的 Arduino core 过旧，不适配当前屏驱代码
- 改用 `pioarduino/platform-espressif32#55.03.39` 后构建通过
- `esp32-s3-devkitc-1` 板卡配置会导致 PSRAM 模式错误
- 切换到 `esp32-s3-devkitc1-n16r8` 后 Flash / PSRAM 识别正常

### 当前 dashboard 行为

上板后：

- 串口输出 `[BikeMB] realtime dashboard bring-up`
- 背光点亮
- 屏幕显示实时 dashboard
- 串口每秒输出一次性能信息

当前 dashboard 展示：

- `CPU`
- `FPS`
- `heap`
- `psram`
- 一个移动中的 sprite

### 技术结论

- 硬件显示链路已经打通
- 手写像素绘制足以验证硬件，但不适合正式应用开发
- 正式工程应切换到 `LVGL`

### 下一步

1. 修复项目文档编码并统一内容
2. 新建 `firmware/bikemb`
3. 使用 `LVGL` 复刻当前 dashboard
