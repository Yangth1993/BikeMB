# Bring-up Log

## 2026-07-05

### Goal

点亮 Waveshare ESP32-S3-Touch-LCD-1.85C V2 的 1.85 寸圆形屏幕，验证 PlatformIO、USB 串口、LCD、背光和基础显示链路。

### Environment

- Board: Waveshare ESP32-S3-Touch-LCD-1.85C V2
- Serial port: COM5
- Toolchain: PlatformIO Core
- Platform: pioarduino/platform-espressif32 `55.03.39`
- Arduino core: ESP32 Arduino `3.3.9`
- Framework: Arduino
- PlatformIO board: `esp32-s3-devkitc1-n16r8`

### Bring-up Project

- Path: `firmware/bringup`
- Display driver source: Waveshare official `Arduino/examples/01_lvgl_example`
- UI scope: no LVGL yet, only color blocks and serial logs

### Result

- PlatformIO Core installed successfully.
- `pio run` succeeded after switching from default `espressif32` to `pioarduino/platform-espressif32`.
- Default PlatformIO `espressif32` used Arduino core 2.0.17 and failed because Waveshare V2 driver uses Arduino/ESP-IDF 3.x LCD APIs.
- Final build succeeded with Arduino core 3.3.9.
- First upload using `esp32-s3-devkitc-1` booted with a PSRAM mode error because that board profile is 8MB Flash / no PSRAM.
- Switching to `esp32-s3-devkitc1-n16r8` fixed Flash/PSRAM detection: 16MB Flash Quad + 8MB PSRAM Octal.
- Upload to COM5 succeeded.
- Serial heartbeat confirmed the app is running:
  - `free heap=338384`
  - `psram=8383924`

### Expected Device Behavior

After upload:

- Serial monitor prints `[BikeMB] ESP32-S3 Touch LCD 1.85C bring-up`.
- Backlight turns on.
- LCD shows a dark background, green top bar, amber bottom bar, white center blocks, and cyan/white lower bars.
- Serial monitor prints an alive heartbeat every 2 seconds.

### Notes

- Use UTF-8 terminal output when running PlatformIO on Windows, for example:
  - PowerShell: `$env:PYTHONIOENCODING='utf-8'`
- Do not force DTR/RTS toggling while reading logs, because the ESP32-S3 can enter download mode.

### Realtime Dashboard Update

- Replaced the static color-block screen with a realtime dashboard.
- The screen now shows:
  - CPU render-load estimate
  - Internal heap memory usage
  - FPS
  - PSRAM usage
  - A moving 48 x 48 pixel image sprite
- The moving sprite redraws at a target frame interval of 33 ms.
- Serial verification after upload:
  - `cpu= 3%`
  - `fps=29.6`
  - `heap=338088/380928`
  - `psram=8383924/8388608`
