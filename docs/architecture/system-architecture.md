# BikeMB 系统架构文档

版本: v1
适用硬件: Waveshare `ESP32-S3-Touch-LCD-1.85C V2 / Rev2.0`
适用固件路径: `src/firmware/bikemb`
更新时间: 2026-07-27
编写依据: `docs/architecture/architecture-documentation-requirements.md`

## 0. 阅读说明

本文是 BikeMB 当前固件的规范化系统架构文档，用于指导开发、代码审查、性能分析、故障定位和 AI 辅助开发。

本文只把已经能从仓库、代码或项目文档确认的信息写成现状。缺少原理图确认、链接 map、运行时诊断或板级实测的数据，统一标为“待确认”或“建议值”。

当前已确认的基础输入：

| 项 | 当前值 | 来源 |
| --- | --- | --- |
| 开发板 | Waveshare `ESP32-S3-Touch-LCD-1.85C V2` | `docs/hardware-notes.md` |
| MCU | `ESP32-S3R8`, dual-core Xtensa LX7, 最高 240 MHz | `docs/hardware-notes.md`, `platformio.ini` |
| Flash | 16 MB | `docs/project-context.md`, `platformio.ini` |
| PSRAM | 8 MB | `docs/project-context.md` |
| 内部 SRAM | 待确认，需要 linker map 或 ESP-IDF memory report | 未在仓库文档中明确 |
| LCD | 360 x 360 round LCD, `ST77916`, QSPI | `docs/hardware-notes.md` |
| Touch | `CST816`, I2C `0x15`, INT `GPIO4` | `docs/hardware-notes.md` |
| Audio | ES7210 Mic Codec, ES8311 Speaker Codec | `src/audio/audio_session.cpp` |
| 网络 | ESP32-S3 内置 Wi-Fi STA | `src/network/wifi_service.cpp` |
| OS/SDK | Arduino 3.3.9 回归路径；ESP-IDF 迁移路径 | `platformio.ini` |
| LVGL | `lvgl/lvgl@^8.4.0` | `platformio.ini` |
| 云供应商 | Qwen ASR, Qwen Chat, CosyVoice TTS | `src/ai/ai_config.h`, `src/ai/cloud_worker.cpp` |
| 音频格式 | capture: 16 kHz mono int16; I2S stereo 16 bit | `audio_capture_core.h`, `audio_session.cpp` |
| 实测 Heap/Stack/CPU | 待确认；当前只有诊断日志代码，没有固化基线 | `bike_runtime.cpp`, `ui_service.cpp` |

## 1. 图 1：系统上下文图

图版本: v1
适用硬件: ESP32-S3-Touch-LCD-1.85C V2
更新时间: 2026-07-27

```mermaid
flowchart LR
  User["用户"] --> Touch["触摸屏 / CST816"]
  User --> Button["BOOT / AI 实体键"]
  User --> Mic["麦克风 / ES7210"]
  Speaker["扬声器 / ES8311"] --> User

  Touch --> Device["BikeMB 设备<br/>ESP32-S3 GUI + AI 固件"]
  Button --> Device
  Mic --> Device
  Device --> Speaker

  Phone["手机或配置工具<br/>待实现"] -.配置 / 配网 / 诊断.-> Device
  Device --> Net["Wi-Fi STA<br/>当前已实现"]
  Eth["以太网<br/>不适用"] -.未规划.-> Device
  Cellular["4G<br/>不适用"] -.未规划.-> Device

  Net --> ASR["云端 ASR<br/>Qwen qwen3-asr-flash"]
  Net --> LLM["LLM<br/>Qwen Chat 当前运行<br/>DeepSeek adapter 未接入"]
  Net --> TTS["云端 TTS<br/>CosyVoice"]
  Net -.未来.-> OTA["OTA / 设备管理<br/>待实现"]
  Net -.可选.-> IoT["MCP / IoT 服务<br/>待确认"]
```

边界说明：

- 当前 P0 码表功能必须离线可用，不依赖云端、手机或 OTA。
- 当前 AI 云语音是 P2 实验能力；ESP-IDF 云端语音基础录音和回复已经通过用户板测确认，但 ADR-0004 尚未关闭。
- OTA、设备管理、MCP/IoT、手机配置工具当前不是已实现能力。

## 2. 图 2：硬件组成与电源树

图版本: v1
适用硬件: ESP32-S3-Touch-LCD-1.85C V2
更新时间: 2026-07-27

```mermaid
flowchart TB
  Power["USB Type-C / 3.7V 电池<br/>PMIC/DCDC/LDO: 待确认"] --> Rail33["3V3 电源轨<br/>Power Good: 待确认"]
  Power --> BattAdc["电池分压 ADC<br/>GPIO8, 约 1/3 分压"]

  Rail33 --> MCU["ESP32-S3R8<br/>dual-core, 240 MHz<br/>SRAM: 待确认<br/>PSRAM: 8 MB"]
  MCU --- Flash["外部 Flash<br/>16 MB<br/>Boot/App/NVS/FS"]
  MCU --- PSRAM["外部 PSRAM<br/>8 MB<br/>LVGL/音频优先使用情况见资源表"]

  MCU -- "QSPI / SPI2_HOST<br/>SCK GPIO40<br/>D0 GPIO46 D1 GPIO45 D2 GPIO42 D3 GPIO41<br/>CS GPIO21<br/>速率/DMA: 待确认" --> LCD["ST77916 LCD<br/>360 x 360"]
  MCU -- "PWM GPIO5" --> Backlight["LCD 背光"]
  MCU -- "GPIO18" --> Te["LCD TE"]

  MCU -- "I2C GPIO10/GPIO11<br/>建议 400 kHz" --> TCA["TCA9554PWR<br/>EXIO expander"]
  TCA -- "EXIO2 Reset" --> LCD
  TCA -- "EXIO1 Reset" --> Touch["CST816 Touch<br/>I2C addr 0x15<br/>INT GPIO4"]
  MCU -- "I2C GPIO10/GPIO11" --> Touch

  MCU -- "I2C codec control<br/>addr 0x40" --> MicCodec["ES7210 Mic Codec"]
  MCU -- "I2C codec control<br/>addr 0x18" --> SpkCodec["ES8311 Speaker Codec"]
  MCU -- "I2S0 16 kHz / 16 bit / stereo<br/>BCLK GPIO48 WS GPIO38 DOUT GPIO47 DIN GPIO39 MCLK GPIO2" --> MicCodec
  MCU -- "I2S0 16 kHz / 16 bit / stereo" --> SpkCodec
  SpkCodec --> Amp["功放 / Speaker path<br/>Enable/Mute: 待确认"]
  Amp --> Speaker["扬声器"]

  MCU -- "Wi-Fi STA 内置射频" --> Wifi["2.4 GHz Wi-Fi<br/>外部天线/射频细节: 待确认"]
  MCU -- "USB D-/D+ GPIO19/GPIO20" --> Usb["USB CDC / 烧录 / 日志"]
  MCU -- "UART TX GPIO43 RX GPIO44" --> Uart["外部 UART header"]
  SD["SD 卡<br/>当前未确认硬件连接"] -.待确认.-> MCU
```

硬件登记表：

| 器件 | 驱动接口 | 总线速率 | DMA 使用 | 主要内存需求 | 初始化负责人 |
| --- | --- | --- | --- | --- | --- |
| ESP32-S3R8 | CPU/SoC | 240 MHz | SoC 内部 | task stack, heap, Wi-Fi/TLS | ESP-IDF/Arduino startup, `BikeRuntime_Start()` |
| Flash 16 MB | SPI flash | 待确认 | Bootloader/flash driver 内部 | app/FS/NVS/model/coredump | Bootloader, partition table |
| PSRAM 8 MB | Octal/外部 PSRAM，具体模式待确认 | 待确认 | Heap caps allocator | audio clip, TTS PCM, 可选大缓冲 | ESP-IDF heap, AudioSession, CloudWorker |
| ST77916 LCD | QSPI / SPI2_HOST | 待确认 | 当前 `LCD_addWindow()` 细节待确认 | LVGL draw buffer: 默认约 57.6 KB internal SRAM | `BoardSupport_Init()`, `LvglPort_Init()` |
| CST816 Touch | I2C `0x15`, INT `GPIO4` | 建议 400 kHz | 无 | 小结构体 | `LvglPort_Init()` |
| TCA9554PWR | I2C `0x20` | 建议 400 kHz | 无 | 无显著需求 | `BoardSupport_Init()` |
| ES7210 | I2C control + I2S0 input | I2S 16 kHz/16 bit/stereo 当前路径 | I2S DMA 细节待确认 | capture clip 最长约 320 KB PSRAM | `AudioSession` |
| ES8311 | I2C control + I2S0 output | I2S 16 kHz/16 bit/stereo 当前路径 | I2S DMA 细节待确认 | TTS playback buffers | `AudioSession` |
| Wi-Fi | ESP32-S3 内置 Wi-Fi STA | AP 决定 | ESP-IDF Wi-Fi 内部 | Wi-Fi/lwIP/TLS heap 待实测 | `bikemb_wifi` |
| SD 卡 | 待确认 | 待确认 | 待确认 | 待确认 | 未实现 |

## 3. 图 3：硬件上电和复位时序图

图版本: v1
适用硬件: ESP32-S3-Touch-LCD-1.85C V2
更新时间: 2026-07-27

当前仓库没有完整电源树时序、PMIC 型号、Power Good 延时、LCD/Codec 最小复位脉宽和最大等待时间。因此本图把电气时序标为待确认，只把软件已经执行的初始化顺序写成现状。

```mermaid
sequenceDiagram
  autonumber
  participant PWR as 主电源输入
  participant RAIL as DCDC/LDO/3V3
  participant MCU as ESP32-S3 Reset
  participant ROM as ROM Bootloader
  participant BL as Flash 二级 Bootloader
  participant APP as app_main / Runtime
  participant BSP as BSP 外设
  participant GUI as GUI
  participant AUDIO as Audio
  participant NET as Wi-Fi/Cloud

  PWR->>RAIL: 电气上电，电源轨爬升
  Note over PWR,RAIL: 前置条件/最小延时/PG 信号/失败处理: 待确认
  RAIL->>MCU: 释放 MCU Reset
  Note over MCU: GPIO0 在复位采样阶段仍是下载模式 strap
  MCU->>ROM: CPU0 从 Mask ROM 启动
  ROM->>BL: 正常 Flash boot 时加载 0x000000 二级 Bootloader
  BL->>BL: 读取分区表，选择并校验 app 镜像
  BL->>APP: 跳转应用 entry_addr / call_start_cpu0
  APP->>APP: 拉起 CPU1，启动 FreeRTOS scheduler
  APP->>BSP: BoardSupport_Init: I2C, TCA9554, Backlight, LCD
  BSP->>GUI: LvglPort_Init: LVGL, draw buffers, display, touch
  APP->>AUDIO: AudioSession_Init, 条件启用
  APP->>NET: Wi-Fi service, 条件启用
  NET->>NET: 云端鉴权和连接，条件启用
```

时序缺口：

| 区段 | 当前状态 | 需要补充 |
| --- | --- | --- |
| 电气上电时序 | 待确认 | PMIC/DCDC/LDO 型号、Power Good、各电源轨最小/最大延时 |
| LCD Reset | 代码通过 TCA9554/LCD driver 初始化，具体复位时序待确认 | ST77916 reset pulse、ready wait、失败重试策略 |
| Touch Reset | 代码通过 TCA9554/LVGL touch init，具体复位时序待确认 | CST816 reset pulse、INT ready、失败重试策略 |
| Audio Codec Reset | AudioSession 写 codec 寄存器，硬件 reset/enable 线待确认 | ES7210/ES8311 reset、MCLK/I2S 前置条件 |
| 功放 Enable/Mute | 待确认 | GPIO/Codec 寄存器/外部功放使能和静音策略 |
| 网络模块 Enable | ESP32-S3 内置 Wi-Fi，无独立 enable 线证据 | RF/PHY 初始化失败恢复策略 |

## 4. 图 4：系统启动和初始化流程图

图版本: v1
适用硬件: ESP32-S3-Touch-LCD-1.85C V2
更新时间: 2026-07-27

```mermaid
flowchart TD
  T0["T0 设备复位"] --> ROM["ROM Boot<br/>GPIO0 strap 采样"]
  ROM --> BL["二级 Bootloader<br/>加载/校验 app"]
  BL --> RTOS["应用 call_start_cpu0/cpu1<br/>FreeRTOS scheduler"]
  RTOS --> APP["app_main 或 Arduino setup"]
  APP --> BSP["基础 BSP 初始化<br/>I2C/TCA9554/背光/LCD"]
  BSP --> NVS["NVS/文件系统<br/>当前 Wi-Fi 会初始化 NVS<br/>FS 使用: 待确认"]
  NVS --> Splash["显示启动画面<br/>当前无专门 splash: 待实现"]
  Splash --> GUI["GUI 服务启动<br/>LVGL + Dashboard"]
  GUI --> Touch["触摸可用<br/>CST816 init 成功时"]
  GUI --> Audio["音频硬件初始化<br/>条件启用 AudioSession"]
  GUI --> Net["网络初始化<br/>条件启用 bikemb_wifi"]
  Audio --> Voice["语音服务启动<br/>AI / ESP-SR 按环境启用"]
  Net --> Auth["云端鉴权<br/>DashScope token 本地配置"]
  Voice --> Ready["Ready / 降级 Ready"]
  Auth --> Ready

  Net -.失败.-> Offline["Offline<br/>Dashboard 继续运行"]
  Audio -.失败.-> AudioOff["Audio unavailable<br/>AI/语音降级"]
  GUI -.失败.-> ErrorUi["错误界面<br/>当前待实现"]
```

启动里程碑：

| 里程碑 | 当前定义 | 当前状态 |
| --- | --- | --- |
| T0 | 设备复位 | 已知 |
| T1 | 串口日志可用 | Arduino/ESP-IDF 均有日志；准确时间待实测 |
| T2 | 屏幕亮起 | `BoardSupport_Init()` 中背光和 LCD 初始化；准确时间待实测 |
| T3 | 启动界面可见 | 当前直接 Dashboard，专门 splash 待实现 |
| T4 | 触摸可用 | `LvglPort_Init()` 中 CST816 成功时可用 |
| T5 | 本地唤醒可用 | 常驻唤醒词未实现；BOOT/AI 键上电 3000 ms 后可作为 AI 键 |
| T6 | 网络连接成功 | `bikemb_wifi` 后台连接；准确时间待实测 |
| T7 | 云语音可用 | Wi-Fi + token + ASR/LLM/TTS 可访问；ESP-IDF 基础录音/回复已确认 |
| T8 | 完整产品功能 Ready | P0 Dashboard 可用；音乐/点歌/OTA 未 Ready |

初始化策略：

| 模块 | 串行/并行 | 超时 | 降级策略 |
| --- | --- | --- | --- |
| Bootloader 到 app | 串行 | ESP-IDF 默认，具体待确认 | Bootloader 失败恢复待确认 |
| BSP/LCD | 串行 | 待确认 | 当前失败处理不足，错误界面待实现 |
| LVGL draw buffer | 串行 | 立即分配 | 当前分配失败后循环等待；需改成错误界面或降级 |
| AudioSession | 可在 UI ready 后条件启动 | codec/I2S 超时待确认 | 音频不可用，Dashboard 继续运行 |
| Wi-Fi | 可并行后台启动 | 10 s 重试连接 | Offline，AI 云能力不可用 |
| Cloud ASR/LLM/TTS | AI 请求时触发 | 总 deadline 60 s，HTTPS read 15 s | AI Error，Dashboard 继续运行 |

## 5. 图 5：软件分层架构图

图版本: v1
适用硬件: ESP32-S3-Touch-LCD-1.85C V2
更新时间: 2026-07-27

```mermaid
flowchart BT
  subgraph HW["1 Hardware"]
    H1["ESP32-S3R8"]
    H2["ST77916 LCD / Backlight"]
    H3["CST816 Touch"]
    H4["ES7210 / ES8311 / Speaker"]
    H5["Flash / PSRAM"]
    H6["Wi-Fi RF"]
  end

  subgraph BSP["2 BSP / HAL"]
    B1["BoardSupport"]
    B2["Display_ST77916"]
    B3["Touch_CST816"]
    B4["AudioSession codec/I2S"]
    B5["I2C / TCA9554"]
  end

  subgraph OS["3 RTOS 与系统组件"]
    O1["FreeRTOS Task"]
    O2["Queue / portMUX"]
    O3["Heap caps"]
    O4["Timer / esp_timer"]
  end

  subgraph MW["4 Middleware"]
    M1["LVGL 8.4"]
    M2["esp_http_client / TLS"]
    M3["JSON adapters"]
    M4["ESP-SR direct command test"]
  end

  subgraph SVC["5 系统服务"]
    S1["Runtime/Event"]
    S2["UiService"]
    S3["Wi-Fi Service"]
    S4["Cloud Worker"]
    S5["AudioSession"]
  end

  subgraph APP["6 业务与状态机"]
    A1["DashboardApp"]
    A2["AI Assistant"]
    A3["AI State Machine"]
    A4["MusicService 计划"]
  end

  subgraph GUI["7 GUI 和用户交互"]
    G1["Dashboard View/Core"]
    G2["Pages"]
    G3["AI UI State"]
    G4["Touch/Button commands"]
  end

  subgraph CLOUD["8 云端服务"]
    C1["Qwen ASR"]
    C2["Qwen Chat"]
    C3["CosyVoice TTS"]
    C4["OTA/Device Mgmt 待实现"]
  end

  BSP --> HW
  OS --> BSP
  MW --> OS
  SVC --> MW
  SVC --> BSP
  APP --> SVC
  GUI --> APP
  CLOUD --> SVC
```

依赖规则：

- 业务层不得直接依赖 LCD、Touch、Codec、I2S 等具体硬件驱动。
- `bike_ui` 是唯一允许调用 LVGL 的产品 task。
- Cloud provider adapter 不依赖 Dashboard 或 LVGL。
- AudioSession 是 I2S0/codec 的共享 owner，其他生产模块不得创建第二个 I2S0 owner。

## 6. 图 6：RTOS 任务与通信图

图版本: v1
适用硬件: ESP32-S3-Touch-LCD-1.85C V2
更新时间: 2026-07-27

```mermaid
flowchart LR
  subgraph Core0["Core 0 / runtime, network, AI"]
    Main["main_task<br/>ESP-IDF 创建"]
    Runtime["bike_runtime<br/>prio 4 stack 4096 words<br/>5 ms poll"]
    Ai["bikemb_ai<br/>prio 2 stack 12 KB<br/>20 ms tick"]
    Cloud["bikemb_cloud<br/>prio 1 stack 12 KB<br/>blocking HTTPS"]
    Wifi["bikemb_wifi<br/>prio 1 stack 6144 B<br/>1 s poll"]
    Audio["AudioSession<br/>非 task, I2S0 owner"]
  end

  subgraph Core1["Core 1 / UI"]
    Ui["bike_ui<br/>prio 5 stack 8192 words<br/>LVGL owner"]
  end

  Main --> Runtime
  Runtime -- "BikeEvent queue len 16" --> Ui
  Runtime -- "button poll" --> Ai
  Ai -- "BikeMbAiEvent queue len 8" --> Ai
  Ai -- "Cloud job queue len 4" --> Cloud
  Wifi -- "connected/offline callback" --> Ai
  Ai -- "capture owner/request_id" --> Audio
  Cloud -- "TTS PCM write" --> Audio
  Ai -- "snapshot + portMUX" --> Ui
```

任务清单：

| Task / 执行体 | 职责 | Priority | Core | Stack | 创建者 | 启动时间 | 调度/触发 | 最大阻塞 | Watchdog | IPC/同步 | 资源所有权 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `main_task` | 调用 `app_main()` | ESP-IDF 默认 | 通常 CPU0 | sdkconfig 决定，待确认 | ESP-IDF | RTOS 启动后 | 一次性 | 待确认 | ESP-IDF 默认，待确认 | 无 BikeMB IPC | 不拥有 LVGL/I2S |
| `bike_runtime` | 系统 tick、DashboardTick、AI button poll、诊断日志 | 4 | Core 0 | 4096 words | `BikeRuntime_Start()` | Runtime start | 5 ms loop | 当前不应阻塞；日志 10 s 一次 | 待确认 | `g_eventQueue` len 16 | event producer |
| `bike_ui` | LVGL/Dashboard 唯一 owner | 5 | Core 1 | 8192 words | `UiService_Start()` | Runtime start | queue wait 5 ms + `lv_timer_handler()` | 不应被网络/音频阻塞 | 待确认 | 消费 `g_eventQueue` | LVGL, Dashboard |
| `bikemb_ai` | AI 状态机、录音轮询、云阶段编排 | 2 | Core 0 | 12 KB | `BikeMbAiAssistant_Init()` | AI feature enabled | queue + 20 ms tick | 不执行 HTTPS 阻塞 | 待确认 | `s_commandQueue` len 8, `s_snapshotMux` | AI state/snapshot |
| `bikemb_cloud` | STT/LLM/TTS 阻塞调用和 TTS 播放 | 1 | Core 0 | 12 KB | `BikeMbCloudWorker_Init()` | AI init | job queue | HTTPS read timeout 15 s，总 deadline 60 s | 待确认 | `s_queue` len 4, request/clip mux | cloud buffers, TTS PCM |
| `bikemb_wifi` | Wi-Fi STA 初始化、后台连接和重连 | 1 | Core 0 | 6144 B | `BikeMbWifiService_Init()` | AI feature enabled | 1 s poll, 10 s retry | Wi-Fi API 阻塞待实测 | 待确认 | 状态机 + callback | Wi-Fi STA/netif |
| `AudioSession` | I2S0/codec owner、录音和播放仲裁 | 非 task | Core 0 计划归属 | 无 task stack | `BikeRuntime_Start()` 条件调用 | Audio feature enabled | 被 AI/Cloud 调用 | I2S read/write 超时待确认 | 待确认 | owner/request_id | ES7210/ES8311/I2S0 |
| `loopTask` | Arduino 回归路径主循环 | Arduino 默认 | 通常 Core 1 | Arduino 默认，待确认 | Arduino core | Arduino 启动 | `loop()` | 不适用产品路径 | 待确认 | 无 runtime queue | 回归路径 |
| `bikemb_prompt` | 档位提示音后台播放 | 1 | 未 pin | 4096 | `BikeMbAudioPrompts_Init()` | feature enabled | task notify overwrite | 播放期间阻塞自身 | 待确认 | `xTaskNotify` | prompt PCM playback |
| `bikemb-audio-mic` | 音频自检麦克风读取 | 1 | Core 0 | 4096 B | `BikeMbAudioSelfTest_Init()` | self-test env | loop/read | 待确认 | 待确认 | task handle | self-test mic |
| `bikemb-audio-capture` | capture self-test | 1 | 未 pin | 4096 B | `BikeMbAudioCaptureSelfTest_Init()` | self-test env | test task | 待确认 | 待确认 | task handle | test capture |

通信约束：

- 只有 `bike_ui` 可调用 LVGL。
- Core 0 task 只通过 runtime event、snapshot 或 App 命令边界影响 GUI。
- 网络阻塞集中在 `bikemb_cloud`，不应阻塞 `bike_ui`。
- ISR 到任务通信当前未形成产品级统一规范；LCD/Touch driver 内部 ISR/DMA 细节待确认。
- 当前没有优先级反转实测；`bikemb_ai` 与 `bikemb_cloud` 使用 portMUX 和 queue，后续需在长稳测试中检查。

## 7. 图 7：AI 云语音交互时序图

图版本: v1
适用硬件: ESP32-S3-Touch-LCD-1.85C V2
更新时间: 2026-07-27

```mermaid
sequenceDiagram
  autonumber
  participant User as 用户
  participant UI as bike_ui / Dashboard
  participant BTN as BOOT/AI Button
  participant AI as bikemb_ai
  participant Audio as AudioSession
  participant Cloud as bikemb_cloud
  participant ASR as Qwen ASR
  participant LLM as Qwen Chat
  participant TTS as CosyVoice
  participant Spk as Speaker

  User->>BTN: 按住 AI 键
  BTN->>UI: ShowAiPage event
  BTN->>AI: BUTTON_PRESSED
  AI->>UI: snapshot: Listening
  AI->>Audio: StartCapture(request_id, 10s)
  Audio-->>AI: PCM 16k mono clip in PSRAM
  User->>BTN: 松开 AI 键
  BTN->>AI: BUTTON_RELEASED
  AI->>Audio: FinishCapture
  AI->>Cloud: STT job + request_id
  Cloud->>ASR: Base64 WAV JSON / HTTPS
  ASR-->>Cloud: ASR text
  Cloud->>AI: STT result
  AI->>UI: snapshot: Thinking
  AI->>Cloud: LLM job
  Cloud->>LLM: UTF-8 text / HTTPS JSON
  LLM-->>Cloud: short answer text
  Cloud->>AI: LLM result
  AI->>UI: snapshot: Synthesizing
  AI->>Cloud: TTS job
  Cloud->>TTS: text/event-stream request
  TTS-->>Cloud: Base64 PCM chunks
  Cloud->>Audio: WriteStereoPcm(gain 2x)
  Audio->>Spk: I2S stereo PCM
  Cloud->>AI: playback done
  AI->>UI: snapshot: Speaking then Idle

  alt 无有效语音或录音短于 300 ms
    AI->>Audio: Release capture
    AI->>UI: Idle / cancelled
  else 网络断开
    Cloud-->>AI: error / wifi offline
    AI->>UI: Error, Dashboard continues
  else 鉴权失败或 ASR/LLM/TTS 超时
    Cloud-->>AI: tagged error with request_id
    AI->>UI: Error 1500 ms, old result ignored
  else 用户打断播放
    User->>BTN: 再次按下
    AI->>Cloud: CancelBefore(new request_id)
    AI->>Audio: Release local audio
    AI->>UI: Listening for new request
  end
```

消息类型：

| 类型 | 路径 | 当前格式 |
| --- | --- | --- |
| PCM 音频流 | Mic -> AudioSession -> PSRAM clip | 16 kHz mono int16 |
| 压缩音频流 | 当前未实现 | HTTPS MP3 / Opus 均待实现 |
| 控制消息 | Button/Runtime/AI/Cloud | FreeRTOS queue event + request_id |
| 文本消息 | ASR/LLM | UTF-8, STT 512B, answer 192B |
| UI 状态事件 | AI snapshot -> UI | copy snapshot, UI 只读 |

异常恢复：

| 异常 | 当前处理 | 待补 |
| --- | --- | --- |
| 无有效语音 | 短录音取消，不提交 STT | VAD/静音检测待实现 |
| 网络断开 | AI Error / Wi-Fi 后台重连，Dashboard 继续 | 断网 UI 和重试策略需板测 |
| 鉴权失败 | cloud error，脱敏显示 | CA/证书和 token 配置流程待完善 |
| ASR/LLM/TTS 超时 | 总 deadline 60 s，HTTPS read 15 s | 主动中断 HTTPS 待实现 |
| 用户打断播放 | 新 request_id 失效旧结果，释放本地音频 | 当前阻塞 HTTPS 仍需等 timeout |
| AEC 失效 | AEC 未实现 | AFE/AEC/VAD 方案待定 |
| 云服务主动结束 | 解析错误/阶段失败 | 需要更细错误码映射 |

## 8. 图 8：资源与内存预算图

图版本: v1
适用硬件: ESP32-S3-Touch-LCD-1.85C V2
更新时间: 2026-07-27

```mermaid
flowchart TB
  subgraph Flash["Flash 16 MB"]
    F1["Bootloader<br/>offset 0x000000"]
    F2["Partition Table<br/>offset 0x008000"]
    F3["NVS<br/>idf: 0x9000 size 0x6000"]
    F4["phy_init<br/>idf: 0xf000 size 0x1000"]
    F5["factory app<br/>idf: 0x10000 size 4M"]
    F6["OTA A/B<br/>默认 Arduino/ESP-SR 外部布局: 待固化"]
    F7["File system / fonts / images / prompts<br/>待确认"]
    F8["ESP-SR model<br/>voice-direct-test 外部 esp_sr_16.csv<br/>srmodels.bin 位置待固化"]
    F9["coredump<br/>布局待确认"]
  end

  subgraph RAM["Internal SRAM / DRAM"]
    R1["Task stacks<br/>runtime/ui/ai/cloud/wifi"]
    R2["LVGL draw buffers<br/>默认 2 x 360 x 40 x sizeof(lv_color_t)<br/>约 57.6 KB if RGB565"]
    R3["JSON body 4096B<br/>HTTP write 1024B"]
    R4["I2S poll temp stack<br/>约 512B per capture poll"]
    R5["Wi-Fi/lwIP/TLS heap<br/>待实测"]
  end

  subgraph PSRAM["PSRAM 8 MB"]
    P1["Audio capture clip<br/>16k mono int16 10s 约 320KB<br/>MAX_BYTES 384KB"]
    P2["CosyVoice TTS PCM<br/>max 320000 samples 约 640KB"]
    P3["SSE line buffer 12KB<br/>优先 PSRAM"]
    P4["图片/字体/动画缓存<br/>待实现/待确认"]
  end
```

资源预算表：

| 资源 | 区域 | 当前值 | 预算上限 | 生命周期 | 所有者 | 内存不足处理 |
| --- | --- | --- | --- | --- | --- | --- |
| `bike_runtime` stack | SRAM | 4096 words | 待实测 | task 常驻 | Runtime | 记录 high-water，阈值待定 |
| `bike_ui` stack | SRAM | 8192 words | 待实测 | task 常驻 | UiService | 记录 high-water，阈值待定 |
| `bikemb_ai` stack | SRAM | 12 KB | 待实测 | AI enabled 常驻 | AI Assistant | 创建失败返回 false |
| `bikemb_cloud` stack | SRAM | 12 KB | 待实测 | AI enabled 常驻 | CloudWorker | 创建失败返回 false |
| `bikemb_wifi` stack | SRAM | 6144 B | 待实测 | AI enabled 常驻 | Wi-Fi Service | 创建失败返回 false |
| Runtime event queue | SRAM heap | 16 x `BikeEvent` | 固定 16 | 常驻 | Runtime | 低优先级 tick 可丢弃 |
| AI command queue | SRAM heap | 8 x `BikeMbAiEvent` | 固定 8 | AI 常驻 | AI Assistant | enqueue 失败当前未上报，待补 |
| Cloud job queue | SRAM heap | 4 x `BikeMbCloudJob` | 固定 4 | Cloud 常驻 | CloudWorker | submit 失败返回 false |
| LVGL draw buffers | internal SRAM | 默认约 57.6 KB if RGB565 | `BIKE_MB_LVGL_BUFFER_LINES` 控制 | UI 常驻 | LvglPort | 当前失败后无限等待，需改进 |
| Audio capture clip | PSRAM | 最长约 320 KB，宏上限 384 KB | 10 s / 16 kHz mono int16 | 单次录音 | AudioSession -> CloudWorker | 分配失败进入 capture failure |
| TTS PCM | PSRAM 优先 | 最多 320000 samples 约 640 KB | 固定上限 | 单次 TTS | CloudWorker | 分配失败返回 TTS error |
| TTS playback window | PSRAM/SRAM | 最多 96000 samples | 固定上限 | 单次播放 | CloudWorker/AudioSession | 超出截断播放 |
| SSE line | PSRAM 优先 | 12 KB | 固定上限 | 单次 TTS | CloudWorker | 分配失败 fallback 或错误 |
| JSON body | stack/SRAM | 4096 B | 固定上限 | 单次 HTTP | CloudWorker | 超出解析失败 |
| Wi-Fi/lwIP/TLS | SRAM/PSRAM | 待实测 | 待定义 | Wi-Fi/HTTPS 期间 | ESP-IDF | 需 heap trace |
| Flash factory app | Flash | ESP-IDF: 4 MB | 当前 CSV 固定 | 常驻 | Partition table | 超出 build 失败 |
| NVS | Flash | ESP-IDF: 0x6000 | 当前 CSV 固定 | 常驻 | NVS/Wi-Fi | 初始化失败需降级策略 |

## 9. 模块清单

| 模块 | 目录 | 职责 | 对外接口 | 依赖模块 | 运行上下文 | 维护负责人 | 单元测试位置 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Runtime | `src/runtime`, `src/services` | ESP-IDF task/core/event 边界 | `BikeRuntime_*`, `UiService_Start` | BoardSupport, UiService | `bike_runtime`, `bike_ui` | 软件架构师/开发工程师 | `tools/tests/test_runtime*.py`, `runtime_plan_test.cpp` |
| Board Support | `src/platform`, `src/drivers` | I2C, TCA9554, Backlight, LCD bring-up | `BoardSupport_Init` | Display/Touch drivers | startup | 嵌入式开发 | `test_display_driver_contract.py` |
| LVGL Port | `src/platform/lvgl_port.*` | LVGL display/touch/tick/run | `LvglPort_*` | ST77916, CST816 | `bike_ui` | GUI 工程师 | `test_lvgl_port_contract.py` |
| Dashboard App | `src/app` | UI 命令边界和页面状态 | `DashboardApp_*` | View, AI UI state | `bike_ui` / Arduino loop | GUI 工程师 | `test_dashboard*.py` |
| AI Button | `src/input` | BOOT/AI 键 guard/debounce | `BikeMbAiButton_*` | ai_config, runtime event | `bike_runtime` poll | 嵌入式开发 | `test_ai_framework.py` |
| AI Assistant | `src/ai/ai_assistant.*` | AI 状态机唯一写入者 | `BikeMbAiAssistant_*` | AudioSession, CloudWorker | `bikemb_ai` | AI/嵌入式 | `test_ai_framework.py` |
| Cloud Worker | `src/ai/cloud_worker.*` | STT/LLM/TTS 串行 worker | `BikeMbCloudWorker_*` | Qwen/CosyVoice adapters, AudioSession | `bikemb_cloud` | 云端/协议 | `test_cloud_worker_real_contract.py`, `test_idf_cloud_transport_contract.py` |
| Wi-Fi Service | `src/network` | Wi-Fi STA 后台连接 | `BikeMbWifiService_*` | esp_wifi/Arduino WiFi | `bikemb_wifi` | 网络工程师 | `test_wifi_service_contract.py`, `test_idf_wifi_service_contract.py` |
| AudioSession | `src/audio/audio_session.*` | I2S0/codec/clip owner | `BikeMbAudioSession_*` | ES7210/ES8311/I2S | 非 task 共享服务 | 音频工程师 | `test_audio_session_contract.py`, `test_idf_audio_session_contract.py` |
| Voice Commands | `src/voice` | ESP-SR direct command 测试 | `BikeMbVoiceCommands_*` | ESP-SR, Audio input | 专项环境 | 音频/语音 | `test_voice_commands_contract.py` |
| Audio Prompts | `src/audio/audio_prompts.*` | 档位播报测试 | `BikeMbAudioPrompts_*` | AudioSession/assets | `bikemb_prompt` | 音频工程师 | `test_audio_prompts_contract.py` |
| Music Service | `src/music` 计划 | 点歌/音乐 URL 管理 | 待定义 | Stream Player | 未实现 | 待定 | 待建立 |

## 10. 状态机说明

系统级目标状态：

```mermaid
stateDiagram-v2
  [*] --> Booting
  Booting --> Provisioning: 配置缺失
  Booting --> Connecting: Wi-Fi feature enabled
  Booting --> Idle: Dashboard ready
  Provisioning --> Connecting: 配网完成 / 待实现
  Connecting --> Idle: 网络可用或降级完成
  Connecting --> Offline: 网络失败
  Idle --> Listening: AI button pressed
  Listening --> Thinking: valid capture + ASR
  Thinking --> Speaking: LLM + TTS ready
  Speaking --> Idle: playback done
  Idle --> Updating: OTA start / 待实现
  Updating --> Idle: update done
  Offline --> Connecting: retry
  Idle --> Error: local fatal error
  Listening --> Error: audio/cloud error
  Thinking --> Error: cloud timeout
  Speaking --> Error: playback error
  Error --> Idle: recoverable
  Idle --> ShuttingDown: power off / 待实现
  ShuttingDown --> [*]
```

状态登记：

| 状态 | 当前实现程度 | Owner | 进入条件 | 退出条件 | UI 行为 |
| --- | --- | --- | --- | --- | --- |
| Booting | 部分实现 | startup/runtime | reset/app start | Dashboard init | 当前无专门启动页 |
| Provisioning | 未实现 | 待定 | Wi-Fi/用户配置缺失 | 配置完成 | 待定义 |
| Connecting | 部分实现 | Wi-Fi Service | AI/Wi-Fi enabled | connected/offline | AI 页面显示连接状态 |
| Idle | 已实现 | Dashboard/AI | 初始化完成或 AI 结束 | 用户输入 | Dashboard 正常 |
| Listening | 已实现 | AI Assistant | AI key pressed | key released/cancel/error | AI 页面/状态提示 |
| Thinking | 已实现 | AI Assistant/Cloud | ASR text ready | LLM/TTS result/error | 正在思考/合成 |
| Speaking | 已实现但时序待修正 | AI/Cloud/Audio | TTS playback done callback 当前短暂出现 | Idle/Error | 正在回答 |
| Updating | 未实现 | OTA | OTA start | reboot/success/fail | 待定义 |
| Offline | 部分实现 | Wi-Fi/AI | Wi-Fi disconnected | reconnect | Dashboard 不受影响 |
| Error | 已实现于 AI，系统错误待补 | AI/System | capture/cloud/playback fail | timeout/retry | 脱敏错误 |
| ShuttingDown | 未实现 | Power | power off | off | 待定义 |

## 11. 故障定位入口

| 故障 | 首看模块 | 关键日志/指标 | 当前恢复 |
| --- | --- | --- | --- |
| 屏幕不亮 | BoardSupport, Display_ST77916 | 串口、背光、LCD init | 待补错误界面 |
| 触摸无效 | LvglPort, Touch_CST816 | `CST816 touch ready/init failed` | Dashboard 仍可运行 |
| GUI 卡顿 | bike_ui, LvglPort | render_ms, handler_max_us, flush_count | 待定义阈值 |
| 录音失败 | AudioSession, AI Assistant | capture start/finish logs | AI Error |
| ASR/LLM/TTS 超时 | CloudWorker | request_id, stage, 15s read timeout | AI Error, Dashboard 继续 |
| Wi-Fi 断开 | Wi-Fi Service | connected/offline, 10s retry | Offline, 后台重连 |
| 内存不足 | Runtime/UI diagnostics | heap free/largest, PSRAM free/largest, stack HWM | 局部失败；全局策略待补 |
| 音频 underrun | AudioSession/CloudWorker | 当前未记录 underrun | ADR-0004 待验收 |

## 12. 文档缺口和下一步

| 缺口 | 为什么缺 | 下一步 |
| --- | --- | --- |
| 电源树和硬件复位时序 | 当前文档未记录 PMIC、电源轨和 Power Good | 从原理图提取并补 Draw.io 或 Mermaid 时序 |
| LCD/QSPI 速率和 DMA | driver 内部细节未在本文确认 | 读取 `Display_ST77916` 和 ESP-IDF LCD config，补实测 |
| I2S DMA buffer | 当前只记录 AudioSession 层缓冲 | 补 I2S driver config 和 runtime heap trace |
| 内部 SRAM 基线 | 缺 linker map 和 `idf.py size` / PlatformIO size 输出 | 固化 build size 报告 |
| Heap/Stack/CPU 峰值 | 只有诊断代码，无验收样本 | 跑 30 分钟长稳，保存串口摘要 |
| Watchdog 策略 | 代码未显式登记每 task WDT 策略 | 明确 TWDT 是否启用和每 task 策略 |
| OTA/Storage/Settings | 当前未实现 | MusicService 后再规划，不能阻塞 P0 |
| AFE/AEC/VAD/Opus | 当前未实现 | 选型和资源预算单独评估 |

## 13. 验收映射

| 验收标准 | 当前文档覆盖 |
| --- | --- |
| 新工程师理解启动过程 | 图 3、图 4、启动里程碑表 |
| 找到每个主要任务创建位置和通信对象 | 图 6、任务清单、模块清单 |
| 判断哪些模块可调用 LVGL | 软件分层、RTOS 通信约束 |
| 追踪一帧音频完整路径 | 图 7、消息类型、资源表 |
| 定位 Flash/RAM/PSRAM 消耗 | 图 8、资源预算表 |
| 识别启动/网络/语音/GUI 故障点 | 故障定位入口 |
| 图中模块名映射真实代码目录 | 模块清单 |
| 代码变化后可由 AI 或脚本更新 | 本文 Mermaid/Markdown 源码入 Git |
