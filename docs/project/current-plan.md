# 当前计划

本文记录当前短期执行计划。正式功能或行为变更仍以 `openspec/changes/` 为准。

## 当前目标

- 稳定 `src/firmware/bikemb` 的云端 AI 语音助手第一版。
- 保持核心码表 UI、页面切换、提示音、麦克风采集和 TTS 播放在连续交互中稳定。
- 将已确认的交互行为同步到 `openspec/changes/` 和 `docs/ui-ux/`。

## 下一步

1. 用户上板验证：按下实体录音/AI 键后应立即跳转到 `AiAssistant` 页面，并进入录音流程。
2. 连续测试 5 到 10 轮短语音问答，观察是否仍有沙沙声、无声或 `pcm missing`。
3. 若语音助手稳定，继续推进 HTTPS MP3 音频流 spike；选型前先检查许可证、RAM、CPU、Arduino 3.x 兼容性和自定义 PCM sink。

## 验证入口

- 文档结构：确认 `docs/product/`、`docs/architecture/`、`docs/project/` 均存在。
- 契约测试：`python tools\tests\test_dashboard_ai_ui_contract.py`
- AI 框架测试：`python tools\tests\test_ai_framework.py`
- 音频自检契约：`python tools\tests\test_audio_self_test_contract.py`
- 固件构建：`python -X utf8 -m platformio run -e esp32-s3-touch-lcd-1-85c-ai-voice-cloud-test`
- 固件烧录：`python -X utf8 -m platformio run -e esp32-s3-touch-lcd-1-85c-ai-voice-cloud-test -t upload`
