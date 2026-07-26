# 嵌入式 GUI + AI 云语音项目架构文档需求

版本: v1
适用项目: BikeMB / ESP32-S3 GUI + AI 云语音固件
维护位置: `docs/architecture/` 和 `docs/software-architecture.*`
用途: 后续架构文档、架构图、代码审查、性能分析、故障定位和 AI 辅助开发的编写与验收标准。

## 1. 文档目标

建立一套能够指导开发、代码审查、性能分析、故障定位和 AI 辅助开发的系统架构文档。

文档需要回答以下问题：

1. 硬件上电后，各电源、复位、时钟和外设按照什么顺序启动？
2. MCU 从复位到显示主界面、语音可用分别经历哪些阶段？
3. 使用什么操作系统，采用怎样的多核和多任务策略？
4. GUI、音频、网络、云语音和业务逻辑分别运行在哪些任务中？
5. 任务之间通过什么方式通信？
6. SRAM、PSRAM、Flash、文件系统和缓冲区如何分配？
7. 一次完整语音交互的数据如何在设备与云端之间流动？
8. 发生网络断开、云服务超时、音频异常或内存不足时如何恢复？

## 2. 目标读者

架构图需要同时服务于：

- 产品和项目负责人
- 嵌入式软件工程师
- GUI/LVGL 工程师
- 音频和语音工程师
- 云端和协议工程师
- 测试工程师
- AI 编程 Agent

同一张图只解决一种主要问题，不把所有信息放在一张图中。

## 3. 必须交付的架构图

### 图 1：系统上下文图

说明设备与外部系统的关系。

必须包含：

- 用户
- 触摸屏
- 麦克风
- 扬声器
- 手机或配置工具
- Wi-Fi、以太网或 4G
- 云端 ASR
- LLM
- 云端 TTS
- OTA 和设备管理服务
- 可选的 MCP 或 IoT 服务

该图不显示函数、任务优先级和内存地址。

### 图 2：硬件组成与电源树

说明主要硬件及连接关系。

必须包含：

- 主 MCU/MPU
- 内部 SRAM
- 外部 PSRAM
- Flash
- LCD 和背光
- Touch Controller
- 麦克风阵列
- Audio Codec
- 功放和扬声器
- Wi-Fi/4G 模块
- SD 卡
- 电池、电源管理芯片和各电源轨
- SPI、I2C、I2S、RGB、MIPI、UART、USB 等接口
- 中断、复位和使能信号

每个器件需要标注：

- 驱动接口
- 总线速率
- DMA 使用情况
- 主要内存需求
- 初始化负责人

### 图 3：硬件上电和复位时序图

必须使用时序图表达，不使用普通方框图。

至少包含：

- 主电源输入
- 各路 DCDC/LDO
- Power Good
- MCU Reset
- 外部 Flash/PSRAM
- LCD Reset
- Touch Reset
- Audio Codec Reset
- 功放 Enable/Mute
- 背光 Enable
- 网络模块 Enable

每个步骤需要标注：

- 前置条件
- 最小延时
- 最大等待时间
- 失败处理
- 是否允许重试
- 软件控制还是硬件自动完成

需要明确区分：

1. 电气上电时序
2. Bootloader 启动时序
3. 应用软件初始化时序

### 图 4：系统启动和初始化流程图

建议流程：

```text
ROM Boot
→ 二级 Bootloader
→ 分区表和固件校验
→ RTOS 启动
→ app_main
→ 基础 BSP 初始化
→ NVS/文件系统
→ 显示和启动画面
→ 音频硬件初始化
→ 网络初始化
→ GUI 服务启动
→ 语音服务启动
→ 云端鉴权
→ 进入 Ready 状态
```

需要定义启动里程碑：

- T0：设备复位
- T1：串口日志可用
- T2：屏幕亮起
- T3：启动界面可见
- T4：触摸可用
- T5：本地唤醒可用
- T6：网络连接成功
- T7：云语音可用
- T8：完整产品功能 Ready

同时标注：

- 可并行初始化的模块
- 必须串行初始化的模块
- 每个阶段的超时时间
- 降级启动策略
- 初始化失败后的错误界面

### 图 5：软件分层架构图

建议从下到上划分为：

1. Hardware
2. BSP / HAL
3. RTOS 与系统组件
4. 中间件
5. 系统服务
6. 业务与状态机
7. GUI 和用户交互
8. 云端服务

建议模块：

- BSP：LCD、Touch、Audio Codec、Mic、Speaker、Storage
- OS：FreeRTOS、Timer、Watchdog、Heap、Event、Queue
- Middleware：LVGL、TLS、HTTP、WebSocket、MQTT、Opus、JSON
- Voice Service：Audio Capture、AFE、Wake Word、Uploader、TTS Player
- System Service：Network、OTA、Storage、Settings、Power、Logging
- Application：设备状态机、页面导航、语音会话、业务功能
- GUI：View、Presenter/ViewModel、UI Event、Resource Manager

图中必须表达依赖方向，禁止业务层直接依赖具体硬件驱动。

### 图 6：RTOS 任务与通信图

每个任务必须登记：

- 任务名称
- 职责
- 优先级
- CPU Core Affinity
- Stack 大小
- 创建者
- 启动时间
- 调度方式
- 周期或触发条件
- 最大允许阻塞时间
- Watchdog 策略
- 使用的 Queue、Event Group、Semaphore、Mutex 或 Ring Buffer

建议至少考虑：

- System/Main Task
- GUI/LVGL Task
- Display Flush Task 或 DMA 回调
- Touch/Input Task
- Audio Capture Task
- AFE/Wake Word Task
- Audio Encode/Upload Task
- Network RX/TX Task
- Cloud Protocol Task
- TTS Download Task
- Audio Decode/Playback Task
- Storage Task
- OTA Task
- Logging/Diagnostics Task

需要明确：

- 哪些任务允许调用 LVGL
- 哪些任务只向 GUI 消息队列发送事件
- 音频实时任务是否固定到某个 CPU 核
- 网络阻塞是否会影响 GUI 和音频
- ISR 到任务的通信方式
- 高优先级任务是否可能产生优先级反转

### 图 7：AI 云语音交互时序图

至少覆盖一次完整会话：

```text
Idle
→ 本地唤醒
→ UI 显示 Listening
→ 麦克风采集
→ AFE/AEC/VAD
→ 编码
→ 上传云端
→ ASR 文本返回
→ LLM 推理
→ TTS 音频流返回
→ 本地解码播放
→ UI 显示 Speaking
→ 会话结束
→ 回到 Idle
```

需要增加异常分支：

- 未检测到有效语音
- 网络断开
- 鉴权失败
- ASR 超时
- LLM 超时
- TTS 中断
- 用户打断播放
- AEC 失效
- 云服务主动结束会话

图中分别标识：

- PCM 音频流
- 压缩音频流
- 控制消息
- 文本消息
- UI 状态事件

### 图 8：资源与内存预算图

Flash 部分：

- Bootloader
- Partition Table
- Factory App
- OTA A/B
- NVS
- 文件系统
- 字体
- 图片和动画
- 提示音
- 语音模型
- 崩溃转储

RAM/PSRAM 部分：

- RTOS Task Stack
- LVGL Heap
- Frame Buffer
- Draw Buffer
- 图片解码缓存
- 字体缓存
- 音频采集 DMA Buffer
- AFE 工作区
- Audio Ring Buffer
- Opus 编解码工作区
- TLS Buffer
- HTTP/WebSocket Buffer
- JSON Buffer
- OTA Buffer
- 日志缓存

每一项记录：

- 所在存储区域
- 静态或动态分配
- 正常值
- 峰值
- 生命周期
- 所有者
- 内存不足时的处理策略

## 4. 辅助文档

除图之外，还应提供以下表格。

### 模块清单

字段：

- 模块名
- 目录
- 职责
- 对外接口
- 依赖模块
- 运行上下文
- 维护负责人
- 单元测试位置

### 任务清单

字段：

- Task
- Priority
- Core
- Stack
- Trigger
- Deadline
- Watchdog
- IPC
- 资源所有权

### 资源预算表

字段：

- 资源
- 当前值
- 预算上限
- 测量方法
- 峰值场景
- 优化措施

### 状态机说明

建议至少定义：

- Booting
- Provisioning
- Connecting
- Idle
- Listening
- Thinking
- Speaking
- Updating
- Offline
- Error
- ShuttingDown

## 5. 图形表达规范

- 一张图只表达一个主要问题。
- 软件层使用从下到上的依赖关系。
- 时序图时间从上到下。
- 数据流和控制流使用不同线型。
- 同一模块在所有图中使用相同名称。
- 模块名称与代码目录和类名建立映射。
- 颜色不超过五类。
- 主图中不直接放函数级细节。
- 复杂模块通过子图展开。
- 每张图必须标注版本号、适用硬件和更新时间。
- 所有架构图源码进入 Git 仓库管理。

## 6. 建议采用的制图格式

优先使用文本化、可版本管理的格式：

- Mermaid：总体架构、流程图、状态机和简单时序图
- PlantUML：复杂时序图、组件图和部署图
- Draw.io：硬件框图、电源树和需要人工排版的图
- CSV/Markdown：任务表和资源预算表

不建议只保存 PNG 图片。图形源文件应与代码一同提交。

## 7. AI 生成架构图时的输入要求

AI 在生成图之前必须获得：

- MCU/MPU 型号
- 核数和主频
- SRAM、PSRAM 和 Flash 容量
- LCD 分辨率、色深和接口
- 音频 Codec 和麦克风数量
- 操作系统和 SDK 版本
- LVGL 版本
- 网络方式
- 云端 ASR/LLM/TTS 供应商
- 音频采样率、位宽和通道数
- 当前代码目录
- 当前任务创建代码
- 分区表和链接 Map 文件
- 实际 Heap、Stack 和 CPU 使用率

缺少数据时，AI 必须将内容标记为“待确认”或“建议值”，不能把假设写成现状。

## 8. 验收标准

架构文档完成后，应能够做到：

1. 新工程师在不阅读全部源码的情况下理解系统启动过程。
2. 能找到每一个主要任务的创建位置和通信对象。
3. 能判断哪些模块可以调用 LVGL。
4. 能追踪一帧音频从麦克风到云端再回到扬声器的完整路径。
5. 能定位主要 Flash、RAM 和 PSRAM 消耗。
6. 能识别启动、网络、语音和 GUI 的关键故障点。
7. 图中的模块名称能够映射到真实代码目录。
8. 代码结构发生变化后，可以通过 AI 或脚本更新架构图。

## 9. BikeMB 执行要求

- 后续新增或更新架构图时，先检查本文档的“必须交付的架构图”和“图形表达规范”。
- 如果当前代码或硬件资料不足以满足某张图，必须在图或表中标记“待确认”，不得把建议值写成现状。
- `docs/software-architecture.html` 可以作为面向阅读的聚合页，但图形源、Markdown 表格或生成输入也必须保留在 Git 仓库中。
- AI 辅助更新架构时，必须先读取当前代码目录、任务创建代码、分区配置和已有架构文档，再更新图表。
- 架构文档变化后，至少运行 `tools/run-tests.ps1`；涉及构建配置、任务模型、分区或硬件路径时，再运行对应 PlatformIO build 或记录为什么未运行。
