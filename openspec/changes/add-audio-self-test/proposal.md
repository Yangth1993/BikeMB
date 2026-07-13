# Change: add audio self-test

## Goal

Add a disabled-by-default audio self-test path in the existing `firmware/bikemb` project so the Waveshare audio input/output chain can be verified before adding speech recognition.

## Scope

- Add a small BikeMB audio layer for ES8311/I2S output self-test and microphone level reporting.
- Keep the audio self-test behind `BIKE_MB_ENABLE_AUDIO_SELF_TEST`.
- Add shared dashboard page command functions for future voice-triggered next/previous page actions.

## Non-goals

- No cloud AI assistant.
- No real-time TTS.
- No full offline speech recognition yet.
- No separate long-lived PlatformIO project.
