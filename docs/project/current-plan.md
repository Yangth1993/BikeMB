# 当前计划

本文记录当前短期执行计划。正式功能或行为变更仍以 `openspec/changes/` 为准。

## 当前目标

- 完成 ESP-IDF 双核迁移验收第一轮，不开发 `MusicService` 或点歌。
- 先确认 `app_main()`、`bike_runtime` Core 0、`bike_ui` Core 1、LVGL 单 owner 和 AI/Cloud/Wi-Fi/AI button 非 LVGL owner。
- 在进入 AudioSession ESP-IDF codec/I2S 迁移前，保留清晰的板级证据和剩余风险。

## 下一步

1. 补齐 ESP-IDF 板级真实触摸手势日志确认；当前已确认 CST816 初始化成功，但尚未在串口窗口内观察到手势事件。
2. 补齐 BOOT/AI 键释放解锁后再触发的板级串口采集；本轮已验证按住上电不误触发，后半段此前按用户要求跳过。
3. 通过双核门禁后，再进入 AudioSession ESP-IDF codec/I2S 迁移；音乐服务和点歌继续冻结。

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
- 2026-07-23：串口窗口内尚未观察到 `[BikeMB][touch] gesture ...`，真实手势输入仍需人工滑动确认。

## 验证入口

- 文档结构：确认 `docs/product/`、`docs/architecture/`、`docs/project/` 均存在。
- 双核运行时契约：`python tools\tests\test_runtime_contract.py`
- 双核规划契约：`python tools\tests\test_runtime_plan_contract.py`
- Avinox/UI 契约：`python tools\tests\test_avinox_ui_contract.py`
- ESP-IDF Wi-Fi 契约：`python tools\tests\test_idf_wifi_service_contract.py`
- 契约测试：`python tools\tests\test_dashboard_ai_ui_contract.py`
- AI 框架测试：`python tools\tests\test_ai_framework.py`
- 音频自检契约：`python tools\tests\test_audio_self_test_contract.py`
- 固件构建：`python -X utf8 -m platformio run -e esp32-s3-touch-lcd-1-85c-ai-voice-cloud-test`
- 固件烧录：`python -X utf8 -m platformio run -e esp32-s3-touch-lcd-1-85c-ai-voice-cloud-test -t upload`
- ESP-IDF 双核构建：`python -X utf8 -m platformio run -e esp32-s3-touch-lcd-1-85c-idf`
- ESP-IDF 双核烧录：`python -X utf8 -m platformio run -e esp32-s3-touch-lcd-1-85c-idf -t upload`
