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
- [ ] Add HTTPS MP3 stream spike; select a decoder only after license, RAM, CPU, Arduino 3.x, and custom PCM sink checks.
- [ ] Add `MusicService` for preset and private user URL playback.
- [ ] Add cancel/stop integration across HTTP, audio capture, TTS, and music.
- [ ] Record the AI-disabled 30-second steady dashboard baseline, then measure only each milestone's implemented phases.
- [ ] Before enabling AI by default, measure internal heap, largest block, PSRAM, task stacks, UI loop work, underruns, and end-to-end latency for boot, capture, TLS, TTS, and music peaks.
- [ ] Keep AI disabled by default until all fallback and resource checks pass.

## Deferred beyond V1

- [ ] Evaluate streaming STT/TTS if the 8-second start-of-reply target cannot be met.
- [ ] Add `MusicCatalogProvider` and voice point-song flow.
- [ ] Add ride-context read-only summaries for V2.
- [ ] Add allowlisted device commands for V3.
- [ ] Replace compile-time secrets with a production provisioning and credential-rotation design.
