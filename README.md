# BikeMB

BikeMB 是一个面向 ESP32 + 1.8 寸圆形屏幕的自行车码表项目。

项目当前处于需求初始化阶段，采用 OpenSpec 风格管理需求、设计和变更：

- `openspec/project.md`: 项目背景、约束和协作规则
- `openspec/specs/`: 已确认的长期能力规格
- `openspec/changes/`: 每次准备实现前的变更提案、设计和任务清单
- `docs/`: 普通说明文档、硬件记录和开发笔记

第一阶段目标是先做一个基础可骑行的圆屏码表：

- 实时速度
- 单次里程
- 骑行时间
- 设备电量
- 总里程保存
- 一个实体按键切换页面

## 推荐工作方式

1. 先修改 `openspec/project.md` 和 `openspec/changes/init-bike-computer-mvp/` 下的需求。
2. 确认硬件型号、屏幕驱动、传感器方案。
3. 再让 Codex 根据已确认的 OpenSpec 文档初始化固件工程。

