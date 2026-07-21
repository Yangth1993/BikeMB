# ADR-0004: 正式 MusicService 前必须完成 ESP-IDF 双核迁移

- Status: Accepted
- Date: 2026-07-22
- Decision owners: 软件架构
- Related: `docs/product/ai-assistant.md`
- OpenSpec change: `openspec/changes/add-cloud-ai-speaker/`

## Context

AI 助手初版已经在 Arduino 路径完成录音、STT、LLM、TTS 和播放闭环，但 UI、AI control、网络和音频仍处在 Arduino 主循环与多个 FreeRTOS task 混合的运行模型中。正式音乐流会进一步引入持续 HTTPS 下载、MP3 解码、I2S DMA、暂停/取消和更长生命周期的资源所有权；点歌还会增加查询解析与请求切换。

如果在运行模型迁移前接入正式 MusicService，后续迁移会同时改变线程所有权、音频缓冲、取消语义和产品接口，增加重复实现与难以复现的跨核问题。

## Decision

完成 ESP-IDF 双核迁移是正式 `MusicService`、产品音乐流和点歌开发的强制前置条件。

门禁关闭前允许：

- 隔离的 HTTPS/MP3 decoder 技术 spike。
- host mock、接口草案、许可证和资源预算评估。
- 不进入产品运行路径的板级测量环境。

门禁关闭前禁止：

- 创建或接入正式 `MusicService` 生产模块。
- 从 AI Assistant 或 UI 发起实际音乐播放。
- 实现 `MusicCatalogProvider` 或点歌产品流程。

“完成 ESP-IDF 双核迁移”必须同时满足：

1. 产品入口使用 BikeMB `app_main()`，创建固定在 Core 0 的 `bike_runtime` 和固定在 Core 1 的 `bike_ui`。
2. `bike_ui` 是 LVGL 唯一 owner；其他 task 只通过 queue/event/snapshot 与 UI 通信。
3. AI control、Cloud/Wi-Fi、AudioSession 和语音链路都由 ESP-IDF runtime/service 启停，生产路径不依赖 Arduino `setup()` / `loop()`。
4. AudioSession 保持 I2S0、ES7210、ES8311 的唯一所有权，并定义跨 task 的 buffer 生命周期、取消和超时回收。
5. ESP-IDF 双核构建和相关合同测试通过；板级验证覆盖启动、触摸、显示、录音、云问答、TTS 和取消。
6. 记录 internal heap、largest block、PSRAM、各 task stack high-water mark、UI 处理延迟和 audio underrun 基线，并通过架构预算审查。

门禁由软件架构角色根据上述证据确认；不能只以“能够编译”或“两个 task 已创建”判定完成。

## Consequences

### Positive

- MusicService 从第一天就进入稳定的 task、事件和音频所有权模型。
- MP3 持续流、云请求和 LVGL 不会在迁移期间反复改变线程边界。
- 点歌只新增目录解析和业务编排，不需要再次重写播放器底层。

### Negative

- 正式音乐和点歌排期受 ESP-IDF 迁移进度约束。
- Arduino 路径上的技术 spike 不能直接视为产品实现，需要在迁移后重新集成验证。

## Supersedes

本 ADR 只替代 ADR-0002 中“可直接在当前 Arduino V1 路径继续实现正式音乐能力”的开发顺序；ADR-0002 的 provider、AudioSession、状态机和安全边界继续有效。
