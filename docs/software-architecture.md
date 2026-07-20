# BikeMB 固件架构说明

本文是 `src/firmware/bikemb` 的当前代码地图。目标不是解释概念，而是让你能快速找到“某个功能在哪个文件、哪个函数里实现”。系统级约束和目标架构分别维护在 `docs/architecture/`；AI 助手的逐函数说明见 [ai-assistant-implementation.md](/D:/MyProject/BikeMB/docs/architecture/ai-assistant-implementation.md)。本文只把已经进入源码的能力标为“当前实现”，尚未落地的能力明确标为“计划中”。

## 当前运行路径

当前工程保留两条运行路径：

- 默认路径：`PlatformIO + Arduino`，入口在 [main.cpp](/D:/MyProject/BikeMB/src/firmware/bikemb/src/main.cpp)，使用 `setup()` / `loop()`。
- 迁移路径：`PlatformIO + ESP-IDF`，入口也在 [main.cpp](/D:/MyProject/BikeMB/src/firmware/bikemb/src/main.cpp)，使用 `app_main()`。

目前 UI、触摸、音频自检、档位播报、直接语音识别和 AI 助手都以 Arduino 路径为主。AI 初版已经包含 BOOT 键、纯状态机、控制 task、异步 Wi-Fi、AudioSession 录音、Qwen ASR、Qwen Chat、CosyVoice TTS、喇叭播放和独立 AI 页面。默认固件仍关闭 AI；真实云链路只在专用测试环境启用。DeepSeek adapter 已实现但未接入 CloudWorker，音乐流和点歌仍是计划能力。ESP-IDF 路径已有 Runtime/Event/Service 骨架，但 AI、音频和语音没有接入该运行路径。

## 总体模块图

```mermaid
flowchart TB
  MAIN["main.cpp<br/>setup/loop 或 app_main"]

  subgraph UI["UI / App"]
    DASHAPP["dashboard_app.cpp/h<br/>DashboardApp_*"]
    DASHVIEW["dashboard_view.cpp/h<br/>C++ adapter"]
    DASHCORE["dashboard_view_core.c/h<br/>页面切换 + 触摸手势"]
    DASHPAGES["dashboard_pages.c/h<br/>LVGL 页面对象 + 档位点击"]
    STYLE["dashboard_ui_style.c/h<br/>颜色/样式/Label helper"]
  end

  subgraph AUDIO["语音输出 / 音频"]
    SESSION["audio_session.cpp/h<br/>I2S0 owner + capture + PCM output"]
    PROMPTS["audio_prompts.cpp/h<br/>预录档位播报"]
    ASSETS["audio_prompt_assets.cpp/h<br/>生成的 PCM 数组"]
    SELFTEST["audio_self_test.cpp/h<br/>beep + 麦克风 RMS + 串口模拟命令"]
  end

  subgraph VOICE["语音识别"]
    VOICECMD["voice_commands.cpp/h<br/>ESP-SR direct command"]
    SRMODELS["srmodels.bin<br/>pio_upload_srmodels.py 烧录"]
  end

  subgraph HW["硬件 / 平台"]
    BSP["board_support.cpp/h<br/>BoardSupport_Init"]
    LVGLPORT["lvgl_port.cpp/h<br/>LVGL display + touch driver"]
    LCD["Display_ST77916 / esp_lcd_st77916<br/>LCD_addWindow"]
    TOUCH["Touch_CST816.cpp/h<br/>TouchCst816_Read / ConsumeGesture"]
    I2C["I2C_Driver.cpp/h<br/>Codec / IO expander register IO"]
  end

  subgraph IDF["ESP-IDF Runtime 骨架"]
    RUNTIME["runtime/bike_runtime.cpp<br/>BikeRuntime_*"]
    UISVC["services/ui_service.cpp<br/>UiService_Start"]
    METRICSVC["services/metrics_service.cpp<br/>MetricsService_*"]
  end

  subgraph AI["AI 助手初版"]
    AIBUTTON["input/ai_button*<br/>BOOT/GPIO0 启动保护 + 消抖"]
    ASSISTANT["ai/ai_assistant.cpp/h<br/>bikemb_ai + command queue + snapshot"]
    AISTATE["ai/ai_state_machine.cpp/h<br/>纯状态 reducer + effects"]
    CLOUD["ai/cloud_worker.cpp/h<br/>bikemb_cloud real/mock stage worker"]
    PROVIDERS["Qwen ASR / Qwen Chat / CosyVoice<br/>DeepSeek adapter 未接入"]
    AIVIEW["app/ai_assistant_ui_state.cpp/h<br/>snapshot -> dashboard AI state"]
    WIFI["network/wifi_service*.cpp/h<br/>bikemb_wifi + 10 s reconnect"]
  end

  MAIN --> BSP
  MAIN --> LVGLPORT
  MAIN --> DASHAPP
  MAIN --> PROMPTS
  MAIN --> SELFTEST
  MAIN --> VOICECMD
  MAIN --> AIBUTTON
  MAIN --> ASSISTANT
  MAIN --> WIFI

  DASHAPP --> DASHVIEW
  DASHVIEW --> DASHCORE
  DASHCORE --> DASHPAGES
  DASHPAGES --> STYLE
  DASHCORE --> TOUCH
  LVGLPORT --> LCD
  LVGLPORT --> TOUCH
  PROMPTS --> ASSETS
  PROMPTS --> I2C
  SELFTEST --> I2C
  VOICECMD --> I2C
  VOICECMD --> SRMODELS
  AIBUTTON --> ASSISTANT
  ASSISTANT --> AISTATE
  ASSISTANT --> CLOUD
  ASSISTANT --> SESSION
  CLOUD --> PROVIDERS
  CLOUD --> SESSION
  WIFI --> ASSISTANT
  ASSISTANT --> AIVIEW
  AIVIEW --> DASHAPP

  MAIN --> RUNTIME
  RUNTIME --> UISVC
  UISVC --> LVGLPORT
  UISVC --> DASHAPP
  UISVC --> METRICSVC
```

## Arduino 主循环框架

文件：[main.cpp](/D:/MyProject/BikeMB/src/firmware/bikemb/src/main.cpp)

### 编译开关

| 开关 | 默认值 | 哪个环境打开 | 作用 |
| --- | --- | --- | --- |
| `BIKE_MB_RUN_DISPLAY_DIAGNOSTIC` | `0` | 手动 build flag | 只跑显示诊断，不跑 dashboard。 |
| `BIKE_MB_ENABLE_AUDIO_SELF_TEST` | `0` | `esp32-s3-touch-lcd-1-85c-audio-self-test` | 开启 beep、麦克风 RMS、串口 `n/p` 模拟翻页。 |
| `BIKE_MB_ENABLE_AUDIO_PROMPTS` | `0` | `esp32-s3-touch-lcd-1-85c-mode-prompts-test` | 开启档位点击预录语音播报。 |
| `BIKE_MB_ENABLE_VOICE_COMMANDS` | `0` | `esp32-s3-touch-lcd-1-85c-voice-direct-test` | 开启 ESP-SR 直接语音命令识别。 |
| `BIKE_MB_ENABLE_AI_ASSISTANT` | `0` | 三个 AI 测试环境 | 开启 AI task、BOOT 键、CloudWorker、UI snapshot 和 Wi-Fi service。 |
| `BIKE_MB_ENABLE_AUDIO_SESSION` | `0` | 音频与 AI voice 环境 | 初始化共享 codec/I2S0、录音 clip 和 PCM 输出。 |
| `BIKE_MB_AI_USE_MOCK_PROVIDERS` | `0` | `ai-framework-test`、`ai-voice-mock-test` | 使用本地阶段延时，不访问真实 STT/LLM/TTS。 |
| `BIKE_MB_AI_TLS_INSECURE_TEST_ONLY` | `0` | `ai-voice-cloud-test` | 测试环境跳过 TLS 证书校验；禁止用于发布。 |

### `setup()` 顺序

1. `Serial.begin(115200)`：打开串口。
2. `BoardSupport_Init()`：板级初始化，包含 LCD/I2C/背光等基础链路。
3. AI 开关打开时依次调用 `BikeMbAiAssistant_Init()`、`BikeMbWifiService_Init()`、`BikeMbAiButton_Init()`；默认环境不创建这些 task。
4. AudioSession 开关打开时调用 `BikeMbAudioSession_Init()`，统一初始化 codec 和 I2S0。
5. 初始化 Audio Capture Self Test、Audio Prompts、Audio Self Test 和 Voice Commands；未开启的模块为空实现。
6. `LvglPort_Init()`：初始化 LVGL 显示和 CST816 触摸输入。
7. `DashboardApp_Init()`：初始化指标服务并创建 dashboard UI。
8. `DashboardApp_SetModeChangedCallback(HandleModeChanged)`：把 UI 档位点击接到音频播报入口。

### `loop()` 顺序

1. 计算 `now` 和 `deltaMs`。
2. AI 开关打开时调用 `BikeMbAiButton_Tick(now)`；这里只读取 GPIO 和推进消抖，不等待网络。
3. `LvglPort_Tick(deltaMs)`：推进 LVGL tick。
4. `DashboardApp_Tick(now)`：更新 demo 数据，复制 AI snapshot，并刷新 UI。
5. `DashboardApp_SetRenderWorkMs(LvglPort_Run())`：运行 `lv_timer_handler()`，并记录渲染耗时。
6. `BikeMbAudioSelfTest_Tick(now)`：音频自检环境下处理串口命令和麦克风 RMS 打印。
7. `HandleAudioSelfTestCommand()`：把音频自检的 `n/p` 命令路由到 dashboard 翻页。
8. `HandleVoiceCommand()`：把语音识别结果路由到 dashboard 翻页。
9. `delay(5)`。

### `main.cpp` 里的命令路由函数

| 函数 | 来源 | 作用 |
| --- | --- | --- |
| `HandleModeChanged(uint8_t modeIndex)` | UI 档位点击 callback | 如果 `BIKE_MB_ENABLE_AUDIO_PROMPTS=1`，调用 `BikeMbAudioPrompts_PlayMode(...)`。 |
| `HandleAudioSelfTestCommand()` | 串口自检命令 | 消费 `BikeMbAudioSelfTest_ConsumeCommand()`，调用 `DashboardApp_NextPage()` / `DashboardApp_PreviousPage()`，再播放提示音。 |
| `HandleVoiceCommand()` | ESP-SR 识别结果 | 消费 `BikeMbVoiceCommands_ConsumeCommand()`，调用 dashboard 翻页命令。 |

## UI 框架

UI 分层的原则：触摸、语音、串口测试都不能直接操作 LVGL 页面，而是走 `DashboardApp_*` 命令入口。

### App 层

文件：[dashboard_app.h](/D:/MyProject/BikeMB/src/firmware/bikemb/src/app/dashboard_app.h)

| 函数 | 调用方 | 作用 |
| --- | --- | --- |
| `DashboardApp_Init()` | `main.cpp` / `UiService` | 初始化 `MetricsService`，创建 dashboard view。 |
| `DashboardApp_Tick(uint32_t nowMs)` | `main.cpp` / `UiService` | 更新 demo 数据、刷新 UI label 和波形。 |
| `DashboardApp_SetRenderWorkMs(uint32_t renderWorkMs)` | `main.cpp` / `UiService` | 把 LVGL 渲染耗时反馈给指标服务。 |
| `DashboardApp_ShowAiPage()` | AI Button | 稳定按下 AI 键时直接显示独立 AI 页面。 |
| `DashboardApp_NextPage()` | 触摸/音频/语音命令 | 下一页。 |
| `DashboardApp_PreviousPage()` | 触摸/音频/语音命令 | 上一页。 |
| `DashboardApp_SetModeChangedCallback(...)` | `main.cpp` | 注册档位变化 callback，用于档位播报。 |

### View Core 层

文件：[dashboard_view_core.h](/D:/MyProject/BikeMB/src/firmware/bikemb/src/app/dashboard_view_core.h)

| 函数/类型 | 作用 |
| --- | --- |
| `BikeMbDashboardMetrics` | C struct，承载所有要显示到 UI 的数据。 |
| `BikeMbDashboardModeChangedCallback` | 档位变化 callback 类型：`void (*)(uint8_t mode_index)`。 |
| `BikeMbDashboardView_Create()` | 创建 screen、首页/AI/详情三个页面容器、page dots 和子控件。 |
| `BikeMbDashboardView_Update(...)` | 更新 label、波形，并轮询触摸手势。 |
| `BikeMbDashboardView_NextPage()` | 显示下一页。 |
| `BikeMbDashboardView_PreviousPage()` | 显示上一页。 |
| `BikeMbDashboardView_SetModeChangedCallback(...)` | 把 callback 传给 `dashboard_pages`。 |

### Page 层

文件：

- [dashboard_pages.c](/D:/MyProject/BikeMB/src/firmware/bikemb/src/app/dashboard_pages.c)
- [dashboard_pages.h](/D:/MyProject/BikeMB/src/firmware/bikemb/src/app/dashboard_pages.h)
- [dashboard_ui_style.c](/D:/MyProject/BikeMB/src/firmware/bikemb/src/app/dashboard_ui_style.c)
- [dashboard_ui_style.h](/D:/MyProject/BikeMB/src/firmware/bikemb/src/app/dashboard_ui_style.h)

| 函数 | 作用 |
| --- | --- |
| `BikeMbDashboardPages_Create(...)` | 创建首页、独立 AI 页面、详细信息页。 |
| `BikeMbDashboardPages_Update(...)` | 更新所有页面上的文字和波形点。 |
| `BikeMbDashboardPages_SetModeChangedCallback(...)` | 保存档位变化 callback。 |
| `mode_click_event_cb(...)` | 私有 LVGL 点击事件：`ECO -> TRAIL -> BOOST -> ECO`，更新 label，并发出 mode index。 |
| `BikeMbUi_MakeFixedLabel(...)` | 创建固定宽度 label，减少圆屏 UI 文字溢出。 |
| `BikeMbUi_SetLabelTextIfChanged(...)` | 只有文字变化时才更新 LVGL label。 |

AI 状态沿用同一 UI 边界：`DashboardApp_Tick()` 复制 `BikeMbAiSnapshot`，通过 `BikeMbAiUiState_FromSnapshot(...)` 转为不含 provider 细节的 `BikeMbDashboardAiUiState`，再随 metrics 传给 View/Page。后台 AI、Wi-Fi 和 cloud task 不直接调用 LVGL。

页面不会再按时间自动跳转。旧的 `activePage = (now / 5000) % 3` 已经删除。当前页面只会因为这些入口变化：

- CST816 左右滑动手势。
- 音频自检环境下串口 `n/p`。
- 直接语音识别环境下识别到上一页/下一页。
- AI 按键稳定按下时调用 `DashboardApp_ShowAiPage()`，立即切到 AI 页面。

## AI 助手控制框架

### 当前实现状态

| 能力 | 当前状态 | 说明 |
| --- | --- | --- |
| 配置与密钥边界 | 已实现 | AI 默认关闭；真实密钥只允许放入 Git 忽略的 `ai_secrets.local.h`。 |
| BOOT/AI 键 | 已实现，待完整上板矩阵 | `GPIO0` 低电平有效；启动 3000 ms 内忽略，连续释放 50 ms 后解锁，运行边沿消抖 30 ms。 |
| AI 状态机 | 已实现并有 host test | 管理 request ID、60 s 云阶段 deadline、取消、短录音、10 s 上限、旧结果丢弃和 snapshot。 |
| AI control task | 已实现 | `bikemb_ai` 使用 8 项命令队列，每 20 ms 产生一次内部 tick。 |
| Cloud worker | 已实现真实与 mock 两条路径 | `bikemb_cloud` 串行执行 Qwen ASR、Qwen Chat、CosyVoice TTS；mock 环境只模拟阶段延时。 |
| Wi-Fi service | 已实现基础连接 | `bikemb_wifi` 后台轮询，每 10 s 重试；只向 AI Assistant 发布连接状态。 |
| AI UI 状态与独立页面 | 已实现展示 | snapshot 映射为 chip、mini overlay 或 full-page 语义；dashboard 第二页显示网络、AI 状态、操作提示、动态波形和状态环。 |
| Audio Session / 真实录音 | 已实现 | 统一拥有 I2S0/codec；16 kHz mono clip 在 PSRAM 中最多保存 10 秒。 |
| 真实 STT / LLM / TTS | 已实现初版 | 当前运行时为 Qwen ASR + Qwen Chat + CosyVoice；DeepSeek adapter 尚未接入。 |
| MP3 stream / Music Service | 计划中 | 解码器选择、流播放器和点歌 resolver 尚未实现。 |

### 模块与接口

| 模块 | 关键接口 | 当前职责 |
| --- | --- | --- |
| `input/ai_button_logic.*` | `BikeMbAiButtonLogic_Init/Update` | 纯启动保护和消抖 reducer，可由 host test 编译。 |
| `input/ai_button.*` | `BikeMbAiButton_Init/Tick` | 读取 `GPIO0`，把稳定边沿转成 Assistant press/release 命令。 |
| `ai/ai_state_machine.*` | `BikeMbAiStateMachine_Init/Dispatch` | 唯一业务状态 reducer，返回 audio/cloud effect bitmask。 |
| `ai/ai_assistant.*` | `Init`、`OnButtonPressed/Released`、`Cancel`、`SetWifiConnected`、`GetSnapshot` | 拥有 command queue、state machine 和并发安全 snapshot。 |
| `ai/cloud_worker.*` | `Init`、`Submit`、`CancelBefore` | 独立 worker；只接受带 `requestId/deadlineMs` 的 stage job。 |
| `audio/audio_session.*` | `Start/Poll/FinishCapture`、`WriteStereoPcm`、`Acquire/Release` | 统一音频 owner、录音 clip 所有权和 TTS PCM 输出。 |
| `ai/qwen_asr_adapter.*` | `BikeMbQwenAsr_WriteRequestJson` | 分块生成 Base64 WAV JSON。 |
| `ai/qwen_chat_adapter.*` | `WriteRequestJson`、`CopyBoundedAnswer` | 当前 LLM 短回答适配器。 |
| `ai/cosyvoice_tts_adapter.*` | `WriteRequestJson`、`HandleSseLineWithStream` | 生成 TTS 请求并解码 SSE/Base64 PCM。 |
| `network/wifi_service_core.*` | `Init/Update` | 与 Arduino Wi-Fi API 解耦的连接/重试 reducer。 |
| `network/wifi_service.*` | `BikeMbWifiService_Init` | 后台连接 Wi-Fi，把 connected/disconnected 事件投给 Assistant。 |
| `app/ai_assistant_ui_state.*` | `BikeMbAiUiState_FromSnapshot` | 把 AI 内部状态压缩为 UI 可显示状态、文案和操作提示。 |

### 当前数据流

```mermaid
sequenceDiagram
  participant Loop as Arduino loop
  participant Button as AiButton
  participant Ai as bikemb_ai
  participant State as AiStateMachine
  participant Audio as AudioSession
  participant Cloud as bikemb_cloud
  participant API as DashScope
  participant Wifi as bikemb_wifi
  participant App as DashboardApp/LVGL

  Loop->>Button: Tick(now), read GPIO0
  Button->>Ai: PRESS / RELEASE command
  Wifi->>Ai: SET_WIFI event
  Ai->>State: Dispatch(event)
  State-->>Ai: new snapshot + effect mask
  Ai->>Audio: capture start / poll / finish
  Ai->>Cloud: STT / LLM / TTS job(requestId, deadline)
  Cloud->>API: Qwen ASR / Chat / CosyVoice HTTPS
  Cloud->>Audio: TTS PCM playback
  Cloud-->>Ai: tagged stage result
  App->>Ai: GetSnapshot(copy)
  App->>App: map snapshot to dashboard AI UI state
```

`bikemb_ai` 是状态唯一写入者。Cloud/Wi-Fi/Button 只投递事件；UI 只复制 snapshot。取消会递增有效 `requestId` 并调用 `BikeMbCloudWorker_CancelBefore(...)`，因此已经排队或晚到的旧结果不能覆盖新状态。当前取消不能主动关闭阻塞中的 HTTPS，请求仍依赖 15 秒读取超时返回。

### BOOT 键安全边界

- `Key1/BOOT` 连接 `GPIO0`，板载 `10 kΩ` 上拉，按下为低电平。
- 前 `3000 ms` 不产生 AI 事件；保护期后必须先连续释放 `50 ms` 才进入 Armed。
- Armed 后按下/松开分别稳定 `30 ms` 才产生一次事件。
- GPIO0 仍是 ESP32-S3 启动 strap。按住 BOOT 上电或复位仍可能进入 ROM 下载模式；应用层延时不能消除该行为。
- 当前 OpenSpec 只确认了原理图，完整上板安全矩阵仍应在对应任务完成后勾选。

## 音频硬件引脚

当前音频层使用已经上板验证过的 Waveshare 引脚：

| 信号 | GPIO | 用途 |
| --- | --- | --- |
| I2S MCLK | GPIO2 | ES8311/ES7210 主时钟 |
| I2S BCLK | GPIO48 | I2S bit clock |
| I2S LRCK | GPIO38 | I2S word select |
| I2S DOUT | GPIO47 | 输出到 ES8311 喇叭链路 |
| I2S DIN | GPIO39 | ES7210 麦克风输入 |
| I2C SDA/SCL | GPIO10/GPIO11 | Codec 寄存器配置 |
| PA enable | GPIO15 | 功放使能 |

## 语音输出框架

现在有三条输出路径：音频自检提示音、档位预录语音播报和 AI TTS。三者通过 `AudioSession` 仲裁 I2S0 owner。

### 音频自检输出

文件：

- [audio_self_test.h](/D:/MyProject/BikeMB/src/firmware/bikemb/src/audio/audio_self_test.h)
- [audio_self_test.cpp](/D:/MyProject/BikeMB/src/firmware/bikemb/src/audio/audio_self_test.cpp)

开启方式：`BIKE_MB_ENABLE_AUDIO_SELF_TEST=1`

公共 API：

| 函数 | 作用 |
| --- | --- |
| `BikeMbAudioSelfTest_Init()` | 获取 AudioSession Self Test owner，播放一次开机 beep，并创建麦克风检测 task。 |
| `BikeMbAudioSelfTest_Tick(uint32_t nowMs)` | 读取串口 `n/p` 命令，每秒打印一次麦克风 mean-square。 |
| `BikeMbAudioSelfTest_PlayPageTone(bool nextPage)` | 翻页命令后播放短提示音。 |
| `BikeMbAudioSelfTest_ConsumeCommand()` | 取出待处理的模拟翻页命令，并清空队列。 |

私有实现：

| 函数 | 作用 |
| --- | --- |
| `writeTone(...)` | 生成方波 stereo sample，通过 `BikeMbAudioSession_WriteStereoPcm(...)` 输出。 |
| `reportMicLevel()` | 通过 `BikeMbAudioSession_ReadMicBytes(...)` 读样本并打印 mean-square。 |
| `readSerialCommand()` | 把串口 `n/N` 和 `p/P` 转成待处理命令。 |

### 档位预录语音播报

文件：

- [audio_prompts.h](/D:/MyProject/BikeMB/src/firmware/bikemb/src/audio/audio_prompts.h)
- [audio_prompts.cpp](/D:/MyProject/BikeMB/src/firmware/bikemb/src/audio/audio_prompts.cpp)
- [audio_prompt_assets.h](/D:/MyProject/BikeMB/src/firmware/bikemb/src/audio/audio_prompt_assets.h)
- [audio_prompt_assets.cpp](/D:/MyProject/BikeMB/src/firmware/bikemb/src/audio/audio_prompt_assets.cpp)
- [generate-mode-prompts.ps1](/D:/MyProject/BikeMB/tools/generate-mode-prompts.ps1)

开启方式：`BIKE_MB_ENABLE_AUDIO_PROMPTS=1`

公共 API：

| 函数/类型 | 作用 |
| --- | --- |
| `BikeMbAudioPromptMode` | 枚举：`ECO`、`TRAIL`、`BOOST`。值和 UI mode index 对齐。 |
| `BikeMbAudioPrompts_Init()` | 检查 AudioSession 可用性并创建后台播放 task。 |
| `BikeMbAudioPrompts_PlayMode(BikeMbAudioPromptMode mode)` | 提交档位语音播放请求后立即返回，不阻塞 UI 档位切换。 |

私有实现：

| 函数/数据 | 作用 |
| --- | --- |
| `promptTask(...)` | 后台等待 `xTaskNotify`，收到最新档位请求后播放对应 PCM。 |
| `g_requestSerial` | 播放请求版本号。连续切换档位时，新版本会让旧语音在下一个 chunk 前中断。 |
| `writePrompt(...)` | 获取 Prompt owner，把 16 kHz mono PCM 复制成 stereo frame 后交给 AudioSession，并检查是否被新请求打断。 |
| `kBikeMbPromptEcoPcm` | `经济模式` 的 PCM 数据。 |
| `kBikeMbPromptTrailPcm` | `越野模式` 的 PCM 数据。 |
| `kBikeMbPromptBoostPcm` | `增强模式` 的 PCM 数据。 |

播放时序：

1. UI 点击档位后先更新页面状态。
2. `main.cpp` 的 `HandleModeChanged(...)` 调用 `BikeMbAudioPrompts_PlayMode(...)`。
3. `BikeMbAudioPrompts_PlayMode(...)` 只更新 `g_requestedMode` / `g_requestSerial`，再用 `xTaskNotify(..., eSetValueWithOverwrite)` 唤醒后台 task。
4. `promptTask(...)` 在后台写 I2S。每写一个小块前比较 `expectedSerial != getRequestSerial()`。
5. 如果用户连续切换档位，旧语音停止，后台 task 转去播放最新档位语音。

语音资产生成流程：

1. 运行 `powershell -ExecutionPolicy Bypass -File tools\generate-mode-prompts.ps1`。
2. 脚本默认读取 `src/assets/audio/mode-prompts/eco.mp3`、`src/assets/audio/mode-prompts/trail.mp3`、`src/assets/audio/mode-prompts/boost.mp3`。
3. Windows Media Transcoder 先把源音频转为临时 WAV，输出到 `src/build/generated-prompts`。
4. 脚本再下混/重采样为 `16-bit mono PCM, 16000 Hz`。
5. 脚本把 PCM 转成 `audio_prompt_assets.cpp` 里的 C 数组。
6. `audio_prompt_assets.cpp` 本身也受 `BIKE_MB_ENABLE_AUDIO_PROMPTS` 保护，所以默认固件不会带入大语音数组。

当前限制：档位播报和 ESP-SR 识别都会使用 I2S 音频链路。现在先放在不同测试环境里，后续如果要同时“边听边播”，需要增加共享音频管理或暂停/恢复识别策略。

## 语音识别框架

文件：

- [voice_commands.h](/D:/MyProject/BikeMB/src/firmware/bikemb/src/voice/voice_commands.h)
- [voice_commands.cpp](/D:/MyProject/BikeMB/src/firmware/bikemb/src/voice/voice_commands.cpp)
- [pio_upload_srmodels.py](/D:/MyProject/BikeMB/tools/pio_upload_srmodels.py)

开启方式：`BIKE_MB_ENABLE_VOICE_COMMANDS=1`

公共 API：

| 函数/类型 | 作用 |
| --- | --- |
| `BikeMbVoiceCommand` | 枚举：`NONE`、`NEXT_PAGE`、`PREVIOUS_PAGE`。 |
| `BikeMbVoiceCommands_Init()` | 初始化 ES7210、I2S 输入、ESP-SR callback，并进入 direct command mode。 |
| `BikeMbVoiceCommands_ConsumeCommand()` | 取出待处理的语音命令，并清空队列。 |

私有实现：

| 函数/数据 | 作用 |
| --- | --- |
| `initMicrophoneCodec()` | 配置 ES7210，I2C 地址 `0x40`。 |
| `kSrCommands[]` | ESP-SR 命令表。当前是英文：`Next page`、`Next screen`、`Next song`、`Previous page`、`Previous screen`、`Go back`。 |
| `handleSrEvent(sr_event_t event, int commandId, int phraseId)` | ESP-SR callback。识别成功时写入 `g_pendingCommand`；timeout 时回到 command mode。 |
| `g_pendingCommand` | 单槽命令队列，由 `main.cpp` 消费。 |

`BikeMbVoiceCommands_Init()` 初始化顺序：

1. 打印 `direct command mode enabled`。
2. 调用 `initMicrophoneCodec()`。
3. 调用 `g_i2s.setPins(GPIO_NUM_48, GPIO_NUM_38, GPIO_NUM_47, GPIO_NUM_39, GPIO_NUM_2)`。
4. 以 `16000 Hz`、16-bit、stereo slot mode 启动 I2S。
5. 注册 `ESP_SR.onEvent(handleSrEvent)`。
6. 调用 `ESP_SR.begin(...)`，参数包含：
   - `SR_CHANNELS_STEREO`
   - `SR_MODE_COMMAND`
   - 输入格式字符串 `"MN"`

语音命令流：

```mermaid
sequenceDiagram
  participant Mic as ES7210 Mic
  participant SR as ESP_SR
  participant Voice as voice_commands.cpp
  participant Main as main.cpp
  participant App as DashboardApp

  Mic->>SR: 16 kHz stereo I2S samples
  SR->>Voice: handleSrEvent(SR_EVENT_COMMAND, commandId, phraseId)
  Voice->>Voice: set g_pendingCommand
  Main->>Voice: BikeMbVoiceCommands_ConsumeCommand()
  Main->>App: DashboardApp_NextPage() or DashboardApp_PreviousPage()
```

当前限制：当前 Arduino ESP-SR wrapper 在本工程里走英文命令。中文 `下一页/上一页` 还没有在这个路径中启用。

## 固件分区和构建环境框架

文件：[platformio.ini](/D:/MyProject/BikeMB/src/firmware/bikemb/platformio.ini)

### PlatformIO 环境

| Environment | Framework | 用途 | 关键配置 |
| --- | --- | --- | --- |
| `esp32-s3-touch-lcd-1-85c` | Arduino | 默认 LVGL dashboard 固件。 | 不打开音频/语音开关。 |
| `esp32-s3-touch-lcd-1-85c-audio-self-test` | Arduino | 音频输入/输出硬件验证。 | `BIKE_MB_ENABLE_AUDIO_SELF_TEST=1` |
| `esp32-s3-touch-lcd-1-85c-mode-prompts-test` | Arduino | 点击档位后播放预录语音。 | `BIKE_MB_ENABLE_AUDIO_PROMPTS=1` |
| `esp32-s3-touch-lcd-1-85c-audio-session-test` | Arduino | 共享 AudioSession 基础验证。 | `BIKE_MB_ENABLE_AUDIO_SESSION=1` |
| `esp32-s3-touch-lcd-1-85c-audio-capture-test` | Arduino | 10 秒有界麦克风采集。 | `BIKE_MB_ENABLE_AUDIO_SESSION=1`，`BIKE_MB_ENABLE_AUDIO_CAPTURE_SELF_TEST=1` |
| `esp32-s3-touch-lcd-1-85c-voice-direct-test` | Arduino | ESP-SR 直接识别上一页/下一页。 | `board_build.partitions = esp_sr_16.csv`，`BIKE_MB_ENABLE_VOICE_COMMANDS=1`，`extra_scripts = pre:../../../tools/pio_upload_srmodels.py` |
| `esp32-s3-touch-lcd-1-85c-ai-framework-test` | Arduino | BOOT 键、AI 状态机、UI 映射、Wi-Fi 和 mock cloud worker 集成验证。 | `BIKE_MB_ENABLE_AI_ASSISTANT=1`，`BIKE_MB_AI_USE_MOCK_PROVIDERS=1` |
| `esp32-s3-touch-lcd-1-85c-ai-voice-mock-test` | Arduino | 真实录音 + mock provider + 本地提示音。 | `BIKE_MB_ENABLE_AI_ASSISTANT=1`，`BIKE_MB_ENABLE_AUDIO_SESSION=1`，mock provider |
| `esp32-s3-touch-lcd-1-85c-ai-voice-cloud-test` | Arduino | Qwen ASR + Qwen Chat + CosyVoice 真实云闭环。 | AI + AudioSession，测试专用 insecure TLS |
| `esp32-s3-touch-lcd-1-85c-idf` | ESP-IDF | Runtime/Event/Service 迁移构建。 | `BIKE_MB_USE_ESPIDF_RUNTIME=1` |

### 默认分区

默认 Arduino 环境没有在仓库里指定自定义分区 CSV。它使用 board/framework 默认分区布局。这是当前稳定 LVGL 固件路径。

竖向内存图：

```mermaid
flowchart TB
  subgraph DEFAULT_FLASH["默认 / 音频播报环境 Flash 16MB"]
    D0["0x000000<br/>ROM/reserved<br/>芯片固定启动区域"]
    D1["0x001000<br/>Bootloader<br/>二级启动加载器"]
    D2["0x008000<br/>Partition Table<br/>分区表"]
    D3["0x009000 - 0x00DFFF<br/>nvs 20KB<br/>参数/校准/小型 KV"]
    D4["0x00E000 - 0x00FFFF<br/>otadata 8KB<br/>OTA 启动选择状态"]
    D5["0x010000 - 0x64FFFF<br/>app0 / ota_0 6.25MB<br/>当前或候选固件 A"]
    D6["0x650000 - 0xC8FFFF<br/>app1 / ota_1 6.25MB<br/>OTA 固件 B"]
    D7["0xC90000 - 0xFEFFFF<br/>spiffs 3.375MB<br/>文件系统资源"]
    D8["0xFF0000 - 0xFFFFFF<br/>coredump 64KB<br/>崩溃转储"]
  end
  D0 --> D1 --> D2 --> D3 --> D4 --> D5 --> D6 --> D7 --> D8
```

默认分区说明：

| 区域 | 内容 | BikeMB 当前用途 |
| --- | --- | --- |
| ROM/reserved | Flash 起始保留区和芯片启动相关固定区域。 | 不由应用直接写入。 |
| Bootloader | ESP32 二级启动加载器。 | 负责读取分区表并启动选中的 app。 |
| Partition Table | 分区表。 | 描述 `nvs`、`app0/app1`、`spiffs` 等区域位置。 |
| `nvs` | 非易失键值存储。 | 后续可放设置、校准值、累计里程等小数据。 |
| `otadata` | OTA 状态区。 | 记录当前应从 `app0` 还是 `app1` 启动。 |
| `app0` / `app1` | 两个 OTA 应用槽。 | 默认/音频播报固件空间较大，适合 LVGL + 语音资源测试。 |
| `spiffs` | SPI Flash 文件系统。 | 预留给图片、字体、配置等文件资源；当前主要资源仍编译进固件。 |
| `coredump` | 崩溃转储区。 | 发生异常时可保存崩溃信息，便于定位问题。 |

### ESP-SR 分区

直接语音识别环境使用 `esp_sr_16.csv`，因为 ESP-SR 需要模型分区。这个 CSV 由 Arduino/ESP32 framework 包提供，不是本仓库自维护文件。

竖向内存图：

```mermaid
flowchart TB
  subgraph SR_FLASH["ESP-SR voice-direct-test Flash 16MB"]
    S0["0x000000<br/>ROM/reserved<br/>芯片固定启动区域"]
    S1["0x001000<br/>Bootloader<br/>二级启动加载器"]
    S2["0x008000<br/>Partition Table<br/>ESP-SR 分区表"]
    S3["0x009000 - 0x00DFFF<br/>nvs 20KB<br/>参数/校准/小型 KV"]
    S4["0x00E000 - 0x00FFFF<br/>otadata 8KB<br/>OTA 启动选择状态"]
    S5["0x010000 - 0x30FFFF<br/>app0 / ota_0 3MB<br/>语音识别固件 A"]
    S6["0x310000 - 0x60FFFF<br/>app1 / ota_1 3MB<br/>语音识别固件 B"]
    S7["0x610000 - 0xC0FFFF<br/>spiffs 6MB<br/>文件系统资源"]
    S8["0xC10000 - 0xFEFFFF<br/>model / srmodels.bin 3.875MB<br/>ESP-SR 模型"]
    S9["0xFF0000 - 0xFFFFFF<br/>coredump 64KB<br/>崩溃转储"]
  end
  S0 --> S1 --> S2 --> S3 --> S4 --> S5 --> S6 --> S7 --> S8 --> S9
```

ESP-SR 分区说明：

| 区域 | 内容 | BikeMB 当前用途 |
| --- | --- | --- |
| ROM/reserved | Flash 起始保留区和芯片启动相关固定区域。 | 不由应用直接写入。 |
| Bootloader | ESP32 二级启动加载器。 | 读取 ESP-SR 分区表并启动选中的语音测试固件。 |
| Partition Table | `esp_sr_16.csv` 对应的分区表。 | 为 `model` 分区留出固定地址。 |
| `nvs` | 非易失键值存储。 | 后续可复用为设置、校准值、用户偏好。 |
| `otadata` | OTA 状态区。 | 管理 `app0/app1` 启动选择。 |
| `app0` / `app1` | 两个 3MB 应用槽。 | 语音识别环境为了给模型让空间，单个 app 空间比默认分区小。 |
| `spiffs` | SPI Flash 文件系统。 | ESP-SR 布局里保留 6MB 文件系统空间。 |
| `model` | ESP-SR 模型镜像分区。 | `pio_upload_srmodels.py` 把 `srmodels.bin` 烧录到 `0xC10000`。 |
| `coredump` | 崩溃转储区。 | 保存异常转储，便于定位语音/音频任务崩溃。 |

模型文件由 [pio_upload_srmodels.py](/D:/MyProject/BikeMB/tools/pio_upload_srmodels.py) 追加烧录：

```python
srmodels = framework_libs / "esp32s3" / "esp_sr" / "srmodels.bin"
env.Append(FLASH_EXTRA_IMAGES=[("0xC10000", str(srmodels))])
```

含义：

- `srmodels.bin` 来自 `framework-arduinoespressif32-libs`。
- 它作为 extra image 烧录到 `0xC10000`。
- 固件镜像和模型镜像必须匹配 `esp_sr_16.csv` 的分区布局。
- 如果找不到模型文件，脚本会在烧录前抛出 `FileNotFoundError`。

### 常用构建命令

建议使用项目内 `.pio-home`，避免全局 `.platformio` 权限问题：

```powershell
$env:PLATFORMIO_CORE_DIR = (Resolve-Path '.pio-home').Path
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = 'No'
py -X utf8 -m platformio run -s -d src\firmware\bikemb
```

构建档位播报固件：

```powershell
$env:PLATFORMIO_CORE_DIR = (Resolve-Path '.pio-home').Path
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = 'No'
py -X utf8 -m platformio run -s -d src\firmware\bikemb -e esp32-s3-touch-lcd-1-85c-mode-prompts-test
```

烧录档位播报固件：

```powershell
$env:PLATFORMIO_CORE_DIR = (Resolve-Path '.pio-home').Path
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = 'No'
py -X utf8 -m platformio run -s -d src\firmware\bikemb -e esp32-s3-touch-lcd-1-85c-mode-prompts-test -t upload
```

烧录直接语音识别固件：

```powershell
$env:PLATFORMIO_CORE_DIR = (Resolve-Path '.pio-home').Path
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = 'No'
py -X utf8 -m platformio run -s -d src\firmware\bikemb -e esp32-s3-touch-lcd-1-85c-voice-direct-test -t upload
```

构建 AI mock framework 固件：

```powershell
$env:PLATFORMIO_CORE_DIR = (Resolve-Path '.pio-home').Path
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = 'No'
py -X utf8 -m platformio run -s -d src\firmware\bikemb -e esp32-s3-touch-lcd-1-85c-ai-framework-test
```

构建真实 AI 语音固件：

```powershell
$env:PLATFORMIO_CORE_DIR = (Resolve-Path '.pio-home').Path
$env:PLATFORMIO_SETTING_ENABLE_TELEMETRY = 'No'
py -X utf8 -m platformio run -s -d src\firmware\bikemb -e esp32-s3-touch-lcd-1-85c-ai-voice-cloud-test
```

真实环境需要本地 `src/firmware/bikemb/include/ai_secrets.local.h`。该环境当前关闭证书校验，只用于开发上板验证。

## ESP-IDF Runtime 骨架

文件：

- [bike_runtime.cpp](/D:/MyProject/BikeMB/src/firmware/bikemb/src/runtime/bike_runtime.cpp)
- [bike_event.h](/D:/MyProject/BikeMB/src/firmware/bikemb/src/runtime/bike_event.h)
- [ui_service.cpp](/D:/MyProject/BikeMB/src/firmware/bikemb/src/services/ui_service.cpp)
- [metrics_service.cpp](/D:/MyProject/BikeMB/src/firmware/bikemb/src/services/metrics_service.cpp)

| 函数 | 作用 |
| --- | --- |
| `BikeRuntime_Init()` | 创建 event queue，并调用 `BoardSupport_Init()`。 |
| `BikeRuntime_Start()` | 启动 `UiService` 和 `RuntimeTickTask`。 |
| `BikeRuntime_PostEvent(...)` | 向固定 FreeRTOS queue 投递事件；队列满时丢弃低优先级 tick。 |
| `BikeRuntime_GetEventQueue()` | 返回 queue handle。 |
| `BikeRuntime_GetDroppedLowPriorityEvents()` | 返回低优先级事件丢弃计数。 |
| `UiService_Start(...)` | 启动拥有 LVGL 的 UI task。 |
| `UiTask(...)` | 初始化 LVGL/dashboard，消费事件，并调用 `LvglPort_Run()`。 |

当前边界：ESP-IDF runtime 还不是音频/语音的验证路径。AudioSession 和 AI 语音闭环目前依赖 Arduino Wi-Fi、`WiFiClientSecure` 和 `ESP_I2S`，迁移前需要新增对应 ESP-IDF transport/audio adapter。

## 验证入口

轻量合同测试在 `tools/tests`。

| 测试文件 | 保护内容 |
| --- | --- |
| `test_audio_self_test_contract.py` | 音频自检开关、引脚、API、dashboard 命令边界。 |
| `test_audio_prompts_contract.py` | 档位播报 API、生成资产、默认关闭、mode callback。 |
| `test_voice_commands_contract.py` | ESP-SR API、命令词、模型烧录脚本、语音环境分区配置。 |
| `test_dashboard_no_auto_page_contract.py` | 防止 demo metrics 再引入自动翻页。 |
| `test_lvgl_port_contract.py` | LVGL display/touch port 边界。 |
| `test_lvgl_simulator_contract.py` | PC simulator 与固件共享 dashboard UI 源码。 |
| `test_runtime_contract.py` | ESP-IDF runtime/event/service 骨架。 |
| `test_ai_framework.py` | AI 配置、BOOT 键 reducer、AI 状态机、UI 映射、task/queue 所有权和 main feature gate。 |
| `test_dashboard_ai_ui_contract.py` | Dashboard 只从 snapshot 派生 AI 页面，不直接依赖 cloud/audio/Assistant 命令接口。 |
| `test_wifi_service_contract.py` | Wi-Fi service 不阻塞启动、配置隔离、重连 reducer 和 Assistant 状态发布。 |
| `test_audio_session_contract.py` | AudioSession 唯一 I2S0 owner、capture 和 PCM 接口。 |
| `test_audio_capture_contract.py` | 16 kHz mono、有界 PSRAM clip 和 stereo-to-mono downmix。 |
| `test_cloud_worker_real_contract.py` | 真实 CloudWorker、Qwen/CosyVoice 串联、请求失效和资源边界。 |
| `test_qwen_asr_contract.py` / `test_qwen_chat_contract.py` | ASR Base64 WAV 与当前 Qwen Chat 请求边界。 |
| `test_cosyvoice_tts_contract.py` | CosyVoice SSE/Base64 PCM 解码与播放边界。 |
| `test_deepseek_adapter_contract.py` | 未接入运行时的 DeepSeek adapter 请求与长度限制。 |
| `ai_button_logic_test.cpp` | 3000 ms 启动保护、50 ms 释放解锁和 30 ms 消抖。 |
| `ai_state_machine_test.cpp` | 短录音、10 s 上限、60 s deadline、取消、忙态替换和旧 request 丢弃。 |
| `ai_assistant_ui_state_test.cpp` | AI snapshot 到 dashboard visual/surface/text 的纯映射。 |
| `wifi_service_core_test.cpp` | 未配置、连接、断开和 10 s 重试动作。 |

运行：

```powershell
powershell -ExecutionPolicy Bypass -File tools\run-tests.ps1
```

## 当前已知架构限制

- Voice Commands 仍未迁入 AudioSession，和共享音频/AI 环境保持编译期互斥。
- `voice-direct-test` 没有唤醒词，环境噪声可能导致误识别，所以不适合作为日常骑行 UI 固件。
- 当前 Arduino ESP-SR 路径还没有启用中文命令识别。
- AI 默认关闭；真实云闭环只在 `ai-voice-cloud-test` 环境启用，尚不是发布配置。
- 真实 LLM 路径当前使用 Qwen Chat；DeepSeek adapter 已实现但未接入 CloudWorker。
- 独立 AI 页面已经进入 dashboard 第二页，但目前是只读展示；取消、重试和停止控件尚未连接到 Assistant 命令接口，并且仍需上板验证布局和交互。
- 真实云环境使用测试专用 insecure TLS，尚无 CA 证书校验；CloudWorker 还会打印截断的转写和回答文本。
- 取消能使旧 request 失效并停止本地音频，但不能主动中断阻塞 HTTPS；单一 CloudWorker 会延迟后续云 job。
- CosyVoice PCM 当前完整缓冲后播放，PSRAM 上限约 625 KiB，不是低延迟流式播放。
- HTTPS MP3 解码器、Stream Player、Music Service 和未来点歌 resolver 尚未实现。
- ESP-IDF runtime 已经有结构，但 AI、音频和语音还没有迁移到这个运行模型。
