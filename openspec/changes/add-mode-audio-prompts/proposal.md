# Change: add mode audio prompts

## Goal

Add disabled-by-default pre-recorded mode prompts so tapping the home page mode chip can announce the selected mode through the verified ES8311 speaker path.

## Scope

- Generate local zh-CN male prompt assets for ECO, TRAIL, and BOOST using Windows TTS.
- Add a small BikeMB audio prompt layer for PCM playback.
- Add a dashboard mode-change callback and route it from `main.cpp` to the prompt layer.
- Keep the feature behind `BIKE_MB_ENABLE_AUDIO_PROMPTS`.

## Non-goals

- No cloud TTS.
- No real-time on-device TTS.
- No voice-recognition changes.
- No concurrent speech recognition and playback mixing in this change.
