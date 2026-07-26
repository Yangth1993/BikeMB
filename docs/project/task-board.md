# 多角色任务看板

本文是 BikeMB 多角色协作的统一交接入口。产品经理、UI/UX 设计师、软件架构师、开发工程师通过本文记录任务状态、输入输出和阻塞项。

## 状态约定

- 阶段：`PM`、`UX`、`ARCH`、`DEV`、`VERIFY`、`DONE`、`BLOCKED`
- 角色状态：`todo`、`doing`、`done`、`blocked`、`n/a`
- 涉及功能或行为变更时，正式规格仍进入 `openspec/changes/`

## 任务列表

| ID | 标题 | 阶段 | 产品经理 | UI/UX | 软件架构师 | 开发工程师 | 阻塞 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| [TASK-0001](#task-0001-建立多角色协作流程) | 建立多角色协作流程 | DONE | done | n/a | done | done | 无 |
| [TASK-0002](#task-0002-esp-idf-双核运行基线) | ESP-IDF 双核运行基线 | DONE | done | done | done | done | 无 |
| [TASK-0003](#task-0003-esp-idf-wi-fi-与-4mb-app-分区) | ESP-IDF Wi-Fi 与 4MB app 分区 | DONE | done | n/a | done | done | 无 |
| [TASK-0004](#task-0004-cst816-触摸与-bootai-键输入验收) | CST816 触摸与 BOOT/AI 键输入验收 | DONE | done | done | done | done | 无 |
| [TASK-0005](#task-0005-esp-idf-audiosession-codeci2s-初始化) | ESP-IDF AudioSession codec/I2S 初始化 | DONE | done | n/a | done | done | 无 |
| [TASK-0006](#task-0006-esp-idf-云端语音基础闭环) | ESP-IDF 云端语音基础闭环 | DONE | done | done | done | done | 无 |
| [TASK-0007](#task-0007-esp-idf-语音助手调优稳定版) | ESP-IDF 语音助手调优稳定版 | DONE | done | done | done | done | 无 |
| [TASK-0008](#task-0008-关闭-adr-0004-esp-idf-双核门禁) | 关闭 ADR-0004 ESP-IDF 双核门禁 | BLOCKED | done | n/a | done | blocked | 当前 Windows 未枚举串口；取消路径、长稳资源基线、audio underrun 待上板验收 |
| [TASK-0009](#task-0009-规范化系统架构文档) | 规范化系统架构文档 | DONE | done | done | done | done | 无 |
| [TASK-0010](#task-0010-脱敏-uiux-figma-展示稿交付) | 脱敏 UI/UX Figma 展示稿交付 | DONE | done | done | n/a | n/a | 无 |

## TASK-0001 建立多角色协作流程

### 状态

- 阶段：DONE
- 产品经理：done
- UI/UX：n/a
- 软件架构师：done
- 开发工程师：done
- 阻塞：无

### 目标

建立项目级规则，让产品经理、UI/UX 设计师、软件架构师和开发工程师通过文档完成任务交接，减少用户重复干预。

### 输出

- `AGENTS.md` 增加多角色协作规则。
- `docs/project/task-board.md` 作为任务状态和交接入口。
- `docs/project/decisions.md` 记录项目级协作决策。

### 验收

- 新任务能在本文登记。
- 每个角色有明确职责和状态。
- 角色交接不依赖口头上下文。
- 阻塞和用户确认项能被明确记录。

## TASK-0002 ESP-IDF 双核运行基线

### 状态

- 阶段：DONE
- 产品经理：done
- UI/UX：done
- 软件架构师：done
- 开发工程师：done
- 阻塞：无

### 目标

建立 `src/firmware/bikemb` 的 ESP-IDF 双核运行基线，确认 LVGL 单 owner 和运行时任务归属。

### 输出

- `esp32-s3-touch-lcd-1-85c-idf` 构建成功。
- 固件烧录到 `COM5` 后进入 BikeMB ESP-IDF runtime。
- 串口确认 `bike_runtime` 在 Core 0，`bike_ui` 在 Core 1。
- AI、cloud、Wi-Fi、button poll 等非 UI 任务不拥有 LVGL。

### 验收

- 30 秒以上诊断无 WDT、panic 或重复重启。
- UI 稳态 `render_ms=1-2`，启动峰值已记录。
- 按住 BOOT/AI 键复位到 `LVGL UI service ready` 前不误触发 AI capture。

### 交接记录

- PM -> UX：保持圆屏主 UI 可用，不因双核迁移改变基础体验。
- UX -> ARCH：LVGL 必须由 UI owner 单点驱动。
- ARCH -> DEV：按 runtime/UI 分核与任务 owner 约束实现。
- DEV -> VERIFY：构建、烧录、串口长稳和 UI 性能日志已记录到 `docs/project/current-plan.md`。

## TASK-0003 ESP-IDF Wi-Fi 与 4MB app 分区

### 状态

- 阶段：DONE
- 产品经理：done
- UI/UX：n/a
- 软件架构师：done
- 开发工程师：done
- 阻塞：无

### 目标

接入 ESP-IDF Wi-Fi STA 路径，并解决默认 1MB factory app 分区不足问题。

### 输出

- Wi-Fi 状态完成 `offline`、`connect start`、`online` 流转。
- `bikemb_wifi` 保持 Core 0 且不拥有 LVGL。
- 新增 16MB flash 分区配置，factory app 扩为 `4M`。

### 验收

- ESP-IDF Wi-Fi 版构建成功，烧录后串口确认分区表 factory app 为 `00400000`。
- 45 秒观察无 WDT、panic 或重复重启。
- Wi-Fi 迁移后 heap、PSRAM、stack HWM 和 UI 稳态日志已记录。

## TASK-0004 CST816 触摸与 BOOT/AI 键输入验收

### 状态

- 阶段：DONE
- 产品经理：done
- UI/UX：done
- 软件架构师：done
- 开发工程师：done
- 阻塞：无

### 目标

完成 ESP-IDF UI owner 侧 CST816 触摸链路和 BOOT/AI 实体键输入链路验收。

### 输出

- CST816 初始化日志输出 `CST816 touch ready`。
- 触摸手势日志输出 `[BikeMB][touch] gesture ...`。
- AI/BOOT 键增加初始化参数、周期诊断和 raw/debounce 事件日志。

### 验收

- 固件重新构建并烧录到 `COM5`。
- 90 秒以上观察无 WDT、panic 或重复重启。
- 用户确认触摸交互正常。
- AI button poll 已运行并完成释放解锁；用户确认按键正常，本轮不再单独复测 raw pressed/released。

## TASK-0005 ESP-IDF AudioSession codec/I2S 初始化

### 状态

- 阶段：DONE
- 产品经理：done
- UI/UX：n/a
- 软件架构师：done
- 开发工程师：done
- 阻塞：无

### 目标

在 ESP-IDF 环境下初始化 AudioSession、I2S0、ES8311 和 ES7210，不启用 AI/云/自检/音乐。

### 输出

- 新增 `esp32-s3-touch-lcd-1-85c-idf-audio-session-test` 环境。
- 使用 ESP-IDF `driver/i2s_std.h` 初始化音频链路。
- 串口确认 `audio_session core=0 owns_lvgl=0`、`session enabled`、`session ready`。

### 验收

- 构建成功并烧录到 `COM5`。
- 25 秒以上观察无 WDT、panic 或重复重启。
- AudioSession 初始化后的 heap、PSRAM、stack HWM 和 UI 稳态日志已记录。

## TASK-0006 ESP-IDF 云端语音基础闭环

### 状态

- 阶段：DONE
- 产品经理：done
- UI/UX：done
- 软件架构师：done
- 开发工程师：done
- 阻塞：无

### 目标

完成默认关闭的 ESP-IDF AI Assistant 云端语音基础闭环：录音、ASR/Chat、TTS 回复。

### 输出

- 新增无声 `esp32-s3-touch-lcd-1-85c-idf-ai-voice-cloud-test` 构建环境。
- 启用 ESP-IDF AI Assistant、Wi-Fi、AudioSession 和真实 Qwen ASR/Qwen Chat transport。
- Cloud Worker 增加 CosyVoice SSE/TTS 路径，解析 SSE/Base64 PCM 并写入 AudioSession。

### 验收

- 本地构建成功，资源占用约 RAM `23.3%`、Flash `10.2%`。
- 固件已烧录成功一次。
- 用户板级确认录音和回复基础功能正常，覆盖录音、ASR/Chat 和 TTS 回复基础链路。

## TASK-0007 ESP-IDF 语音助手调优稳定版

### 状态

- 阶段：DONE
- 产品经理：done
- UI/UX：done
- 软件架构师：done
- 开发工程师：done
- 阻塞：无

### 目标

针对板测反馈优化 ESP-IDF 云端语音助手的音量、回复长度和中文识别稳定性，保存当前稳定版本。

### 输出

- TTS 增加 2x 饱和增益。
- Qwen Chat 缩短回答上限，并增加 UTF-8 安全截断。
- Qwen ASR 固定 `language=zh`。

### 验收

- `esp32-s3-touch-lcd-1-85c-idf-ai-voice-cloud-test` 构建成功。
- 资源占用约 RAM `23.2%`、Flash `10.2%`。
- 已烧录到 `COM5`。
- 用户确认调优版整体不错，可作为当前稳定语音助手版本保存。

## TASK-0008 关闭 ADR-0004 ESP-IDF 双核门禁

### 状态

- 阶段：BLOCKED
- 产品经理：done
- UI/UX：n/a
- 软件架构师：done
- 开发工程师：blocked
- 阻塞：当前 Windows 未枚举串口；取消路径、长稳资源基线、audio underrun 待上板验收

### 目标

完成 ESP-IDF 双核迁移剩余验收，关闭 ADR-0004 前保持 `MusicService` 和点歌功能冻结。

### 已完成输入

- 双核 task 归属、LVGL 单 owner 和启动稳定性已完成第一轮验收。
- Wi-Fi STA、4MB app 分区、CST816 触摸、BOOT/AI 键、AudioSession 初始化已完成板级确认。
- ESP-IDF Qwen ASR/Qwen Chat/CosyVoice TTS transport 已构建通过。
- 用户已确认最新语音固件录音和回复基础功能正常。

### 待完成输出

- ESP-IDF 取消路径回归。
- 语音闭环长稳资源基线。
- audio underrun 基线。
- ADR-0004 关闭建议。

### 验收

- 记录 heap、PSRAM、task stack high-water mark、UI 延迟和 audio underrun。
- 长稳观察期间无 WDT、panic 或重复重启。
- 取消路径不造成 UI 卡死、音频残留或请求串扰。
- 软件架构师确认门禁可关闭后，更新 ADR 或项目问题清单。

### 软件架构师输出

- 任务性质：门禁验收，不新增产品功能，不进入 `MusicService` 或点歌。
- 角色顺序：产品经理确认目标和非目标已足够清晰；UI/UX 本轮 n/a；软件架构师输出验收策略；开发工程师执行自动化测试、串口观察和板级检查。
- 验收顺序：
  1. 启动后观察 60 秒，确认无 WDT、panic、重复重启，记录 runtime/UI heap、PSRAM、stack HWM 和 UI handler 延迟。
  2. 录音中取消、云端等待中取消、TTS 播放中取消，确认旧 request 不继续更新 UI、不残留音频 owner、不串扰下一次请求。
  3. 连续多轮语音闭环后观察资源与 UI 延迟，记录是否出现 I2S timeout、underrun 或明显卡顿。
- 风险：取消测试和 underrun 基线必须依赖硬件、麦克风、喇叭和串口；当前串口不可用时不能关闭 ADR-0004。

### 开发工程师输出

- 本地验证已完成：`python tools\tests\test_ai_framework.py`、`python tools\tests\test_runtime_contract.py`、`python tools\tests\test_runtime_plan_contract.py`、`python tools\tests\test_idf_cloud_transport_contract.py`、`python tools\tests\test_idf_audio_session_contract.py` 均通过。
- 代码检查结论：取消机制已有状态机 `CANCEL_AUDIO/CANCEL_CLOUD`、`BikeMbCloudWorker_CancelBefore()` 和 `BikeMbAudioSession_ReleaseAll()` 路径；仍需板级验证实际录音、HTTPS 等待和 TTS 播放期间的表现。
- 当前阻塞：`python -X utf8 -m serial.tools.list_ports` 返回 `no ports found`，无法执行串口长稳和取消板测。

### 交接记录

- PM -> ARCH（2026-07-27）：产品侧复核通过。`TASK-0008` 不是新增产品功能，当前目标仍是关闭 ADR-0004 前的稳定性和资源验收；不扩大到 `MusicService`、点歌、AI 产品能力新增或 UI 改版。无需新增产品需求文档；若验证中发现需要改变用户可见行为，再回到 PM/OpenSpec。
- ARCH -> DEV（2026-07-27）：只补齐取消回归、长稳资源和 underrun 验收，不开发 `MusicService` 或点歌；串口恢复后先做 60 秒启动长稳，再做三段取消，再做连续语音闭环资源观察。
- DEV -> VERIFY（2026-07-27）：本地契约测试通过；当前阻塞在 Windows 未枚举串口，待串口恢复后执行板级验收并回写 `docs/project/current-plan.md` 与本文。

## TASK-0009 规范化系统架构文档

### 状态

- 阶段：DONE
- 产品经理：done
- UI/UX：done
- 软件架构师：done
- 开发工程师：done
- 阻塞：无

### 目标

按用户提供的《嵌入式 GUI + AI 云语音项目架构文档需求》建立一份新的规范化系统架构文档，后续架构文档、图表、任务表和资源预算表均按该需求验收。

### 输入

- 用户原始需求：建立面向产品、嵌入式、GUI、音频、云端、测试和 AI Agent 的系统架构文档规范，并按规范更新一份新文档。
- 相关文档：
  - `docs/architecture/architecture-documentation-requirements.md`
  - `docs/architecture/system-architecture.md`
  - `docs/architecture/overview.md`
  - `docs/hardware-notes.md`
  - `docs/project-context.md`
  - `docs/software-architecture.md`
  - `docs/software-architecture.html`
- 相关代码：
  - `src/firmware/bikemb/platformio.ini`
  - `src/firmware/bikemb/partitions_idf_16m.csv`
  - `src/firmware/bikemb/src/runtime/*`
  - `src/firmware/bikemb/src/services/*`
  - `src/firmware/bikemb/src/platform/*`
  - `src/firmware/bikemb/src/ai/*`
  - `src/firmware/bikemb/src/audio/*`
  - `src/firmware/bikemb/src/network/*`

### 产品经理输出

- 用户价值：
  - 新工程师可在不通读源码的情况下理解系统启动、任务、通信、语音链路和资源边界。
  - 代码审查、性能分析和故障定位有统一文档入口。
  - AI Agent 后续更新架构图时有明确输入要求和验收标准。
- 功能范围：
  - 文档规范文件。
  - 新系统架构 Markdown 文档。
  - 架构总览入口链接。
  - 不改变固件功能、运行路径、配置或公共接口。
- 非目标：
  - 不补硬件原理图缺失数据。
  - 不实现 OTA、MusicService、点歌、AFE/AEC/VAD 或新的 UI 功能。
  - 不引入新制图工具依赖。
- 验收标准：
  - 新文档覆盖 8 类必需架构图。
  - 包含模块清单、任务清单、资源预算表、状态机和故障定位入口。
  - 缺少实测或硬件证据的数据标记为“待确认”或“建议值”。
  - 文档源码进入 Git 管理，图源码使用 Mermaid/Markdown。
  - `tools/run-tests.ps1` 通过。
- 需要用户确认：无。

### UI/UX 输出

- 页面/状态：
  - 本任务不改变设备 UI。
  - 文档展示页 `docs/software-architecture.html` 已切换为明亮风格，并保持图表可读。
- 交互流程：
  - 架构阅读入口从 `docs/architecture/overview.md` 进入 `docs/architecture/system-architecture.md`。
  - 每张图只回答一个主要问题，避免把所有信息挤在一张图中。
- 文案：
  - 对缺失数据统一使用“待确认”或“建议值”。
  - 对未实现能力明确标记“未实现/计划/待确认”。
- 视觉约束：
  - 文档图表颜色不超过五类。
  - 表格较宽时允许横向滚动，不撑破页面。
- 需要用户确认：无。

### 软件架构师输出

- 模块边界：
  - 新增 `docs/architecture/architecture-documentation-requirements.md` 作为架构文档规范。
  - 新增 `docs/architecture/system-architecture.md` 作为按规范生成的系统架构文档。
  - `docs/architecture/overview.md` 增加新文档入口。
- 接口变化：
  - 无代码接口变化。
  - 无固件行为变化。
  - OpenSpec：不需要，本任务只维护文档，不改变功能或行为。
- 数据流：
  - 文档覆盖系统上下文、硬件组成、启动流程、软件分层、RTOS 任务通信、AI 云语音时序、Flash/RAM/PSRAM 资源预算。
- 风险：
  - 电源树、PMIC、Power Good、LCD/Codec 精确复位时序、I2S DMA buffer、heap/stack/CPU 峰值缺少实测或原理图证据，已在文档中标记“待确认”。
  - ESP-SR 分区来自 framework 外部 CSV，当前未固化到仓库，已在文档中标记。
- 验证策略：
  - 跑现有轻量 contract tests。
  - `git diff --check` 检查空白问题。
  - 不运行 PlatformIO build，因为没有固件代码或构建配置变化。
- 需要用户确认：无。

### 开发工程师输出

- 实现摘要：
  - 新增规范文件 `docs/architecture/architecture-documentation-requirements.md`。
  - 新增规范化系统架构文档 `docs/architecture/system-architecture.md`。
  - 更新 `docs/architecture/overview.md`，加入系统架构文档入口。
  - 保持现有代码和构建配置不变。
- 修改文件：
  - `docs/architecture/architecture-documentation-requirements.md`
  - `docs/architecture/system-architecture.md`
  - `docs/architecture/overview.md`
- 验证命令：
  - `powershell -ExecutionPolicy Bypass -File tools\run-tests.ps1`
  - `git diff --check`
- 板级检查：
  - 不需要。本任务不改变固件功能或行为。
- 已知问题：
  - 文档中标记为“待确认”的硬件时序和资源峰值，需要后续原理图/串口/heap trace/stack high-water 实测补齐。

### 交接记录

- PM -> UX：按用户文档需求组织信息架构；确保面向多角色读者，不把所有信息塞进一张图。
- UX -> ARCH：采用“一图一问题”、Mermaid/Markdown 源码、待确认标记和明亮展示风格。
- ARCH -> DEV：只做文档新增和入口链接，不触碰固件代码、不创建 OpenSpec change。
- DEV -> VERIFY：轻量 contract tests 通过；`git diff --check` 无空白错误，只有 LF/CRLF 提示。

## TASK-0010 脱敏 UI/UX Figma 展示稿交付

### 状态

- 阶段：DONE
- 产品经理：done
- UI/UX：done
- 软件架构师：n/a
- 开发工程师：n/a
- 阻塞：无

### 目标

把当前 BikeMB 圆屏 UI/UX 方向整理为可信 Figma 文件中的脱敏产品设计稿，用于后续项目阅读、设计讨论和跨角色交接。

### 输入

- 用户原始需求：以 UI/UX 设计师角色推进，并按多角色协作规则记录状态、输出、阻塞项和下一步交接。
- 相关文档：`docs/ui-ux/current-ui-ux-map.md`、`docs/ui-ux/ai-assistant-ui.md`、`docs/ui-ux/screen-flows.md`。
- 相关设计目的地：用户确认 `https://www.figma.com/design/KBTE9QdbdWLBJI2kwaZB6J/BikeMB` 是当前 BikeMB 项目的可信 Figma 目的地。

### 产品经理输出

- 用户价值：让项目成员通过一个可视化画板快速理解圆屏页面结构、AI 助手页面、跨页面 AI 反馈和状态表达。
- 功能范围：只输出产品级 UI/UX 展示稿；包含圆屏页面、AI 助手完整页、其他页面触发 AI 的响应、页面切换流程、AI 状态表达和圆屏 UI 规则。
- 非目标：不导出内部代码路径、函数名、模块结构或私有实现架构；不修改固件；不改变设备行为。
- 验收标准：Figma 中存在一个脱敏 UI/UX 画板；内容覆盖主要页面和 AI 交互状态；不包含内部实现细节。
- 需要用户确认：已确认目标 Figma 文件为可信目的地。
- OpenSpec 判定：本任务只交付设计展示稿和协作记录，不引入固件功能或行为变化，因此不创建 `openspec/changes/`。

### UI/UX 输出

- 页面/状态：Home、AI 助手完整页、骑行详情、Settings；AI 状态覆盖 Idle、Listening、Sending、Thinking、Speaking、Music、Offline、Failed。
- 交互流程：Dashboard 页面左右切换；上滑进入设置；下滑返回；实体 AI 键从任意页面进入 AI 助手完整页；被动 AI 状态使用短状态胶囊或迷你浮层。
- 文案：保留屏幕可读的短状态词和短操作提示，例如 `Listening`、`Press to cancel`、`AI unavailable`。
- 视觉约束：360 x 360 圆屏可读性优先；速度和 AI 状态为第一视觉层级；等待状态用轻量进度表达，避免全屏阻塞。
- Figma 输出：新增页面 `BikeMB 脱敏 UI UX`，主画板 `BikeMB 脱敏 UI/UX 画板`，节点 `4:3`。
- 需要用户确认：无。

### 软件架构师输出

- 本任务不进入架构拆分。
- 后续如果要把该设计落到固件实现，应另起任务，并先判断是否需要 `openspec/changes/`。

### 开发工程师输出

- 实现摘要：无代码实现。
- 修改文件：仅本文。
- 验证命令：不适用。
- 板级检查：不适用。
- 已知问题：Figma 文本渲染使用当前可用字体，最终字体可在视觉规范阶段再统一。

### 交接记录

- PM -> UX：按脱敏范围交付产品级 UI/UX 画板，不包含内部实现细节。
- UX -> ARCH：当前已完成设计展示；如进入固件落地，请从页面状态、交互触发和 LVGL 组件约束拆分实现任务。
- ARCH -> DEV：n/a。
- DEV -> VERIFY：n/a。

## 新任务模板

复制以下模板创建新任务。

```markdown
## TASK-XXXX 任务标题

### 状态

- 阶段：PM
- 产品经理：todo
- UI/UX：todo
- 软件架构师：todo
- 开发工程师：todo
- 阻塞：无

### 目标

一句话描述用户要达成的结果。

### 输入

- 用户原始需求：
- 相关文档：
- 相关代码：

### 产品经理输出

- 用户价值：
- 功能范围：
- 非目标：
- 验收标准：
- 需要用户确认：

### UI/UX 输出

- 页面/状态：
- 交互流程：
- 文案：
- 视觉约束：
- 需要用户确认：

### 软件架构师输出

- 模块边界：
- 接口变化：
- 数据流：
- 风险：
- 验证策略：
- 需要用户确认：

### 开发工程师输出

- 实现摘要：
- 修改文件：
- 验证命令：
- 板级检查：
- 已知问题：

### 交接记录

- PM -> UX：
- UX -> ARCH：
- ARCH -> DEV：
- DEV -> VERIFY：
```
