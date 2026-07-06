# BikeMB

BikeMB 是一个基于 `ESP32-S3` 圆形屏开发板的自行车码表项目。

当前项目目标不是一次性做完完整产品，而是先建立一个稳定、可扩展、可反复上板验证的开发基线，再逐步接入真实骑行数据和更多功能。

## 当前状态

- `firmware/bringup`：硬件 bring-up 基线，已经验证串口、背光、屏幕和基础绘制链路
- `firmware/bikemb`：正式 demo 工程，目标是使用 `LVGL` 复刻当前 dashboard，并作为后续应用开发基座
- `openspec/`：使用 OpenSpec 管理需求、设计和变更
- `docs/`：项目上下文、硬件记录、bring-up 记录和开发辅助说明

## 当前硬件结论

- 开发板：Waveshare `ESP32-S3-Touch-LCD-1.85C V2`
- 主控：`ESP32-S3R8`
- 屏幕：`360 x 360` 圆形 LCD
- LCD 驱动：`ST77916`
- 触摸：`CST816`
- I2C：`GPIO10 / GPIO11`
- 电池 ADC：`GPIO8`
- 当前串口：`COM5`

更多硬件细节见 [hardware-notes.md](/D:/MyProject/BikeMB/docs/hardware-notes.md)。

## 仓库结构

- `docs/`
  - 项目上下文、硬件记录、bring-up 日志、串口说明、软件架构
- `openspec/`
  - `project.md`：项目目标、约束和协作规则
  - `specs/`：已确认的长期需求
  - `changes/`：变更提案、设计和任务清单
- `firmware/bringup/`
  - 已验证的硬件 bring-up 工程
- `firmware/bikemb/`
  - 正式 `LVGL` demo 工程
- `tools/`
  - 本地串口查看辅助脚本

## 第一阶段目标

第一阶段先做一个基础可骑行的圆屏码表，核心能力包括：

- 实时速度
- 单次里程
- 骑行时间
- 设备电量
- 总里程保存
- 至少一个实体按键切换页面

## 当前软件方向

- 保留 `firmware/bringup` 作为硬件验证基线
- 使用 `firmware/bikemb` 作为正式 demo 基线工程
- 使用 `LVGL` 作为正式 UI 绘制层
- 尽量复刻当前 bring-up dashboard 的观感和指标结构
- 后续在此基础上替换 demo 数据为真实传感器数据

## 推荐工作方式

1. 先更新 `openspec/` 文档，明确需求、设计和任务
2. 在 `firmware/bikemb` 中做最小可运行改动
3. 先完成本地构建，再上板验证
4. 每次只推进一个稳定的小目标
5. 及时更新 `docs/project-context.md`

## 参考入口

- [project-context.md](/D:/MyProject/BikeMB/docs/project-context.md)
- [bringup-log.md](/D:/MyProject/BikeMB/docs/bringup-log.md)
- [software-architecture.md](/D:/MyProject/BikeMB/docs/software-architecture.md)
- [project.md](/D:/MyProject/BikeMB/openspec/project.md)
