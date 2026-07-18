# 当前计划

本文记录当前短期执行计划。正式功能或行为变更仍以 `openspec/changes/` 为准。

## 当前目标

- 稳定 `src/firmware/bikemb` 的 LVGL dashboard。
- 保持 `src/firmware/bringup` 作为硬件验证基线。
- 将文档沉淀到 `docs/`，将开发输入材料收敛到 `src/`。

## 下一步

1. 修正中文文档编码和内容一致性。
2. 继续验证 LVGL dashboard 的模拟器与固件构建链路。
3. 在 OpenSpec 中推进真实骑行数据接入前的设计。

## 验证入口

- 文档结构：确认 `docs/product/`、`docs/architecture/`、`docs/project/` 均存在。
- 固件构建：优先使用 `tools/run-tests.ps1` 或定向 PlatformIO 命令。
