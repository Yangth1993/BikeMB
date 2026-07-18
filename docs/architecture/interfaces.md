# 接口定义

本文记录当前模块之间应优先使用的公共接口。新增接口前应先确认调用方数量和真实边界，避免为单点调用创建抽象。

## Dashboard App

`DashboardApp_*` 是 UI 命令的主入口。

| 接口 | 调用方 | 说明 |
| --- | --- | --- |
| `DashboardApp_Init()` | `main.cpp`, `UiService` | 初始化 metrics 和 dashboard view |
| `DashboardApp_Tick(uint32_t nowMs)` | `main.cpp`, `UiService` | 推进 demo 数据和 UI 更新 |
| `DashboardApp_SetRenderWorkMs(uint32_t renderWorkMs)` | `main.cpp`, `UiService` | 回传 LVGL 渲染耗时 |
| `DashboardApp_NextPage()` | 触摸、串口、语音命令 | 切换到下一页 |
| `DashboardApp_PreviousPage()` | 触摸、串口、语音命令 | 切换到上一页 |
| `DashboardApp_SetModeChangedCallback(...)` | `main.cpp` | 注册档位变化回调 |

约束：外部输入不直接修改 LVGL 对象，统一通过 App 命令入口进入。

## Dashboard View

| 接口 | 说明 |
| --- | --- |
| `BikeMbDashboardView_Create()` | 创建 screen、页面容器和子控件 |
| `BikeMbDashboardView_Update(...)` | 更新 labels、波形和触摸手势 |
| `BikeMbDashboardView_NextPage()` | 显示下一页 |
| `BikeMbDashboardView_PreviousPage()` | 显示上一页 |
| `BikeMbDashboardView_SetModeChangedCallback(...)` | 传递档位变化回调 |

`BikeMbDashboardMetrics` 是 View 的输入数据结构。它应表达 UI 需要展示的值，不应携带硬件驱动细节。

## LVGL Port

| 接口 | 说明 |
| --- | --- |
| `LvglPort_Init()` | 初始化 LVGL display 和 touch 输入 |
| `LvglPort_Tick(uint32_t deltaMs)` | 推进 LVGL tick |
| `LvglPort_Run()` | 执行 `lv_timer_handler()` 并返回渲染耗时 |

约束：App 层只关心 `LvglPort_Run()` 的耗时反馈，不关心 display flush 的实现细节。

## 音频提示

| 接口 | 说明 |
| --- | --- |
| `BikeMbAudioPrompts_Init()` | 初始化音频播报测试路径 |
| `BikeMbAudioPrompts_PlayMode(BikeMbAudioPromptMode mode)` | 异步提交档位播报请求 |

约束：播放请求不能阻塞 UI 档位切换。连续切换时应以最新请求为准。

## 音频自检

| 接口 | 说明 |
| --- | --- |
| `BikeMbAudioSelfTest_Init()` | 初始化自检音频链路 |
| `BikeMbAudioSelfTest_Tick(uint32_t nowMs)` | 处理串口测试命令和麦克风 RMS 输出 |
| `BikeMbAudioSelfTest_PlayPageTone(bool nextPage)` | 翻页后播放提示音 |
| `BikeMbAudioSelfTest_ConsumeCommand()` | 消费串口模拟命令 |

## 语音命令

| 接口 | 说明 |
| --- | --- |
| `BikeMbVoiceCommands_Init()` | 初始化 ESP-SR direct command 测试路径 |
| `BikeMbVoiceCommands_ConsumeCommand()` | 消费待处理语音命令 |

约束：当前语音命令是测试路径，不作为默认骑行固件输入源。

## Runtime/Event

| 接口 | 说明 |
| --- | --- |
| `BikeRuntime_Init()` | 创建 event queue 并初始化板级支持 |
| `BikeRuntime_Start()` | 启动 UI service 和 runtime tick task |
| `BikeRuntime_PostEvent(...)` | 投递事件 |
| `BikeRuntime_GetEventQueue()` | 返回事件队列 |
| `BikeRuntime_GetDroppedLowPriorityEvents()` | 返回低优先级事件丢弃计数 |

约束：低优先级 tick 可以丢弃；控制类和状态变更类事件后续需要单独定义优先级策略。

## AI Assistant（计划接口）

```cpp
enum BikeMbAiState {
  BIKE_MB_AI_DISABLED,
  BIKE_MB_AI_IDLE,
  BIKE_MB_AI_RECORDING,
  BIKE_MB_AI_RECOGNIZING,
  BIKE_MB_AI_THINKING,
  BIKE_MB_AI_SYNTHESIZING,
  BIKE_MB_AI_SPEAKING,
  BIKE_MB_AI_CONNECTING_MUSIC,
  BIKE_MB_AI_MUSIC_PLAYING,
  BIKE_MB_AI_ERROR,
};

struct BikeMbAiSnapshot {
  BikeMbAiState state;
  uint32_t requestId;
  uint32_t stateSinceMs;
  bool wifiConnected;
  bool cancelAvailable;
  char detail[96];
};
```

| 接口 | 调用方 | 语义 |
| --- | --- | --- |
| `BikeMbAiAssistant_Init()` | `main.cpp` | 创建队列/task，读取非敏感配置，不阻塞等待 Wi-Fi |
| `BikeMbAiAssistant_OnButtonPressed()` | AI Button | 开始一次按住说话交互；正在播放音乐时先停止音乐 |
| `BikeMbAiAssistant_OnButtonReleased()` | AI Button | 结束录音并进入 STT；非 Recording 状态下忽略 |
| `BikeMbAiAssistant_Cancel()` | AI Button、AI 页面 | 取消当前录音、云请求、TTS 或音乐播放 |
| `BikeMbAiAssistant_PlayMusicUrl(const char *url)` | AI 页面、Music Service | 播放已验证的 HTTPS MP3 URL |
| `BikeMbAiAssistant_StopPlayback()` | AI 页面、AI Button | 停止 TTS 或音乐播放并释放音频会话 |
| `BikeMbAiAssistant_GetSnapshot(BikeMbAiSnapshot *out)` | Dashboard/View | 非阻塞复制当前只读状态 |

约束：命令接口只提交事件并立即返回。`detail` 只包含适合 UI 的短状态或脱敏错误，不包含 token、SSID、完整 URL、转写全文或回答全文。

按键语义：

- Idle 下按下开始录音；录音短于 `300 ms` 时松开视为取消，不提交 STT。
- Recording 下松开且录音有效时提交 STT。
- Recognizing、Thinking、Synthesizing、Speaking、ConnectingMusic 或 MusicPlaying 下再次按下，会使旧 request 失效，并在 Wi-Fi 可用时直接开始新的 Recording。
- AI 页面 Cancel 取消当前状态但不自动开始新录音。
- 达到 10 秒上限后保持 Error，直到按键松开；该次松开不得提交录音。

## Provider（计划接口）

provider 调用由 `bikemb_cloud` task 串行执行，`bikemb_ai` task 只处理带 request ID 的结果事件。接口应返回统一错误码，并接收取消上下文；不得把 HTTP 状态、供应商 request JSON 或 SDK 类型暴露给上层。

```cpp
struct BikeMbAiRequestContext {
  uint32_t requestId;
  uint32_t deadlineMs;
  bool (*isCancelled)(uint32_t requestId);
};

struct BikeMbAudioClip {
  const uint8_t *data;
  size_t size;
  uint32_t sampleRate;
  uint8_t channels;
  uint8_t bitsPerSample;
};

typedef bool (*BikeMbPcmSink)(const int16_t *samples,
                              size_t sampleCount,
                              uint32_t sampleRate,
                              void *context);
```

| 接口 | 输入 | 输出 |
| --- | --- | --- |
| `SttProvider_Transcribe(...)` | `BikeMbAudioClip` + request context | 最多 `512 bytes` UTF-8 文本 |
| `LlmProvider_Complete(...)` | 问题文本 + request context | 最多 `1024 bytes` UTF-8 短回答 |
| `TtsProvider_Synthesize(...)` | 回答文本 + PCM sink + request context | 分块 PCM；不要求完整音频常驻内存 |

默认 adapter 合约：

- STT：阿里云百炼 `qwen3-asr-flash` 短音频 REST。请求使用 JSON data URL 携带 Base64 WAV；Base64 应分块编码，避免复制完整录音。adapter 只返回最终文本。
- LLM：DeepSeek HTTPS chat completion。
- TTS：阿里云百炼 `cosyvoice-v3-flash` HTTP/SSE，明确请求 `16 kHz`、`16-bit`、mono PCM。adapter 按顺序解码 SSE 中的 Base64 音频块并调用 PCM sink。

provider 接口允许替换供应商，但 V1 不实现运行时动态插件系统；具体 adapter 在编译期选择。

`bikemb_cloud` 一次只执行一个 request。取消至少保证：AI 状态立即变化、音频立即停止、旧结果被丢弃；底层 HTTP/TLS 若无法线程安全地立即关闭，可在短读超时和 15 秒总 deadline 内退出，不得阻塞 UI 或新的本地码表操作。

## Audio Session（计划接口）

```cpp
enum BikeMbAudioOwner {
  BIKE_MB_AUDIO_OWNER_NONE,
  BIKE_MB_AUDIO_OWNER_AI_CAPTURE,
  BIKE_MB_AUDIO_OWNER_AI_SPEECH,
  BIKE_MB_AUDIO_OWNER_MUSIC,
  BIKE_MB_AUDIO_OWNER_PROMPT,
};
```

| 接口 | 说明 |
| --- | --- |
| `BikeMbAudioSession_Init()` | 唯一初始化 codec、I2S0、DMA 和 audio task |
| `BikeMbAudioSession_StartCapture(uint32_t requestId, uint32_t maxMs)` | 获取麦克风会话并开始写入 PSRAM |
| `BikeMbAudioSession_FinishCapture(uint32_t requestId, BikeMbAudioClip *out)` | 结束录音并返回只在当前请求有效的 clip 视图 |
| `BikeMbAudioSession_PlayPcm(...)` | 把 provider 分块 PCM 提交给 audio task |
| `BikeMbAudioSession_PlayMp3Stream(...)` | 播放已验证的 HTTPS MP3 stream descriptor |
| `BikeMbAudioSession_Stop(uint32_t requestId)` | 停止当前 owner，清空 ring buffer 并释放 clip |
| `BikeMbAudioSession_GetOwner()` | 返回当前音频 owner，用于诊断和测试 |

所有接口都必须校验 `requestId`。取消后旧 provider 不得继续向新的音频会话写入数据。

## Music Service（计划接口）

```cpp
struct BikeMbMusicStreamDescriptor {
  char url[256];
  char contentType[32];
  uint32_t maxBitrateKbps;
};
```

| 接口 | V1 行为 | 后续点歌行为 |
| --- | --- | --- |
| `BikeMbMusic_GetDefaultStream(...)` | 从私有配置读取预设 URL | 不变 |
| `BikeMbMusic_GetUserStream(...)` | 从私有配置读取用户 URL | 后续可接设置页持久化 |
| `MusicCatalogProvider_Resolve(query, out)` | V1 不实现 | 把歌曲名解析成有时效的 stream descriptor |

Stream Player 只消费 `BikeMbMusicStreamDescriptor`，不接收歌曲名，不调用 LLM，也不持有服务商 token。

## 配置边界

| 配置 | 来源 | 是否允许进入 Git |
| --- | --- | --- |
| feature enable、超时、录音上限 | 普通 tracked config | 是 |
| Wi-Fi SSID/password | `ai_secrets.local.h` | 否 |
| DeepSeek base URL/model/token | base URL/model 可 tracked，token 不可 tracked | 部分 |
| STT/TTS endpoint/model/token | endpoint/model 可 tracked，token 不可 tracked | 部分 |
| 用户音频流 URL | `ai_secrets.local.h`（V1） | 否 |

tracked 示例配置必须使用明显占位符。正式代码不得在串口输出上述敏感值。

板级 `AI_BUTTON_GPIO`、有效电平和上下拉配置不属于敏感信息，但必须在核对原理图并上板验证后才能加入 tracked board config。不得复用 BOOT、复位或电源控制引脚，除非另有硬件 ADR。
