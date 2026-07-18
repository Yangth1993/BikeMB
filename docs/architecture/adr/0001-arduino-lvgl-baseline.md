# ADR-0001: 使用 Arduino + LVGL 作为当前固件基线

## 状态

Accepted

## 背景

BikeMB 第一阶段目标是先建立可上板、可扩展、可持续迭代的基础码表。`src/firmware/bringup` 已验证串口、背光、屏幕、基础绘制链路；`src/firmware/bikemb` 当前承载正式 demo。

项目同时存在 ESP-IDF Runtime/Event/Service 骨架，但音频、语音和 dashboard 主要验证仍在 Arduino 路径。

## 决策

当前默认固件基线保持为 `PlatformIO + Arduino + LVGL`。

ESP-IDF 路径保留为迁移骨架，用于逐步验证任务、事件和服务边界，但不作为当前默认骑行 UI 固件路径。

## 后果

收益：

- 复用已经验证的上板路径，降低硬件 bring-up 后继续迭代的风险。
- LVGL 作为正式 UI 绘制层，避免继续扩展手写像素渲染 demo。
- 音频和语音实验可以继续通过独立 PlatformIO environment 验证。

限制：

- Arduino 主循环仍是当前默认调度模型，跨模块任务所有权需要谨慎控制。
- ESP-IDF Runtime 骨架与 Arduino 验证路径之间存在迁移差距。
- 语音输出和语音识别暂不作为同一默认固件环境同时启用。

后续触发条件：

- 当真实传感器、持久化、音频所有权或多任务调度复杂度超过 Arduino 主循环可控范围时，重新评估 ESP-IDF 迁移。
