# ADR-0002: AI 助手 V1 采用设备直连的分阶段云端管线

- Status: Accepted
- Date: 2026-07-18
- Decision owners: 软件架构
- Requirement: `docs/product/ai-assistant.md`
- OpenSpec change: `openspec/changes/add-cloud-ai-speaker/`

## Context

BikeMB 需要在 ESP32-S3R8、16 MB Flash、8 MB PSRAM 和现有 Arduino + LVGL 固件上增加默认关闭的 AI 助手实验能力。V1 包含实体键按住说话、云端 STT、DeepSeek 问答、云端 TTS、语音回复和云音频流播放。实体键的板级实现由 ADR-0003 进一步固定为复用 BOOT/GPIO0。

当前 ES7210 麦克风、ES8311 喇叭和 I2S0 已分别在音频自检、档位提示和直接语音命令环境中验证，但这些模块各自创建 I2S owner，不能作为生产 AI 链路同时运行。云端能力也不得阻塞 LVGL 或成为核心码表依赖。

## Decision

1. 保留 PlatformIO + Arduino 默认固件路径，以 FreeRTOS task 隔离 AI 和音频工作；不为 V1 迁移整个应用到 ESP-IDF。
2. 语音问答采用分阶段设备直连：bounded PCM/WAV capture -> STT -> DeepSeek -> TTS -> PCM playback。V1 不使用自建中转服务。
3. 新建 `audio_session`，作为 I2S0、ES7210 和 ES8311 的唯一运行时 owner。V1 为半双工，不混音、不同时录音和播放。
4. AI control task、cloud worker、provider adapter、网络连接、音频会话和 UI snapshot 分层。后台 task 不访问 LVGL；阻塞 provider 不得阻塞取消或状态读取。
5. V1 默认 STT 使用阿里云百炼 `qwen3-asr-flash` Base64 WAV REST，TTS 使用 `cosyvoice-v3-flash` HTTP/SSE 16 kHz mono PCM，LLM 使用 DeepSeek。三个 provider 均保留编译期可替换接口。
6. V1 音乐只支持预设或私有配置中的 HTTPS MP3 直链。未来点歌通过 `MusicCatalogProvider` 把查询解析为 stream descriptor，复用现有 Stream Player。
7. AI 启用后 Wi-Fi 在后台保持连接，以满足松键后 8 秒内开始回复的目标；AI 关闭时该模块不启动 Wi-Fi。
8. 敏感配置来自 Git 忽略的本地编译配置。TLS 必须验证证书，日志必须脱敏。
9. 录音少于 300 ms 视为取消；忙态按下会使旧 request 失效并开始新录音；10 秒录音上限后的松键不得提交截断音频。

## Alternatives considered

### 全流式 WebSocket STT/TTS

优点是首字和首音频延迟更低，也更接近成熟语音助手。缺点是供应商协议耦合、连接恢复、音频帧同步和内存生命周期复杂。V1 先验证完整闭环，暂不选择。

### 采用 xiaozhi-esp32 协议栈

该项目在 GitHub 上有成熟的 ESP32-S3 流式 ASR + LLM + TTS、Opus 和状态机实现，可作为任务、音频和取消设计的参考。但其主要路径依赖服务端协议和 ESP-IDF，超出 BikeMB 不自建中转服务及保留 Arduino 基线的约束。

### 直接引入通用 ESP32 音频播放库

`ESP32-audioI2S`、`ESP8266Audio` 和 `arduino-audio-tools` 支持多种网络音频格式，适合作为实现参考。但它们通常拥有 I2S 生命周期，且当前候选均涉及 GPL-3.0 许可。V1 不在架构层指定其为默认依赖；MP3 decoder 需在实现 spike 中验证许可证、内存、Arduino 3.x 兼容性和自定义 PCM sink。

## Consequences

### Positive

- 不改变核心码表启动和 UI 运行模型。
- 云供应商变化局限在 adapter 内。
- 单一音频 owner 消除当前多个 `I2SClass(I2S_NUM_0)` 并存风险。
- 未来点歌不会迫使 AI 状态机或播放器重写。
- 分阶段流程容易通过 mock provider 和确定性状态测试验证。

### Negative

- 分阶段 STT/LLM/TTS 延迟高于全流式方案，8 秒目标需要真实网络验证。
- 最长 10 秒 PCM 录音约占 320 KiB PSRAM。
- 编译期 API key 可从固件镜像提取，只适合 V1 开发验证。
- MP3 decoder 仍需单独做技术与许可证选择。

## Revisit triggers

- 典型网络下松键到首音频无法稳定满足 8 秒目标。
- TLS、JSON、录音和解码叠加后超出架构资源预算。
- 产品进入正式点歌或连续多轮对话阶段。
- 产品化要求设备级密钥轮换、远程吊销或用户配网。
- Arduino 路径无法稳定承载音频和网络任务，需要评估 ESP-IDF/ESP-ADF。

## References

- https://github.com/78/xiaozhi-esp32
- https://github.com/schreibfaul1/ESP32-audioI2S
- https://github.com/espressif/esp-adf
- https://github.com/modelscope/FunASR
- https://github.com/FunAudioLLM/CosyVoice
- https://github.com/aliyun/alibabacloud-bailian-speech-demo
