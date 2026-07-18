# Design: cloud AI speaker mode

## Summary

Cloud AI assistant mode is a future experimental BikeMB capability. V1 lets the user hold the reused BOOT/AI physical button after its startup guard, ask a short question, release the button to submit it, receive a spoken answer, and optionally play a configured cloud audio stream through the onboard speaker.

The feature must stay separate from the first-stage bicycle computer MVP. Core ride display remains local and available when the AI service, Wi-Fi, or music stream is unavailable.

## Product behavior

- The user starts one interaction by pressing and holding the reused `Key1/BOOT (GPIO0)` button after its startup guard and submits it by releasing the button.
- BikeMB records only while that button is held, with a maximum duration of 10 seconds.
- The V1 AI answer covers short general questions. Ride-context questions and device-control commands are deferred.
- Music playback uses a preset HTTPS stream URL or a user URL from private configuration. V1 does not support voice point-song requests.
- The dashboard exposes a small state indicator and a dedicated AI page exposes detailed state, Wi-Fi status, errors, cancel, and stop controls.
- Internal/public states are `Disabled`, `Idle`, `Recording`, `Recognizing`, `Thinking`, `Synthesizing`, `Speaking`, `ConnectingMusic`, `MusicPlaying`, and `Error`; the dashboard may omit `Disabled`.
- Cancel or replacement must be available throughout every active AI and music stage.

## System design

The later implementation should keep these responsibilities separate:

- `ai_button`: applies the BOOT/AI startup guard and debounces runtime press/release events.
- `ai_assistant`: owns user-triggered interaction state, deadlines, cancellation, request IDs, and the public snapshot.
- `cloud_worker`: executes one potentially blocking STT, DeepSeek, or TTS request and returns results tagged with request ID; it does not own state.
- `wifi_service`: keeps Wi-Fi connected while AI is enabled and publishes connection state without blocking boot.
- `audio_session`: exclusively owns I2S0, ES7210, and ES8311 and arbitrates microphone capture, AI speech, prompt, and music playback.
- `provider`: independently adapts STT, DeepSeek, and TTS protocols without exposing vendor JSON to the state machine.
- `music_service`: supplies configured stream descriptors and later hosts a replaceable music catalog resolver.
- `ai_ui`: maps the immutable assistant snapshot to the dashboard indicator and dedicated AI page.

The intended flow is:

1. User presses the armed BOOT/AI button.
2. `BikeMbAiAssistant_OnButtonPressed()` stops music if necessary and requests microphone ownership.
3. `audio_session` records 16 kHz, 16-bit, mono PCM into a bounded PSRAM clip.
4. User releases the button and the clip is submitted to the STT provider over HTTPS.
5. The STT text is sent to DeepSeek and its short answer is sent to the TTS provider.
6. TTS PCM/WAV chunks are forwarded to `audio_session` without buffering the complete answer in Flash.
7. The UI reads `BikeMbAiSnapshot` and returns to `Idle` or `Error` after playback.

Every interaction has a monotonically increasing request ID and one 15-second deadline starting at button release. Cancel invalidates the request ID, aborts network/audio work where possible, and causes late callbacks to be ignored.

A recording shorter than 300 ms is treated as cancel. Pressing the AI button while cloud work, TTS, or music is active invalidates the old request and starts a new recording when Wi-Fi is available. Reaching the 10-second recording limit enters Error and the subsequent release does not submit the clipped audio.

The board input contract is `GPIO0`, active low, with the populated `R27 10 kΩ` pull-up. The input emits no AI events during the first 3000 ms after power-on. After that guard, it must observe a continuous 50 ms released level before arming, then apply 30 ms debounce to runtime edges. Holding BOOT during power-on or reset can still select the ESP32-S3 ROM download mode; firmware cannot mask the strap sampling.

## Future interfaces

These names define the intended firmware contract. They are not implemented by this documentation change.

| Interface | Purpose |
| --- | --- |
| `BikeMbAiAssistant_OnButtonPressed()` | Starts bounded recording after Wi-Fi availability is confirmed. |
| `BikeMbAiAssistant_OnButtonReleased()` | Stops recording and starts STT. |
| `BikeMbAiAssistant_Cancel()` | Cancels recording, cloud wait, spoken response, or music playback. |
| `BikeMbAiAssistant_PlayMusicUrl(const char *url)` | Starts playback of a validated HTTPS MP3 stream. |
| `BikeMbAiAssistant_GetSnapshot(...)` | Returns immutable state for UI without exposing provider details. |
| `BikeMbAiState` | Tracks the V1 visible states defined above. |

V1 configuration decisions:

- Wi-Fi credentials, provider tokens, and user stream URL come from Git-ignored `ai_secrets.local.h`.
- Tracked configuration may contain feature flags, provider base URLs, model names, 10-second recording limit, and timeout constants.
- AI-enabled builds keep Wi-Fi connected in the background; disabled builds do not start Wi-Fi for this feature.
- V1 defaults to Alibaba Cloud Model Studio `qwen3-asr-flash` Base64 WAV REST for STT and `cosyvoice-v3-flash` HTTP/SSE with 16 kHz mono PCM for TTS, with DeepSeek for LLM completion.
- V1 stream playback accepts HTTPS MP3 direct URLs up to 128 kbps. Future point-song support resolves a query to the same stream descriptor.

Real Wi-Fi passwords, API tokens, and personal identifiers must never be stored in tracked files.

## Failure and safety behavior

- If Wi-Fi is disconnected, the UI should show an AI unavailable state and the speaker may play a short local fixed prompt.
- If cloud AI or TTS fails, BikeMB should return to a usable local dashboard state.
- If music streaming fails, playback should stop without affecting page switching or ride metrics.
- AI recording and music playback must be cancelable.
- AI and music tasks must not block LVGL refresh, sensor sampling, battery display, or core page navigation.
- Background tasks must never call LVGL. The UI polls or copies an immutable snapshot.
- The feature should remain disabled by default until the network, provider, and audio session behavior are validated.

## Resource budget

- Voice clip: at most 384 KiB PSRAM.
- Total AI incremental PSRAM peak: at most 1.5 MiB.
- Total AI incremental internal DRAM peak: at most 256 KiB.
- AI control stack: at most 12 KiB; cloud worker stack: at most 12 KiB; audio task stack: at most 8 KiB.
- STT text: at most 512 UTF-8 bytes; LLM answer: at most 1024 UTF-8 bytes.
- Dashboard snapshot work: at most 1 ms per loop iteration.

These are implementation limits, not measured current usage. The baseline is the same dashboard build with AI disabled after 30 seconds of steady running. Each milestone measures only the phases it implements; the full pre-enable test reports free internal heap, largest free block, free PSRAM, task stack high-water marks, and audio underrun count for boot, capture, TLS, TTS, and music peaks.

## Implementation boundary

This documentation change does not implement firmware. MP3 decoder selection remains an implementation spike because candidate libraries differ in license, I2S ownership, RAM use, and Arduino 3.x compatibility. A production credential provisioning mechanism and music catalog provider are also deferred beyond V1.
