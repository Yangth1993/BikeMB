# Mode Audio Prompts Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add disabled-by-default pre-recorded mode prompts for the existing BikeMB firmware.

**Architecture:** Generate short local Windows TTS WAV files, convert them to 16 kHz 16-bit mono PCM arrays, and play them through a small `audio_prompts` layer. UI mode changes emit a callback; `main.cpp` connects that callback to the audio prompt layer when the feature flag is enabled.

**Tech Stack:** PlatformIO Arduino firmware, LVGL UI, ES8311/I2S speaker output, PowerShell/.NET local TTS generation, Python contract tests.

## Global Constraints

- Keep the feature in `firmware/bikemb`; do not create a long-lived second firmware project.
- Keep prompt playback behind `BIKE_MB_ENABLE_AUDIO_PROMPTS`, default off.
- Use local `Microsoft Kangkang` zh-CN male TTS when available.
- Do not add cloud TTS or real-time TTS.
- Do not change the direct voice command path in this task.

---

### Task 1: Contract and OpenSpec

**Files:**
- Create: `tools/tests/test_audio_prompts_contract.py`
- Create: `openspec/changes/add-mode-audio-prompts/proposal.md`
- Create: `openspec/changes/add-mode-audio-prompts/tasks.md`

- [ ] Write a failing contract test for the prompt layer, generator script, explicit opt-in env, and mode-change callback.
- [ ] Run `py -X utf8 tools/tests/test_audio_prompts_contract.py` and verify it fails because the prompt layer is missing.

### Task 2: Local TTS Assets

**Files:**
- Create: `tools/generate-mode-prompts.ps1`
- Create: `firmware/bikemb/src/audio/audio_prompt_assets.h`
- Create: `firmware/bikemb/src/audio/audio_prompt_assets.cpp`

- [ ] Generate `ECO`, `TRAIL`, and `BOOST` WAV files with local Windows TTS voice `Microsoft Kangkang`.
- [ ] Convert WAV payloads to firmware PCM arrays.

### Task 3: Firmware Playback

**Files:**
- Create: `firmware/bikemb/src/audio/audio_prompts.h`
- Create: `firmware/bikemb/src/audio/audio_prompts.cpp`
- Modify: `firmware/bikemb/src/CMakeLists.txt`
- Modify: `firmware/bikemb/platformio.ini`
- Modify: `firmware/bikemb/src/main.cpp`

- [ ] Add `BikeMbAudioPrompts_Init()` and `BikeMbAudioPrompts_PlayMode(...)`.
- [ ] Add `esp32-s3-touch-lcd-1-85c-mode-prompts-test` env.
- [ ] Keep the default env unchanged.

### Task 4: UI Event Bridge

**Files:**
- Modify: `firmware/bikemb/src/app/dashboard_pages.h`
- Modify: `firmware/bikemb/src/app/dashboard_pages.c`
- Modify: `firmware/bikemb/src/app/dashboard_view_core.h`
- Modify: `firmware/bikemb/src/app/dashboard_view_core.c`
- Modify: `firmware/bikemb/src/app/dashboard_view.h`
- Modify: `firmware/bikemb/src/app/dashboard_view.cpp`
- Modify: `firmware/bikemb/src/app/dashboard_app.h`
- Modify: `firmware/bikemb/src/app/dashboard_app.cpp`

- [ ] Add a mode changed callback that passes the new mode index.
- [ ] Wire `main.cpp` to play the matching prompt.

### Task 5: Verification

- [ ] Run `py -X utf8 tools/tests/test_audio_prompts_contract.py`.
- [ ] Run `powershell -ExecutionPolicy Bypass -File tools/run-tests.ps1`.
- [ ] Run `pio run -s -d firmware/bikemb -e esp32-s3-touch-lcd-1-85c-mode-prompts-test`.
