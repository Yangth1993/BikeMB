# BikeMB 项目上下文

这份文档用于把当前对话里已经确认的项目上下文沉淀到仓库中，方便后续继续开发时快速接上状态，而不必重新回忆聊天记录。

## 项目定位

BikeMB 是一个基于 Waveshare ESP32-S3-Touch-LCD-1.85C V2 的自行车码表项目。

当前阶段目标不是一次性完成完整码表，而是按小步快跑的方式推进：

- 先打通硬件 bring-up
- 再确定软件框架
- 最后逐步接入真实业务数据和交互

## 当前硬件结论

已确认开发板为：

- Waveshare ESP32-S3-Touch-LCD-1.85C V2
- 主控：ESP32-S3R8
- Flash：16 MB
- PSRAM：8 MB
- 屏幕：1.85 寸圆形 LCD，360 x 360
- LCD 驱动：ST77916
- Touch：CST816
- I2C：GPIO10 / GPIO11
- 电池 ADC：GPIO8
- 调试串口：当前 Windows 识别为 `COM5`

本地参考资料已经放在：

- `ESP32_Datas/`

该目录只作为离线资料，不纳入 Git 版本管理。

## 已完成工作

### 1. 项目初始化

已建立：

- `openspec/`
- `docs/`
- `firmware/bringup/`
- 本地离线 Git 仓库

### 2. OpenSpec 风格需求文档

已写入并保留：

- 项目目标和边界
- 第一阶段 MVP 定位
- 基础码表核心规格
- 初始任务清单

### 3. 硬件资料整理

已完成：

- 下载 Waveshare 官方资料到 `ESP32_Datas/`
- 整理硬件记录
- 确认屏幕、触摸、PSRAM、Flash、I2C、ADC 等关键参数

### 4. Bring-up 工程验证

已用 `firmware/bringup` 验证：

- PlatformIO Core 可用
- 串口烧录可用
- COM5 可通讯
- LCD 已成功点亮
- 背光可控
- PSRAM 配置已修正为正确板卡配置
- 串口日志输出可见

Bring-up 路线使用：

- PlatformIO
- `pioarduino/platform-espressif32#55.03.39`
- board：`esp32-s3-devkitc1-n16r8`
- Arduino core：3.3.9

### 5. 调试辅助工具

已加入本地串口查看工具：

- `tools/open-serial-monitor.bat`
- `tools/serial-monitor.ps1`

约定：

- 后续每次上板调试时默认打开串口日志窗口
- 若烧录时串口被占用，先关闭日志窗口，烧录后再重新打开

## 当前技术判断

通过 bring-up 阶段验证后，我们已经确认：

1. 手写像素刷屏只适合做最初的硬件验证，不适合正式 UI 开发。
2. 刷新闪烁和高负载主要来自粗粒度重绘策略，不是硬件本身异常。
3. 正式工程应转向 LVGL，而不是继续扩大手写渲染代码。

## 当前软件方向

当前已确认的软件框架方向是：

- 保留 `firmware/bringup` 作为硬件验证基线
- 新建 `firmware/bikemb` 作为正式应用工程
- 使用 LVGL 作为唯一 UI 绘制层
- 复用 Waveshare 官方 LCD / Touch / I2C / TCA9554 驱动
- 第一版正式 UI 先做系统状态页：
  - FPS
  - CPU 渲染负载估算
  - MEM 占用
  - PSRAM 占用

## Git 管理约定

仓库当前使用本地离线 Git。

已完成本地提交：

- `9d37acb` `Initialize BikeMB bring-up baseline`
- `96b4169` `Add local serial monitor tools`

约定：

- 每完成一个稳定小目标就做一次本地提交
- 不提交 `ESP32_Datas/`
- 不提交 `.pio/`

## 当前暂停点

在上一阶段中，我们开始规划正式 LVGL 工程，但你决定先暂停那部分，优先把项目上下文沉淀到仓库中。

也就是说，目前仓库的真实开发状态是：

- bring-up 工程存在并可作为硬件验证入口
- 正式 LVGL 工程方向已经明确
- 但 `firmware/bikemb` 还没有正式落地到项目目录

## 后续推荐顺序

建议下一步按这个顺序继续：

1. 先检查并整理现有文档编码，避免 README 和部分日志出现乱码
2. 再正式创建 `firmware/bikemb`
3. 用 LVGL 做第一个稳定状态页
4. 之后再接入触摸、电池、电量、轮速等真实数据

## 如何使用这份文档

以后如果我们中断一段时间，再回来继续开发，可以先读：

- `docs/project-context.md`
- `docs/hardware-notes.md`
- `docs/bringup-log.md`
- `openspec/project.md`

这样能最快恢复项目状态。

