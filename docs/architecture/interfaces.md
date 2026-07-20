# 接口定义

本文记录当前模块之间应优先使用的公共接口。新增接口前应先确认调用方数量和真实边界，避免为单点调用创建抽象。

## Dashboard App

`DashboardApp_*` 是 UI 命令的主入口。

| 接口 | 调用方 | 说明 |
| --- | --- | --- |
| `DashboardApp_Init()` | `main.cpp`, `UiService` | 初始化 metrics 和 dashboard view |
| `DashboardApp_Tick(uint32_t nowMs)` | `main.cpp`, `UiService` | 推进 demo 数据和 UI 更新 |
| `DashboardApp_SetRenderWorkMs(uint32_t renderWorkMs)` | `main.cpp`, `UiService` | 回传 LVGL 渲染耗时 |
| `DashboardApp_ShowAiPage()` | AI Button | 直接显示独立 AI 页面 |
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

## AI Assistant（当前接口）

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
| `BikeMbAiAssistant_OnButtonPressed()` | AI Button | 非阻塞投递按下事件；Wi-Fi 可用时开始或替换为新录音 |
| `BikeMbAiAssistant_OnButtonReleased()` | AI Button | 结束录音并进入 STT；非 Recording 状态下忽略 |
| `BikeMbAiAssistant_Cancel()` | 预留 UI/控制入口 | 使当前 request 失效并停止本地音频；AI 页面尚未绑定该命令 |
| `BikeMbAiAssistant_SetWifiConnected(bool)` | Wi-Fi Service | 非阻塞发布连接状态；断网时取消活跃请求 |
| `BikeMbAiAssistant_GetSnapshot(BikeMbAiSnapshot *out)` | Dashboard/View | 非阻塞复制当前只读状态 |

约束：命令接口只提交事件并立即返回。`detail` 只包含适合 UI 的短状态或脱敏错误，不包含 token、SSID、完整 URL、转写全文或回答全文。

按键语义：

- Idle 下按下开始录音；录音短于 `300 ms` 时松开视为取消，不提交 STT。
- Recording 下松开且录音有效时提交 STT。
- Recognizing、Thinking、Synthesizing 或 Speaking 下再次按下，会使旧 request 失效，并在 Wi-Fi 可用时直接开始新的 Recording。
- AI 页面 Cancel 取消当前状态但不自动开始新录音。
- 达到 10 秒上限后保持 Error，直到按键松开；该次松开不得提交录音。

## AI 状态机（当前接口）

| 接口 | 说明 |
| --- | --- |
| `BikeMbAiStateMachine_Init(machine, enabled, nowMs)` | 初始化纯状态机和 snapshot |
| `BikeMbAiStateMachine_Dispatch(machine, event)` | 处理事件并返回 `BikeMbAiEffect` bitmask |

状态机不执行副作用。`bikemb_ai` 根据 effect 调用 AudioSession 或 CloudWorker。云阶段共享 `60000 ms` deadline，Worker 结果必须携带匹配的 `requestId`。

## Cloud Worker（当前接口）

```cpp
struct BikeMbCloudJob {
  BikeMbCloudStage stage;
  uint32_t requestId;
  uint32_t deadlineMs;
};
```

| 接口 | 说明 |
| --- | --- |
| `BikeMbCloudWorker_Init(BikeMbCloudResultSink sink)` | 创建长度为 4 的 job 队列和 `bikemb_cloud` task |
| `BikeMbCloudWorker_Submit(const BikeMbCloudJob &job)` | 非阻塞提交 STT、LLM 或 TTS stage |
| `BikeMbCloudWorker_SetCaptureClip(requestId, BikeMbAudioClip *clip)` | 接管录音 clip 所有权，成功时清零调用方结构 |
| `BikeMbCloudWorker_CancelBefore(validRequestId)` | 失效旧 request 并释放未消费的 clip |

当前 Worker 串行调用 Qwen ASR、Qwen Chat 和 CosyVoice。取消阻止旧 callback 生效，但不能主动中断已经阻塞的 HTTPS 请求；单次网络读取超时为 `15000 ms`。

## Provider Adapter（当前接口）

| 接口 | 当前用途 |
| --- | --- |
| `BikeMbQwenAsr_WriteRequestJson(...)` | 分块生成 `qwen3-asr-flash` Base64 WAV JSON |
| `BikeMbQwenChat_WriteRequestJson(...)` | 生成当前运行时 `qwen-plus` 短回答请求 |
| `BikeMbQwenChat_CopyBoundedAnswer(...)` | 把回答限制在 `384 bytes` |
| `BikeMbCosyVoice_WriteRequestJson(...)` | 生成 `cosyvoice-v3-flash` SSE 请求 |
| `BikeMbCosyVoice_HandleSseLineWithStream(...)` | 跨 SSE 行连续解码 Base64 PCM |
| `BikeMbDeepSeek_WriteRequestJson(...)` | 已实现但当前 Worker 未调用的备用 adapter |

adapter 通过 sink 分块生成 JSON 或 PCM，避免在 adapter 内持有 transport。HTTP 状态读取、鉴权和 stage 串联仍由 CloudWorker 负责。

## Audio Session（当前接口）

```cpp
typedef struct BikeMbAudioClip {
  int16_t *samples;
  uint32_t sampleCount;
  uint32_t sampleRateHz;
  uint32_t capacitySamples;
  bool hitLimit;
} BikeMbAudioClip;
```

| 接口 | 说明 |
| --- | --- |
| `BikeMbAudioSession_Init()` | 初始化 ES8311、ES7210、I2S0 和 owner 状态；没有独立 audio task |
| `BikeMbAudioSession_Acquire(owner, requestId)` | 获取单一音频所有权 |
| `BikeMbAudioSession_Release(owner, requestId)` | 仅在 owner/request 匹配时释放 |
| `BikeMbAudioSession_ReleaseAll()` | 取消时强制释放当前 owner |
| `BikeMbAudioSession_WriteStereoPcm(...)` | 匹配 owner/request 后向 I2S 写 stereo PCM |
| `BikeMbAudioSession_ReadMicBytes(...)` | 匹配 owner/request 后从 I2S 读麦克风 bytes |
| `BikeMbAudioSession_StartCapture(requestId, maxMs)` | 获取 AI Capture owner，并在 PSRAM 分配有界 mono clip |
| `BikeMbAudioSession_PollCapture(requestId)` | 读取 stereo、downmix 为 mono 并写入 clip |
| `BikeMbAudioSession_FinishCapture(requestId, outClip)` | 停止录音并转移 clip 所有权 |
| `BikeMbAudioSession_ReleaseClip(clip)` | 释放 clip PSRAM |
| `BikeMbAudioSession_GetOwner/GetRequestId()` | 返回当前会话，用于校验和诊断 |

## Wi-Fi Service（当前接口）

| 接口 | 说明 |
| --- | --- |
| `BikeMbWifiService_Init()` | 创建异步 Wi-Fi task，不阻塞启动 |
| `BikeMbWifiServiceCore_Init(...)` | 初始化纯连接状态 |
| `BikeMbWifiServiceCore_Update(...)` | 产生连接、发布 connected/disconnected action |

Wi-Fi task 每秒轮询，断开时每 10 秒重试，只通过 `BikeMbAiAssistant_SetWifiConnected(...)` 发布状态。

## AI UI（当前接口）

| 接口 | 说明 |
| --- | --- |
| `DashboardApp_ShowAiPage()` | 显示独立 AI 页面；AI 按键稳定按下时调用 |
| `BikeMbAiUiState_FromSnapshot(...)` | 把业务 snapshot 映射为 UI-facing 状态和文案 |

后台任务不得访问 LVGL。当前 AI 页面只展示 snapshot；取消、重试和停止尚未绑定触摸命令。

## Music Service（计划接口）

HTTPS MP3、Stream Player、Music Service 和点歌 resolver 当前均未实现。不要在代码中调用先前文档出现过的 `BikeMbAiAssistant_PlayMusicUrl`、`BikeMbAudioSession_PlayMp3Stream` 等占位接口；新增音乐行为前应先更新 OpenSpec 和 ADR。

## 配置边界

| 配置 | 来源 | 是否允许进入 Git |
| --- | --- | --- |
| feature enable、超时、录音上限 | 普通 tracked config | 是 |
| Wi-Fi SSID/password | `ai_secrets.local.h` | 否 |
| Qwen Chat、DeepSeek base URL/model/token | base URL/model 可 tracked，token 不可 tracked | 部分 |
| STT/TTS endpoint/model/token | endpoint/model 可 tracked，token 不可 tracked | 部分 |
| 用户音频流 URL | `ai_secrets.local.h`（V1） | 否 |

tracked 示例配置必须使用明显占位符。正式代码不得在串口输出上述敏感值。

V1 的板级按键配置由 ADR-0003 固定为 `AI_BUTTON_GPIO=0`、低电平有效、板载 `10 kΩ` 上拉、`3000 ms` 启动保护和 `50 ms` 释放解锁。该配置允许进入 tracked board config。GPIO0 仍保留 ROM 下载模式 strap 语义，按住 BOOT 上电或复位会阻止正常应用启动。

详细函数调用关系和数据所有权见 `docs/architecture/ai-assistant-implementation.md`。
