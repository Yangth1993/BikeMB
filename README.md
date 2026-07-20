# BikeMB

BikeMB 是一个基于 `ESP32-S3` 圆形屏开发板的自行车码表项目。

当前产品目标不是一次性做完整骑行座舱，而是先做一个可上板运行、圆屏可读、接口可替换、能反复验证的基础码表。核心显示能力必须独立于云端、手机 App 和复杂外设。

## 当前产品阶段

- Stage 0：硬件 bring-up 已完成，已验证串口、背光、屏幕和基础绘制链路。
- Stage 1：正式 `LVGL` UI demo 基线进行中，重点是稳定 `src/firmware/bikemb` 的圆屏 dashboard。
- Stage 2：基础可骑行 MVP 待推进，目标是把 demo 数据替换为可验证的真实或可替换数据模型。

当前优先级：保留 `src/firmware/bringup` 作为硬件验证基线，在 `src/firmware/bikemb` 中建立正式应用基座。

## 第一阶段 P0

第一阶段只交付基础码表能力：

- 当前速度作为主视觉元素。
- 单次里程。
- 骑行时间。
- 设备电量。
- 总里程持久化保存。
- 至少一个实体按键切换页面。
- 上电后 3 秒内进入可用主界面。

## 后续范围

P1 重点：

- 轮径、单位、亮度等基础设置。
- 正式 `LVGL` UI 基线。
- 可替换的数据接入层。

P2 只做默认关闭的实验能力：

- 音频自检。
- 本地模式语音提示。
- 语音页面切换。
- 云端 AI 语音助手和音乐播放探索。

AI、语音、云端和音乐能力不得影响 P0 码表可用性。仓库内不得提交密钥、账号、令牌或个人敏感信息。

## 明确非目标

当前阶段不做：

- 电机控制。
- 导航、轨迹记录、OTA、手机 App。
- 心率、踏频、功率等完整骑行生态。
- 常驻唤醒词。
- 完整骑行助理或设备助理。

## 当前硬件

- 开发板：Waveshare `ESP32-S3-Touch-LCD-1.85C V2`
- 主控：`ESP32-S3R8`
- 屏幕：`360 x 360` 圆形 LCD
- LCD 驱动：`ST77916`
- 触摸：`CST816`
- I2C：`GPIO10 / GPIO11`
- 电池 ADC：`GPIO8`
- 当前串口：`COM5`

更多硬件细节见 `docs/hardware-notes.md`。

## 目录结构

- `docs/product/`
  - 产品愿景、需求、路线图、用户故事和 AI 助手边界。
- `docs/architecture/`
  - 架构概览、接口、模块、状态机和 ADR。
- `docs/project/`
  - 当前计划、决策记录和问题清单。
- `openspec/`
  - 需求、设计和变更提案的长期管理入口。
- `src/firmware/bringup/`
  - 已验证的硬件 bring-up 工程。
- `src/firmware/bikemb/`
  - 正式 `LVGL` demo 和后续应用基座。
- `src/assets/`
  - 固件开发用 UI、图片、声音等资源。
- `src/build/`
  - 本地构建输出目录，不作为源码提交。
- `tools/`
  - 本地脚本、测试辅助和工程工具。
- `tools/simulator/`
  - 桌面模拟器相关工程。
- `refs_files/`
  - 参考资料目录，包含 `ESP32_Datas` 等外部资料。

## Sources of truth

- 产品愿景：`docs/product/vision.md`
- 产品需求：`docs/product/requirements.md`
- 产品路线图：`docs/product/roadmap.md`
- AI 助手边界：`docs/product/ai-assistant.md`
- 架构设计：`docs/architecture/overview.md`
- 接口定义：`docs/architecture/interfaces.md`
- 当前计划：`docs/project/current-plan.md`
- 已知问题：`docs/project/issues.md`
- OpenSpec 项目约束：`openspec/project.md`

## 推荐工作方式

1. 修改前先读相关产品文档和架构文档。
2. 涉及功能或行为变化时，先更新或创建 `openspec/` 变更。
3. 在 `src/firmware/bikemb` 中做最小可运行改动。
4. 固件改动先本地构建，再上板验证。
5. 每次只推进一个稳定的小目标，并同步 `docs/project/` 中的计划、决策或问题。

常用验证命令：

```powershell
powershell -ExecutionPolicy Bypass -File tools\run-tests.ps1 -SmokeBuild
```

## 参考入口

- `docs/project-context.md`
- `docs/bringup-log.md`
- `docs/software-architecture.md`
- `docs/ui-ux/README.md`
- `openspec/project.md`
