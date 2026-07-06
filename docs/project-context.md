# BikeMB 项目上下文

这份文档用于沉淀当前仓库的真实开发状态，方便中断后快速恢复上下文。

## 项目定位

BikeMB 是一个基于 Waveshare `ESP32-S3-Touch-LCD-1.85C V2` 的自行车码表项目。

当前策略是按小步快跑推进：

1. 先打通硬件 bring-up
2. 再建立正式 `LVGL` demo 工程
3. 最后逐步接入真实骑行数据与交互

## 当前确认的硬件信息

- Board：Waveshare `ESP32-S3-Touch-LCD-1.85C V2`
- MCU：`ESP32-S3R8`
- Flash：`16 MB`
- PSRAM：`8 MB`
- Display：`360 x 360` round LCD
- LCD driver：`ST77916`
- Touch：`CST816`
- I2C：`GPIO10 / GPIO11`
- Battery ADC：`GPIO8`
- Current serial port on Windows：`COM5`

本地参考资料目录：

- `ESP32_Datas/`

该目录只作为离线资料，不纳入 Git 版本管理。

## 已完成事项

### 1. 仓库基础结构

已建立：

- `docs/`
- `openspec/`
- `firmware/bringup/`
- `tools/`

### 2. OpenSpec 文档基础

已经建立：

- 项目目标和边界
- 核心能力规格
- 初始变更提案
- 任务清单

### 3. 硬件资料整理

已确认或记录：

- 开发板型号
- 屏幕分辨率和驱动芯片
- 触摸芯片
- I2C 和电池 ADC 引脚
- bring-up 所需串口信息

### 4. Bring-up 工程验证

`firmware/bringup` 已验证：

- PlatformIO Core 可用
- 串口烧录可用
- `COM5` 可通信
- LCD 点亮成功
- 背光可控
- PSRAM 配置已修正为正确板卡配置
- 实时 dashboard 可运行

Bring-up 使用的关键配置：

- PlatformIO
- `pioarduino/platform-espressif32#55.03.39`
- board：`esp32-s3-devkitc1-n16r8`
- Arduino core：`3.3.9`

### 5. 调试辅助工具

仓库中已有：

- `tools/open-serial-monitor.bat`
- `tools/serial-monitor.ps1`

约定：

- 板级调试优先保留串口日志
- 串口占用时先关闭监视器，再执行烧录

## 当前技术判断

经过 bring-up 阶段验证，已经形成以下结论：

1. 手写像素渲染适合硬件验证，不适合作为正式 UI 方案
2. 当前刷新和动效已经足以说明硬件链路可用
3. 正式工程应切换到 `LVGL`
4. 正式工程应保留清晰的层次边界，避免再次演变成“只适合验证”的一次性 demo

## 当前软件方向

当前正式方向是：

- 保留 `firmware/bringup` 作为硬件验证入口
- 新建 `firmware/bikemb` 作为正式 demo 基线工程
- 复用已验证的底层 LCD / I2C / EXIO / 背光驱动
- 在 `bikemb` 中接入 `LVGL`
- 第一版页面尽量复刻当前 dashboard 的观感和信息结构

第一版 `bikemb` demo 目标：

- `CPU`
- `FPS`
- `MEM`
- `PSRAM`
- 一个简单动效区域

## 下一阶段目标

下一阶段按这个顺序推进：

1. 修复仓库中文文档编码并统一内容
2. 初始化 `firmware/bikemb`
3. 跑通 `LVGL` 显示链路
4. 复刻当前 dashboard 页面
5. 再逐步接入真实传感器与业务数据

## 如何快速恢复项目状态

中断后建议先读：

- [README.md](/D:/MyProject/BikeMB/README.md)
- [project-context.md](/D:/MyProject/BikeMB/docs/project-context.md)
- [bringup-log.md](/D:/MyProject/BikeMB/docs/bringup-log.md)
- [software-architecture.md](/D:/MyProject/BikeMB/docs/software-architecture.md)
- [project.md](/D:/MyProject/BikeMB/openspec/project.md)
