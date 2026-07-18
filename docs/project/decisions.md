# 项目决策

本文记录轻量项目级决策。架构级长期决策应进入 `docs/architecture/adr/`。

## 已确认

- `docs/` 只保存项目文档、产品文档、架构文档和项目管理记录。
- 开发输入材料放入 `src/`，例如 LVGL 源 UI 图片和音频源文件。
- 固件工程统一放入 `src/firmware/`，其中 `bringup` 保持为硬件验证基线，`bikemb` 保持为正式 demo 工程。

## 待确认

- 是否保留 `docs/assets/` 作为文档截图和设计参考图目录。
