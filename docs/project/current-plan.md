# 当前计划

本文记录当前短期执行计划。正式功能或行为变更仍以 `openspec/changes/` 为准。

## 当前目标

- 继续 ESP-IDF 双核迁移，不开发 `MusicService` 或点歌。
- 已完成双核第一轮和 AudioSession ESP-IDF codec/I2S 初始化验收。
- 下一步补齐 ESP-IDF 录音、TTS 播放、取消和完整云 transport 回归，关闭 ADR-0004 前仍保持音乐功能冻结。

## 下一步

1. 等 USB 串口恢复后，烧录 `esp32-s3-touch-lcd-1-85c-idf-ai-voice-cloud-test`，只验证 ASR/Chat 日志和 UI/runtime 稳定，不验证蜂鸣或 TTS 播放。
2. 迁移 ESP-IDF CosyVoice/TTS 播放和取消路径，迁移前不主动发声。
3. 记录 ESP-IDF 语音闭环的 heap、PSRAM、task stack high-water mark、UI 延迟和 audio underrun 基线，再请求架构师关闭 ADR-0004。

## 最新验收记录

- 2026-07-22：`esp32-s3-touch-lcd-1-85c-idf` 构建成功，RAM `16.1%`，Flash `5.6%`。
- 2026-07-22：已烧录到 `COM5`，串口确认 `app_main()` 进入 BikeMB ESP-IDF runtime。
- 2026-07-22：串口确认 `bike_runtime core=0 owns_lvgl=0`，`bike_ui core=1 owns_lvgl=1`，`bikemb_ai`、`bikemb_cloud`、`bikemb_wifi`、`ai_button_poll` 均为 Core 0 且 `owns_lvgl=0`。
- 2026-07-22：30 秒诊断无 WDT、panic 或重复重启；稳定值约为 internal heap free `225587`、largest `147456`、PSRAM free `8385936`、PSRAM largest `8257536`、runtime stack HWM `2000` words、UI stack HWM `5504` words。
- 2026-07-22：UI 10 秒窗口诊断显示稳态 `render_ms=1-2`，`handler_max_us≈20744-20803`，`handler_avg_us≈561-570`；首屏初始化窗口峰值约 `109036 us`。
- 2026-07-22：按住 BOOT/AI 键复位到 `LVGL UI service ready` 未出现 `ai capture` 日志，确认启动保护前半段不误触发。
- 2026-07-22：ESP-IDF Wi-Fi STA 已接入真实连接路径；`bikemb_wifi` 仍在 Core 0 且不拥有 LVGL，串口确认 `offline`、`connect start`、`online` 状态流转。
- 2026-07-22：ESP-IDF 默认 1MB factory app 分区不足以容纳 Wi-Fi 迁移后的固件；新增 `partitions_idf_16m.csv`，factory app 扩为 `4M`，串口确认 partition table `factory ... 00400000` 且不再出现 `Image length ... doesn't fit`。
- 2026-07-22：ESP-IDF Wi-Fi 版构建成功并烧录到 `COM5`，资源占用 RAM `22.5%`，Flash `8.8%`；45 秒串口观察无 WDT、panic 或重复重启。
- 2026-07-22：Wi-Fi 迁移后稳态诊断约为 internal heap free `116299`、largest `40960`、PSRAM free `8385820`、PSRAM largest `8257536`、runtime stack HWM `1980` words、UI stack HWM `5424` words；UI 稳态 `render_ms=1`，`handler_avg_us≈580-699`。
- 2026-07-23：新增 UI owner 侧触摸验收日志：CST816 初始化成功打印 `CST816 touch ready`，滑动手势将打印 `[BikeMB][touch] gesture ...`。
- 2026-07-23：`esp32-s3-touch-lcd-1-85c-idf` 重新构建并烧录到 `COM5`，资源占用 RAM `22.5%`，Flash `8.8%`；启动串口确认 `CST816 touch ready`。
- 2026-07-23：连续 90 秒加后续 45 秒串口观察无 WDT、panic 或重复重启；Wi-Fi 在线，UI 稳态 `render_ms=1-2`，runtime/UI 诊断持续输出。
- 2026-07-23：用户确认触摸交互正常；CST816 触摸链路本轮按板级观察通过。
- 2026-07-23：新增 AI/BOOT 键板级验收日志：初始化参数、10 秒一次 `ai button diag raw=...`、raw pressed/released 和去抖后的 pressed/released 事件。
- 2026-07-23：`esp32-s3-touch-lcd-1-85c-idf` 重新构建并烧录成功，资源占用 RAM `22.5%`，Flash `8.8%`。
- 2026-07-23：串口确认 `ai button diag raw=0 armed=1 stable=0 candidate=0` 周期输出，说明 AI button poll 已运行且释放解锁完成；后续 60 秒窗口未观察到 raw pressed/released，BOOT/AI 物理触发链路仍需复测或核对实物 GPIO。
- 2026-07-23：用户确认按键是好的，本轮不再单独复测 raw pressed/released。
- 2026-07-23：新增 `esp32-s3-touch-lcd-1-85c-idf-audio-session-test`，使用 ESP-IDF `driver/i2s_std.h` 初始化 I2S0、ES8311 和 ES7210，不启用 AI/云/自检/音乐。
- 2026-07-23：`esp32-s3-touch-lcd-1-85c-idf-audio-session-test` 构建成功并烧录到 `COM5`，资源占用 RAM `16.0%`，Flash `5.7%`。
- 2026-07-23：板级启动日志确认 `audio_session core=0 owns_lvgl=0`，`BikeMBAudioSession: session enabled` 和 `BikeMBAudioSession: session ready`；随后 25 秒串口观察无 WDT、panic 或重复重启。
- 2026-07-23：AudioSession ESP-IDF 初始化后资源约为 internal heap free `244407`、largest `167936`、PSRAM free `8384900`、PSRAM largest `8257536`、runtime stack HWM `2012-3356` words、UI stack HWM `5504` words；UI 稳态 `render_ms=1-9`，启动首个窗口 `handler_max_us≈110635`。
- 2026-07-24：跳过 ESP-IDF 蜂鸣器自检，不再继续发声测试；相关未提交改动已撤销。
- 2026-07-24：新增无声 `esp32-s3-touch-lcd-1-85c-idf-ai-voice-cloud-test` 构建环境，启用 ESP-IDF AI Assistant、Wi-Fi、AudioSession 和真实 Qwen ASR/Qwen Chat transport；CosyVoice/TTS 播放仍显式返回 `idf cosyvoice playback unavailable`。
- 2026-07-24：`esp32-s3-touch-lcd-1-85c-idf-ai-voice-cloud-test` 构建成功，资源占用 RAM `23.3%`，Flash `10.2%`；当前 Windows 未枚举到串口，尚未烧录上板。

## 验证入口

- 文档结构：确认 `docs/product/`、`docs/architecture/`、`docs/project/` 均存在。
- 双核运行时契约：`python tools\tests\test_runtime_contract.py`
- 双核规划契约：`python tools\tests\test_runtime_plan_contract.py`
- Avinox/UI 契约：`python tools\tests\test_avinox_ui_contract.py`
- ESP-IDF Wi-Fi 契约：`python tools\tests\test_idf_wifi_service_contract.py`
- ESP-IDF AudioSession 契约：`python tools\tests\test_idf_audio_session_contract.py`
- ESP-IDF Cloud Transport 契约：`python tools\tests\test_idf_cloud_transport_contract.py`
- 契约测试：`python tools\tests\test_dashboard_ai_ui_contract.py`
- AI 框架测试：`python tools\tests\test_ai_framework.py`
- 音频自检契约：`python tools\tests\test_audio_self_test_contract.py`
- 固件构建：`python -X utf8 -m platformio run -e esp32-s3-touch-lcd-1-85c-ai-voice-cloud-test`
- 固件烧录：`python -X utf8 -m platformio run -e esp32-s3-touch-lcd-1-85c-ai-voice-cloud-test -t upload`
- ESP-IDF 双核构建：`python -X utf8 -m platformio run -e esp32-s3-touch-lcd-1-85c-idf`
- ESP-IDF 双核烧录：`python -X utf8 -m platformio run -e esp32-s3-touch-lcd-1-85c-idf -t upload`
