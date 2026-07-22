# Tasks

## Documentation baseline

- [x] Align OpenSpec with `docs/product/ai-assistant.md` and confirmed product decisions.
- [x] Define AI Assistant, provider, audio session, music, UI, and Wi-Fi boundaries.
- [x] Define V1 state machines, cancellation, timeout, and stale-request behavior.
- [x] Define credential handling and privacy requirements.
- [x] Define provisional memory, task stack, latency, and measurement budgets.
- [x] Record the architecture decision in ADR-0002.

## Implementation sequence

- [x] 核对 V2 原理图并记录 `Key1/BOOT = GPIO0`、低电平有效、板载 `10 kΩ` 上拉和 ROM 下载模式风险。
- [ ] 上板确认 3000 ms 启动保护、50 ms 释放解锁、30 ms 消抖和按住 BOOT 复位后的恢复行为。
- [x] Add host-testable `BikeMbAiState` reducer with request ID, cancel, and timeout tests.
- [x] Add tracked non-secret AI config and ignored `ai_secrets.local.h` template workflow.
- [x] Add `AiButton` input events using a board-configured GPIO and debounce tests.
- [x] Add `AiAssistant` command queue and immutable snapshot with mock providers.
- [x] Add a separate cloud worker and prove a blocked mock provider cannot block cancel or UI snapshot updates.
- [x] Consolidate ES7210, ES8311, and I2S0 ownership into `AudioSession` while keeping existing test environments mutually exclusive.
- [x] Migrate Audio Self Test to `AudioSession` and re-verify speaker tone and mic RMS.
- [x] Migrate Audio Prompts to `AudioSession` and re-verify asynchronous latest-request behavior.
- [x] Keep Voice Commands compile-time exclusive until it has its own `AudioSession` migration test.
- [x] Validate 10-second 16 kHz mono capture into a bounded PSRAM clip.
- [x] Add asynchronous `WifiService`; verify boot and dashboard do not wait for Wi-Fi.
- [x] Add `qwen3-asr-flash` adapter spike using streamed Base64 WAV JSON without duplicating the full clip.
- [x] Add DeepSeek adapter with response-length limits and redacted logs.
- [x] Add `cosyvoice-v3-flash` HTTP/SSE adapter with chunked 16 kHz mono PCM playback.
- [x] Add dashboard AI status indicator and dedicated AI page using snapshots only.
- [x] Route the AI recording key to the dedicated AI page before recording starts.
- [x] Add first board voice-assistant mock build with Wi-Fi, `AudioSession` capture, and local audible reply feedback.
- [x] Add Bailian-only cloud voice build using Qwen ASR, Qwen Chat, and CosyVoice TTS.
- [ ] Complete and accept the ADR-0004 ESP-IDF dual-core migration gate.
  - [x] Code-stage 1: `app_main()` starts `bike_runtime` on Core 0 and `bike_ui` on Core 1; AI Assistant, Cloud Worker, and Wi-Fi Worker are pinned to Core 0.
  - [x] Code-stage 1: BOOT/AI key page navigation is routed through runtime event queue to the Core 1 UI owner.
  - [x] Build-stage 1: `esp32-s3-touch-lcd-1-85c-idf` builds successfully with the local `esp_lcd` compiler workaround.
  - [x] Board-stage 1: `esp32-s3-touch-lcd-1-85c-idf` boots with a 4MB factory app partition after ESP-IDF Wi-Fi growth exceeded the default 1MB app partition.
  - [x] Board-stage 1: ESP-IDF `WifiService` uses `esp_wifi`/`esp_netif`/`esp_event`/`nvs_flash`, stays on Core 0, and reaches online state without directly touching LVGL.
  - [x] Board-stage 1: CST816 touch controller initializes under the Core 1 LVGL owner, and touch gesture logging is available for board acceptance.
  - [x] Board-stage 1: User confirmed touch interaction is normal on the ESP-IDF build.
  - [x] Board-stage 1: AI/BOOT key poll diagnostics are available and show the poll task running after release-to-arm.
  - [x] Board-stage 1: User confirmed the AI/BOOT key is normal; no additional raw press/release capture is required for this round.
  - [x] Board-stage 2: `esp32-s3-touch-lcd-1-85c-idf-audio-session-test` boots under ESP-IDF and reaches `BikeMBAudioSession: session ready` without WDT or panic.
  - [ ] Board-stage: verify boot, touch, display, recording, TTS, cancellation, and resource baselines before accepting the gate.
- [ ] Add an isolated HTTPS MP3 stream spike; select a decoder only after license, RAM, CPU, ESP-IDF, and custom PCM sink checks. This spike must not enter the product runtime before the migration gate closes.
- [ ] After the ADR-0004 gate closes, add `MusicService` for preset and private user URL playback.
- [ ] Add cancel/stop integration across HTTP, audio capture, TTS, and music.
- [ ] Record the AI-disabled 30-second steady dashboard baseline, then measure only each milestone's implemented phases.
- [ ] Before enabling AI by default, measure internal heap, largest block, PSRAM, task stacks, UI loop work, underruns, and end-to-end latency for boot, capture, TLS, TTS, and music peaks.
- [ ] Keep AI disabled by default until all fallback and resource checks pass.

## Deferred beyond V1

- [ ] Evaluate streaming STT/TTS if the 8-second start-of-reply target cannot be met.
- [ ] After the ADR-0004 gate closes, add `MusicCatalogProvider` and voice point-song flow.
- [ ] Add ride-context read-only summaries for V2.
- [ ] Add allowlisted device commands for V3.
- [ ] Replace compile-time secrets with a production provisioning and credential-rotation design.
