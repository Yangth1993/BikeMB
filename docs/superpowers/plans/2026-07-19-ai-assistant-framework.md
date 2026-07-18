# AI Assistant Framework Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the default-off, host-tested AI assistant control framework with guarded BOOT-key input, deterministic state transitions, request cancellation, immutable snapshots, and a separate mock cloud worker.

**Architecture:** `AiButton` converts GPIO0 levels into debounced press/release commands only after the startup guard. `AiAssistant` is the sole owner of the public state machine and accepts commands through a FreeRTOS queue; a separate `CloudWorker` executes mock blocking stages and returns request-tagged results. This milestone deliberately uses mock audio and cloud effects so state, cancellation, and scheduling can be verified before I2S and HTTPS are introduced.

**Tech Stack:** C++17, Arduino ESP32 / ESP-IDF compatibility gates, FreeRTOS queues and tasks, PlatformIO, Python contract tests, MinGW `g++` host tests.

## Global Constraints

- Keep `BIKE_MB_ENABLE_AI_ASSISTANT=0` in the default environment.
- Use `Key1/BOOT`, `GPIO0`, active low, with the populated `10 kΩ` pull-up.
- Ignore GPIO0 for the first `3000 ms` after startup, require `50 ms` continuously released before arming, then debounce runtime edges for `30 ms`.
- Holding BOOT during power-on or reset can still enter ESP32-S3 ROM download mode; firmware must not claim otherwise.
- Record only while the physical key is held; a recording under `300 ms` is canceled and the maximum is `10000 ms`.
- Use one `15000 ms` deadline beginning at button release for STT, LLM, and TTS combined.
- Display an error for at least `1500 ms`; late results with an old request ID must not change state.
- Keep STT text at or below `512` UTF-8 bytes and LLM answer at or below `1024` UTF-8 bytes.
- AI control stack must be at or below `12 KiB`; cloud worker stack must be at or below `12 KiB`.
- Background tasks must never call LVGL. UI access is limited to copying `BikeMbAiSnapshot`.
- Never log or commit Wi-Fi passwords, provider tokens, full private URLs, transcripts, or complete AI answers.
- Do not initialize I2S0, ES7210, or ES8311 in this milestone.
- Preserve default dashboard, audio self-test, mode prompts, voice-direct test, and ESP-IDF build behavior.

---

## File Map

| File | Responsibility |
| --- | --- |
| `src/firmware/bikemb/src/ai/ai_config.h` | Tracked non-secret constants and feature defaults. |
| `src/firmware/bikemb/include/ai_secrets.example.h` | Tracked credential shape with non-secret sentinel values. |
| `src/firmware/bikemb/src/ai/ai_types.h` | C-compatible public states, commands, effects, and snapshot. |
| `src/firmware/bikemb/src/input/ai_button_logic.h/.cpp` | Pure startup guard and debounce reducer. |
| `src/firmware/bikemb/src/input/ai_button.h/.cpp` | GPIO0 adapter and event callback. |
| `src/firmware/bikemb/src/ai/ai_state_machine.h/.cpp` | Pure AI state reducer and effect output. |
| `src/firmware/bikemb/src/ai/cloud_worker.h/.cpp` | Separate mock worker queue and blocking stage simulation. |
| `src/firmware/bikemb/src/ai/ai_assistant.h/.cpp` | Control queue/task, request ownership, worker result routing, immutable snapshot. |
| `src/firmware/bikemb/src/main.cpp` | Feature-gated initialization and short non-blocking button polling. |
| `src/firmware/bikemb/src/CMakeLists.txt` | Explicit ESP-IDF source registration. |
| `src/firmware/bikemb/platformio.ini` | Dedicated mock framework test environment; default stays disabled. |
| `tools/tests/ai_button_logic_test.cpp` | Native behavioral tests for startup guard and debounce. |
| `tools/tests/ai_state_machine_test.cpp` | Native transition, deadline, cancel, and stale-result tests. |
| `tools/tests/test_ai_framework.py` | Compiles/runs native tests and checks tracked configuration boundaries. |

### Task 1: Lock Configuration and Secret Boundaries

**Files:**
- Create: `src/firmware/bikemb/src/ai/ai_config.h`
- Create: `src/firmware/bikemb/include/ai_secrets.example.h`
- Modify: `.gitignore`
- Modify: `src/firmware/bikemb/platformio.ini`
- Test: `tools/tests/test_ai_framework.py`

**Interfaces:**
- Consumes: ADR-0003 hardware decision.
- Produces: `BIKE_MB_ENABLE_AI_ASSISTANT`, `BikeMbAiConfig::*` constants, and the private include contract used by later tasks.

- [ ] **Step 1: Write the failing configuration contract test**

Create `tools/tests/test_ai_framework.py` with:

```python
from pathlib import Path
import subprocess
import tempfile

from contract_helpers import REPO_ROOT


PROJECT_ROOT = REPO_ROOT
FIRMWARE_ROOT = REPO_ROOT / "src" / "firmware" / "bikemb"


def read_text(path):
    return Path(path).read_text(encoding="utf-8")


def test_configuration_contract():
    config = read_text(FIRMWARE_ROOT / "src" / "ai" / "ai_config.h")
    example = read_text(FIRMWARE_ROOT / "include" / "ai_secrets.example.h")
    gitignore = read_text(PROJECT_ROOT / ".gitignore")
    platformio = read_text(FIRMWARE_ROOT / "platformio.ini")

    assert "#define BIKE_MB_ENABLE_AI_ASSISTANT 0" in config
    assert "kButtonGpio = 0" in config
    assert "kStartupGuardMs = 3000" in config
    assert "kReleaseToArmMs = 50" in config
    assert "kDebounceMs = 30" in config
    assert "kMinRecordingMs = 300" in config
    assert "kMaxRecordingMs = 10000" in config
    assert "kCloudDeadlineMs = 15000" in config
    assert "kErrorDisplayMs = 1500" in config
    assert "BIKE_MB_AI_WIFI_PASSWORD" in example
    assert "BIKE_MB_AI_DEEPSEEK_TOKEN" in example
    assert "ai_secrets.local.h" in gitignore
    assert "[env:esp32-s3-touch-lcd-1-85c-ai-framework-test]" in platformio
    assert "-D BIKE_MB_ENABLE_AI_ASSISTANT=1" in platformio
    assert "-D BIKE_MB_AI_USE_MOCK_PROVIDERS=1" in platformio


def compile_and_run(name, sources):
    with tempfile.TemporaryDirectory(prefix="bikemb-ai-") as temp_dir:
        output = Path(temp_dir) / f"{name}.exe"
        command = ["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror"]
        command.extend(str(path) for path in sources)
        command.extend(["-o", str(output)])
        subprocess.run(command, check=True, cwd=PROJECT_ROOT)
        subprocess.run([str(output)], check=True, cwd=PROJECT_ROOT)


def test_native_ai_reducers():
    compile_and_run(
        "ai_button_logic_test",
        [
            FIRMWARE_ROOT / "src" / "input" / "ai_button_logic.cpp",
            PROJECT_ROOT / "tools" / "tests" / "ai_button_logic_test.cpp",
        ],
    )
    compile_and_run(
        "ai_state_machine_test",
        [
            FIRMWARE_ROOT / "src" / "ai" / "ai_state_machine.cpp",
            PROJECT_ROOT / "tools" / "tests" / "ai_state_machine_test.cpp",
        ],
    )


if __name__ == "__main__":
    test_configuration_contract()
    test_native_ai_reducers()
    print("PASS test_ai_framework")
```

- [ ] **Step 2: Run the test and verify the missing files fail**

Run: `python tools/tests/test_ai_framework.py`

Expected: FAIL while reading `src/firmware/bikemb/src/ai/ai_config.h`.

- [ ] **Step 3: Add the exact tracked configuration**

Create `src/firmware/bikemb/src/ai/ai_config.h`:

```cpp
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef BIKE_MB_ENABLE_AI_ASSISTANT
#define BIKE_MB_ENABLE_AI_ASSISTANT 0
#endif

#ifndef BIKE_MB_AI_USE_MOCK_PROVIDERS
#define BIKE_MB_AI_USE_MOCK_PROVIDERS 0
#endif

namespace BikeMbAiConfig {
constexpr uint8_t kButtonGpio = 0;
constexpr bool kButtonActiveLow = true;
constexpr uint32_t kStartupGuardMs = 3000;
constexpr uint32_t kReleaseToArmMs = 50;
constexpr uint32_t kDebounceMs = 30;
constexpr uint32_t kMinRecordingMs = 300;
constexpr uint32_t kMaxRecordingMs = 10000;
constexpr uint32_t kCloudDeadlineMs = 15000;
constexpr uint32_t kErrorDisplayMs = 1500;
constexpr size_t kMaxSttTextBytes = 512;
constexpr size_t kMaxAnswerBytes = 1024;
constexpr uint32_t kAssistantStackBytes = 12 * 1024;
constexpr uint32_t kCloudWorkerStackBytes = 12 * 1024;
}
```

Create `src/firmware/bikemb/include/ai_secrets.example.h`:

```cpp
#pragma once

#define BIKE_MB_AI_WIFI_SSID "CHANGE_ME_WIFI_SSID"
#define BIKE_MB_AI_WIFI_PASSWORD "CHANGE_ME_WIFI_PASSWORD"
#define BIKE_MB_AI_STT_TOKEN "CHANGE_ME_STT_TOKEN"
#define BIKE_MB_AI_DEEPSEEK_TOKEN "CHANGE_ME_DEEPSEEK_TOKEN"
#define BIKE_MB_AI_TTS_TOKEN "CHANGE_ME_TTS_TOKEN"
#define BIKE_MB_AI_DEFAULT_STREAM_URL "https://example.invalid/audio.mp3"
```

Append this line to `.gitignore`:

```gitignore
src/firmware/bikemb/include/ai_secrets.local.h
```

Append this environment to `src/firmware/bikemb/platformio.ini`:

```ini
[env:esp32-s3-touch-lcd-1-85c-ai-framework-test]
extends = env:esp32-s3-touch-lcd-1-85c
build_flags =
  ${env:esp32-s3-touch-lcd-1-85c.build_flags}
  -D BIKE_MB_ENABLE_AI_ASSISTANT=1
  -D BIKE_MB_AI_USE_MOCK_PROVIDERS=1
```

- [ ] **Step 4: Run only the configuration test**

Run: `python -c "import sys; sys.path.insert(0, 'tools/tests'); import test_ai_framework as t; t.test_configuration_contract()"`

Expected: PASS.

- [ ] **Step 5: Verify no real secret-shaped values were added**

Run:

```powershell
git diff --check
rg -n "sk-[A-Za-z0-9]{16,}|Bearer [A-Za-z0-9]{16,}" src/firmware/bikemb --glob "!ai_secrets.example.h"
```

Expected: `git diff --check` exits 0 and `rg` prints no matches (exit code 1 means the scan is clean).

- [ ] **Step 6: Commit the configuration boundary**

```bash
git add .gitignore src/firmware/bikemb/platformio.ini src/firmware/bikemb/include/ai_secrets.example.h src/firmware/bikemb/src/ai/ai_config.h tools/tests/test_ai_framework.py
git commit -m "feat: add AI assistant configuration boundary"
```

### Task 2: Implement the Guarded BOOT-Key Reducer

**Files:**
- Create: `src/firmware/bikemb/src/input/ai_button_logic.h`
- Create: `src/firmware/bikemb/src/input/ai_button_logic.cpp`
- Create: `tools/tests/ai_button_logic_test.cpp`

**Interfaces:**
- Consumes: `BikeMbAiConfig::kStartupGuardMs`, `kReleaseToArmMs`, and `kDebounceMs`.
- Produces: `BikeMbAiButtonLogic_Init(...)` and `BikeMbAiButtonLogic_Update(...)`; the hardware adapter in Task 4 depends on these exact signatures.

- [ ] **Step 1: Write the native reducer test**

Create `tools/tests/ai_button_logic_test.cpp`:

```cpp
#include <assert.h>

#include "../../src/firmware/bikemb/src/input/ai_button_logic.h"

int main() {
  BikeMbAiButtonLogic logic = {};
  BikeMbAiButtonLogic_Init(&logic);

  assert(BikeMbAiButtonLogic_Update(&logic, 0, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 2999, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3000, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3050, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(!logic.armed);

  assert(BikeMbAiButtonLogic_Update(&logic, 3060, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3109, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3110, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(logic.armed);

  assert(BikeMbAiButtonLogic_Update(&logic, 3120, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3149, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3150, true) == BIKE_MB_AI_BUTTON_EVENT_PRESSED);
  assert(BikeMbAiButtonLogic_Update(&logic, 3151, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3152, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3182, true) == BIKE_MB_AI_BUTTON_EVENT_NONE);

  assert(BikeMbAiButtonLogic_Update(&logic, 3200, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  assert(BikeMbAiButtonLogic_Update(&logic, 3230, false) == BIKE_MB_AI_BUTTON_EVENT_RELEASED);
  assert(BikeMbAiButtonLogic_Update(&logic, 3240, false) == BIKE_MB_AI_BUTTON_EVENT_NONE);
  return 0;
}
```

- [ ] **Step 2: Run the native test and verify the header is missing**

Run: `python tools/tests/test_ai_framework.py`

Expected: FAIL compiling `ai_button_logic_test.cpp` because `ai_button_logic.h` does not exist.

- [ ] **Step 3: Add the pure reducer declaration**

Create `src/firmware/bikemb/src/input/ai_button_logic.h`:

```cpp
#pragma once

#include <stdbool.h>
#include <stdint.h>

enum BikeMbAiButtonEvent {
  BIKE_MB_AI_BUTTON_EVENT_NONE = 0,
  BIKE_MB_AI_BUTTON_EVENT_PRESSED,
  BIKE_MB_AI_BUTTON_EVENT_RELEASED,
};

struct BikeMbAiButtonLogic {
  bool armed;
  bool releaseCandidateActive;
  bool stablePressed;
  bool candidatePressed;
  uint32_t releaseCandidateSinceMs;
  uint32_t edgeCandidateSinceMs;
};

void BikeMbAiButtonLogic_Init(BikeMbAiButtonLogic *logic);
BikeMbAiButtonEvent BikeMbAiButtonLogic_Update(
    BikeMbAiButtonLogic *logic,
    uint32_t nowMs,
    bool rawPressed);
```

- [ ] **Step 4: Add the startup guard and debounce implementation**

Create `src/firmware/bikemb/src/input/ai_button_logic.cpp`:

```cpp
#include "ai_button_logic.h"

#include "../ai/ai_config.h"

namespace {
bool elapsed(uint32_t nowMs, uint32_t sinceMs, uint32_t durationMs) {
  return static_cast<uint32_t>(nowMs - sinceMs) >= durationMs;
}
}

void BikeMbAiButtonLogic_Init(BikeMbAiButtonLogic *logic) {
  if (logic == nullptr) {
    return;
  }
  *logic = {};
}

BikeMbAiButtonEvent BikeMbAiButtonLogic_Update(
    BikeMbAiButtonLogic *logic,
    uint32_t nowMs,
    bool rawPressed) {
  if (logic == nullptr || nowMs < BikeMbAiConfig::kStartupGuardMs) {
    return BIKE_MB_AI_BUTTON_EVENT_NONE;
  }

  if (!logic->armed) {
    if (rawPressed) {
      logic->releaseCandidateActive = false;
      return BIKE_MB_AI_BUTTON_EVENT_NONE;
    }
    if (!logic->releaseCandidateActive) {
      logic->releaseCandidateActive = true;
      logic->releaseCandidateSinceMs = nowMs;
      return BIKE_MB_AI_BUTTON_EVENT_NONE;
    }
    if (!elapsed(nowMs, logic->releaseCandidateSinceMs, BikeMbAiConfig::kReleaseToArmMs)) {
      return BIKE_MB_AI_BUTTON_EVENT_NONE;
    }
    logic->armed = true;
    logic->stablePressed = false;
    logic->candidatePressed = false;
    logic->edgeCandidateSinceMs = nowMs;
    return BIKE_MB_AI_BUTTON_EVENT_NONE;
  }

  if (rawPressed != logic->candidatePressed) {
    logic->candidatePressed = rawPressed;
    logic->edgeCandidateSinceMs = nowMs;
    return BIKE_MB_AI_BUTTON_EVENT_NONE;
  }
  if (logic->candidatePressed == logic->stablePressed ||
      !elapsed(nowMs, logic->edgeCandidateSinceMs, BikeMbAiConfig::kDebounceMs)) {
    return BIKE_MB_AI_BUTTON_EVENT_NONE;
  }

  logic->stablePressed = logic->candidatePressed;
  return logic->stablePressed ? BIKE_MB_AI_BUTTON_EVENT_PRESSED
                              : BIKE_MB_AI_BUTTON_EVENT_RELEASED;
}
```

- [ ] **Step 5: Run the native reducer test**

Run: `python tools/tests/test_ai_framework.py`

Expected: button test PASS; state-machine compile still fails because Task 3 files are absent.

- [ ] **Step 6: Commit the BOOT-key reducer**

```bash
git add src/firmware/bikemb/src/input/ai_button_logic.h src/firmware/bikemb/src/input/ai_button_logic.cpp tools/tests/ai_button_logic_test.cpp
git commit -m "feat: guard and debounce BOOT AI button"
```

### Task 3: Add the Deterministic AI State Machine

**Files:**
- Create: `src/firmware/bikemb/src/ai/ai_types.h`
- Create: `src/firmware/bikemb/src/ai/ai_state_machine.h`
- Create: `src/firmware/bikemb/src/ai/ai_state_machine.cpp`
- Create: `tools/tests/ai_state_machine_test.cpp`

**Interfaces:**
- Consumes: timing and text limits from `ai_config.h`.
- Produces: `BikeMbAiStateMachine_Dispatch(...) -> uint32_t effectMask`; Task 4 must execute only these returned effects.

- [ ] **Step 1: Define public C-compatible types**

Create `src/firmware/bikemb/src/ai/ai_types.h`:

```cpp
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum BikeMbAiState {
  BIKE_MB_AI_DISABLED = 0,
  BIKE_MB_AI_IDLE,
  BIKE_MB_AI_RECORDING,
  BIKE_MB_AI_RECOGNIZING,
  BIKE_MB_AI_THINKING,
  BIKE_MB_AI_SYNTHESIZING,
  BIKE_MB_AI_SPEAKING,
  BIKE_MB_AI_CONNECTING_MUSIC,
  BIKE_MB_AI_MUSIC_PLAYING,
  BIKE_MB_AI_ERROR,
} BikeMbAiState;

typedef struct BikeMbAiSnapshot {
  BikeMbAiState state;
  uint32_t requestId;
  uint32_t stateSinceMs;
  bool wifiConnected;
  bool cancelAvailable;
  char detail[96];
} BikeMbAiSnapshot;

typedef enum BikeMbAiEventType {
  BIKE_MB_AI_EVENT_SET_ENABLED = 0,
  BIKE_MB_AI_EVENT_SET_WIFI,
  BIKE_MB_AI_EVENT_BUTTON_PRESSED,
  BIKE_MB_AI_EVENT_BUTTON_RELEASED,
  BIKE_MB_AI_EVENT_STT_READY,
  BIKE_MB_AI_EVENT_LLM_READY,
  BIKE_MB_AI_EVENT_TTS_STARTED,
  BIKE_MB_AI_EVENT_PLAYBACK_DONE,
  BIKE_MB_AI_EVENT_CANCEL,
  BIKE_MB_AI_EVENT_FAILURE,
  BIKE_MB_AI_EVENT_TICK,
} BikeMbAiEventType;

typedef struct BikeMbAiEvent {
  BikeMbAiEventType type;
  uint32_t nowMs;
  uint32_t requestId;
  bool value;
  const char *detail;
} BikeMbAiEvent;

enum BikeMbAiEffect : uint32_t {
  BIKE_MB_AI_EFFECT_NONE = 0,
  BIKE_MB_AI_EFFECT_START_CAPTURE = 1U << 0,
  BIKE_MB_AI_EFFECT_FINISH_CAPTURE = 1U << 1,
  BIKE_MB_AI_EFFECT_CANCEL_AUDIO = 1U << 2,
  BIKE_MB_AI_EFFECT_CANCEL_CLOUD = 1U << 3,
  BIKE_MB_AI_EFFECT_SUBMIT_STT = 1U << 4,
  BIKE_MB_AI_EFFECT_SUBMIT_LLM = 1U << 5,
  BIKE_MB_AI_EFFECT_SUBMIT_TTS = 1U << 6,
};
```

- [ ] **Step 2: Write transition tests before the reducer**

Create `tools/tests/ai_state_machine_test.cpp` with one helper and five complete scenarios:

```cpp
#include <assert.h>

#include "../../src/firmware/bikemb/src/ai/ai_state_machine.h"

static uint32_t dispatch(
    BikeMbAiStateMachine *machine,
    BikeMbAiEventType type,
    uint32_t nowMs,
    uint32_t requestId = 0,
    bool value = false) {
  const BikeMbAiEvent event = {type, nowMs, requestId, value, nullptr};
  return BikeMbAiStateMachine_Dispatch(machine, event);
}

int main() {
  BikeMbAiStateMachine machine = {};
  BikeMbAiStateMachine_Init(&machine, false, 0);
  assert(machine.snapshot.state == BIKE_MB_AI_DISABLED);

  dispatch(&machine, BIKE_MB_AI_EVENT_SET_ENABLED, 1, 0, true);
  dispatch(&machine, BIKE_MB_AI_EVENT_SET_WIFI, 2, 0, true);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);

  uint32_t effects = dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 100);
  assert(machine.snapshot.state == BIKE_MB_AI_RECORDING);
  assert(machine.snapshot.requestId == 1);
  assert(effects == BIKE_MB_AI_EFFECT_START_CAPTURE);

  effects = dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_RELEASED, 250);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);
  assert((effects & BIKE_MB_AI_EFFECT_CANCEL_AUDIO) != 0);
  assert((effects & BIKE_MB_AI_EFFECT_SUBMIT_STT) == 0);

  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 1000);
  const uint32_t request = machine.snapshot.requestId;
  effects = dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_RELEASED, 1400);
  assert(machine.snapshot.state == BIKE_MB_AI_RECOGNIZING);
  assert(machine.deadlineMs == 16400);
  assert((effects & BIKE_MB_AI_EFFECT_SUBMIT_STT) != 0);

  dispatch(&machine, BIKE_MB_AI_EVENT_STT_READY, 2000, request);
  assert(machine.snapshot.state == BIKE_MB_AI_THINKING);
  dispatch(&machine, BIKE_MB_AI_EVENT_LLM_READY, 3000, request);
  assert(machine.snapshot.state == BIKE_MB_AI_SYNTHESIZING);
  dispatch(&machine, BIKE_MB_AI_EVENT_TTS_STARTED, 4000, request);
  assert(machine.snapshot.state == BIKE_MB_AI_SPEAKING);
  dispatch(&machine, BIKE_MB_AI_EVENT_PLAYBACK_DONE, 5000, request);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);

  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 6000);
  const uint32_t canceledRequest = machine.snapshot.requestId;
  effects = dispatch(&machine, BIKE_MB_AI_EVENT_CANCEL, 6100);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);
  assert(machine.snapshot.requestId == canceledRequest + 1);
  assert((effects & BIKE_MB_AI_EFFECT_CANCEL_CLOUD) != 0);
  dispatch(&machine, BIKE_MB_AI_EVENT_STT_READY, 6200, canceledRequest);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);

  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 7000);
  dispatch(&machine, BIKE_MB_AI_EVENT_TICK, 17000);
  assert(machine.snapshot.state == BIKE_MB_AI_ERROR);
  assert(machine.releaseRequired);
  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_RELEASED, 17100);
  dispatch(&machine, BIKE_MB_AI_EVENT_TICK, 18499);
  assert(machine.snapshot.state == BIKE_MB_AI_ERROR);
  dispatch(&machine, BIKE_MB_AI_EVENT_TICK, 18500);
  assert(machine.snapshot.state == BIKE_MB_AI_IDLE);

  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 19000);
  const uint32_t timedRequest = machine.snapshot.requestId;
  dispatch(&machine, BIKE_MB_AI_EVENT_BUTTON_RELEASED, 19400);
  dispatch(&machine, BIKE_MB_AI_EVENT_STT_READY, 34401, timedRequest);
  assert(machine.snapshot.state == BIKE_MB_AI_ERROR);

  BikeMbAiStateMachine unavailable = {};
  BikeMbAiStateMachine_Init(&unavailable, true, 0);
  effects = dispatch(&unavailable, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 100);
  assert(unavailable.snapshot.state == BIKE_MB_AI_ERROR);
  assert((effects & BIKE_MB_AI_EFFECT_START_CAPTURE) == 0);

  BikeMbAiStateMachine replacement = {};
  BikeMbAiStateMachine_Init(&replacement, true, 0);
  dispatch(&replacement, BIKE_MB_AI_EVENT_SET_WIFI, 1, 0, true);
  dispatch(&replacement, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 1000);
  dispatch(&replacement, BIKE_MB_AI_EVENT_BUTTON_RELEASED, 1400);
  const uint32_t replacedRequest = replacement.snapshot.requestId;
  effects = dispatch(&replacement, BIKE_MB_AI_EVENT_BUTTON_PRESSED, 1500);
  assert(replacement.snapshot.state == BIKE_MB_AI_RECORDING);
  assert(replacement.snapshot.requestId == replacedRequest + 1);
  assert((effects & BIKE_MB_AI_EFFECT_CANCEL_CLOUD) != 0);
  assert((effects & BIKE_MB_AI_EFFECT_START_CAPTURE) != 0);
  return 0;
}
```

- [ ] **Step 3: Run the test and verify the reducer is missing**

Run: `python tools/tests/test_ai_framework.py`

Expected: FAIL compiling because `ai_state_machine.h` does not exist.

- [ ] **Step 4: Declare the reducer state and exact function contract**

Create `src/firmware/bikemb/src/ai/ai_state_machine.h`:

```cpp
#pragma once

#include "ai_types.h"

struct BikeMbAiStateMachine {
  BikeMbAiSnapshot snapshot;
  bool enabled;
  bool buttonHeld;
  bool releaseRequired;
  uint32_t deadlineMs;
  uint32_t errorUntilMs;
};

void BikeMbAiStateMachine_Init(
    BikeMbAiStateMachine *machine,
    bool enabled,
    uint32_t nowMs);
uint32_t BikeMbAiStateMachine_Dispatch(
    BikeMbAiStateMachine *machine,
    const BikeMbAiEvent &event);
```

- [ ] **Step 5: Implement the reducer using this transition table**

Create `src/firmware/bikemb/src/ai/ai_state_machine.cpp` with this complete reducer:

```cpp
#include "ai_state_machine.h"

#include <string.h>

#include "ai_config.h"

namespace {
bool elapsed(uint32_t nowMs, uint32_t sinceMs, uint32_t durationMs) {
  return static_cast<uint32_t>(nowMs - sinceMs) >= durationMs;
}

bool deadlinePassed(uint32_t nowMs, uint32_t deadlineMs) {
  return deadlineMs != 0 && static_cast<int32_t>(nowMs - deadlineMs) > 0;
}

bool isActive(BikeMbAiState state) {
  return state == BIKE_MB_AI_RECORDING ||
         state == BIKE_MB_AI_RECOGNIZING ||
         state == BIKE_MB_AI_THINKING ||
         state == BIKE_MB_AI_SYNTHESIZING ||
         state == BIKE_MB_AI_SPEAKING ||
         state == BIKE_MB_AI_CONNECTING_MUSIC ||
         state == BIKE_MB_AI_MUSIC_PLAYING;
}

bool isCloudState(BikeMbAiState state) {
  return state == BIKE_MB_AI_RECOGNIZING ||
         state == BIKE_MB_AI_THINKING ||
         state == BIKE_MB_AI_SYNTHESIZING;
}

bool isWorkerEvent(BikeMbAiEventType type) {
  return type == BIKE_MB_AI_EVENT_STT_READY ||
         type == BIKE_MB_AI_EVENT_LLM_READY ||
         type == BIKE_MB_AI_EVENT_TTS_STARTED ||
         type == BIKE_MB_AI_EVENT_PLAYBACK_DONE ||
         type == BIKE_MB_AI_EVENT_FAILURE;
}

const char *stateDetail(BikeMbAiState state) {
  switch (state) {
    case BIKE_MB_AI_DISABLED: return "AI disabled";
    case BIKE_MB_AI_IDLE: return "AI ready";
    case BIKE_MB_AI_RECORDING: return "Listening";
    case BIKE_MB_AI_RECOGNIZING: return "Recognizing";
    case BIKE_MB_AI_THINKING: return "Thinking";
    case BIKE_MB_AI_SYNTHESIZING: return "Preparing speech";
    case BIKE_MB_AI_SPEAKING: return "Speaking";
    case BIKE_MB_AI_CONNECTING_MUSIC: return "Connecting music";
    case BIKE_MB_AI_MUSIC_PLAYING: return "Music playing";
    case BIKE_MB_AI_ERROR: return "AI unavailable";
  }
  return "AI unavailable";
}

void setDetail(BikeMbAiSnapshot *snapshot, const char *detail) {
  const char *safeDetail = detail == nullptr ? "" : detail;
  strncpy(snapshot->detail, safeDetail, sizeof(snapshot->detail) - 1);
  snapshot->detail[sizeof(snapshot->detail) - 1] = '\0';
}

void setState(BikeMbAiStateMachine *machine, BikeMbAiState state, uint32_t nowMs) {
  machine->snapshot.state = state;
  machine->snapshot.stateSinceMs = nowMs;
  machine->snapshot.cancelAvailable = isActive(state);
  setDetail(&machine->snapshot, stateDetail(state));
}

void setError(BikeMbAiStateMachine *machine, uint32_t nowMs, const char *detail) {
  setState(machine, BIKE_MB_AI_ERROR, nowMs);
  machine->errorUntilMs = nowMs + BikeMbAiConfig::kErrorDisplayMs;
  setDetail(&machine->snapshot, detail == nullptr ? "AI unavailable" : detail);
}

uint32_t cancelEffects() {
  return BIKE_MB_AI_EFFECT_CANCEL_AUDIO | BIKE_MB_AI_EFFECT_CANCEL_CLOUD;
}
}

void BikeMbAiStateMachine_Init(
    BikeMbAiStateMachine *machine,
    bool enabled,
    uint32_t nowMs) {
  if (machine == nullptr) {
    return;
  }
  *machine = {};
  machine->enabled = enabled;
  setState(machine, enabled ? BIKE_MB_AI_IDLE : BIKE_MB_AI_DISABLED, nowMs);
}

uint32_t BikeMbAiStateMachine_Dispatch(
    BikeMbAiStateMachine *machine,
    const BikeMbAiEvent &event) {
  if (machine == nullptr) {
    return BIKE_MB_AI_EFFECT_NONE;
  }

  if (isWorkerEvent(event.type) && event.requestId != machine->snapshot.requestId) {
    return BIKE_MB_AI_EFFECT_NONE;
  }

  switch (event.type) {
    case BIKE_MB_AI_EVENT_SET_ENABLED:
      machine->enabled = event.value;
      machine->buttonHeld = false;
      machine->releaseRequired = false;
      machine->deadlineMs = 0;
      if (!event.value) {
        ++machine->snapshot.requestId;
        setState(machine, BIKE_MB_AI_DISABLED, event.nowMs);
        return cancelEffects();
      }
      setState(machine, BIKE_MB_AI_IDLE, event.nowMs);
      return BIKE_MB_AI_EFFECT_NONE;

    case BIKE_MB_AI_EVENT_SET_WIFI:
      machine->snapshot.wifiConnected = event.value;
      if (!event.value && isActive(machine->snapshot.state)) {
        ++machine->snapshot.requestId;
        setError(machine, event.nowMs, "Wi-Fi disconnected");
        return cancelEffects();
      }
      return BIKE_MB_AI_EFFECT_NONE;

    case BIKE_MB_AI_EVENT_BUTTON_PRESSED: {
      machine->buttonHeld = true;
      if (!machine->enabled) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      if (machine->snapshot.state == BIKE_MB_AI_ERROR &&
          !elapsed(event.nowMs, machine->snapshot.stateSinceMs,
                   BikeMbAiConfig::kErrorDisplayMs)) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      const bool replacing = isActive(machine->snapshot.state);
      if (!machine->snapshot.wifiConnected) {
        ++machine->snapshot.requestId;
        setError(machine, event.nowMs, "Wi-Fi unavailable");
        return replacing ? cancelEffects() : BIKE_MB_AI_EFFECT_NONE;
      }
      ++machine->snapshot.requestId;
      machine->releaseRequired = false;
      machine->deadlineMs = 0;
      setState(machine, BIKE_MB_AI_RECORDING, event.nowMs);
      uint32_t effects = BIKE_MB_AI_EFFECT_START_CAPTURE;
      if (replacing) {
        effects |= cancelEffects();
      }
      return effects;
    }

    case BIKE_MB_AI_EVENT_BUTTON_RELEASED: {
      machine->buttonHeld = false;
      if (machine->snapshot.state == BIKE_MB_AI_ERROR && machine->releaseRequired) {
        machine->releaseRequired = false;
        return BIKE_MB_AI_EFFECT_NONE;
      }
      if (machine->snapshot.state != BIKE_MB_AI_RECORDING) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      const uint32_t recordingMs = event.nowMs - machine->snapshot.stateSinceMs;
      if (recordingMs < BikeMbAiConfig::kMinRecordingMs) {
        setState(machine, BIKE_MB_AI_IDLE, event.nowMs);
        return BIKE_MB_AI_EFFECT_CANCEL_AUDIO;
      }
      if (recordingMs >= BikeMbAiConfig::kMaxRecordingMs) {
        setError(machine, event.nowMs, "Recording limit reached");
        return BIKE_MB_AI_EFFECT_CANCEL_AUDIO;
      }
      machine->deadlineMs = event.nowMs + BikeMbAiConfig::kCloudDeadlineMs;
      setState(machine, BIKE_MB_AI_RECOGNIZING, event.nowMs);
      return BIKE_MB_AI_EFFECT_FINISH_CAPTURE | BIKE_MB_AI_EFFECT_SUBMIT_STT;
    }

    case BIKE_MB_AI_EVENT_STT_READY:
      if (machine->snapshot.state != BIKE_MB_AI_RECOGNIZING) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      if (deadlinePassed(event.nowMs, machine->deadlineMs)) {
        setError(machine, event.nowMs, "AI request timed out");
        return cancelEffects();
      }
      setState(machine, BIKE_MB_AI_THINKING, event.nowMs);
      return BIKE_MB_AI_EFFECT_SUBMIT_LLM;

    case BIKE_MB_AI_EVENT_LLM_READY:
      if (machine->snapshot.state != BIKE_MB_AI_THINKING) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      if (deadlinePassed(event.nowMs, machine->deadlineMs)) {
        setError(machine, event.nowMs, "AI request timed out");
        return cancelEffects();
      }
      setState(machine, BIKE_MB_AI_SYNTHESIZING, event.nowMs);
      return BIKE_MB_AI_EFFECT_SUBMIT_TTS;

    case BIKE_MB_AI_EVENT_TTS_STARTED:
      if (machine->snapshot.state != BIKE_MB_AI_SYNTHESIZING) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      if (deadlinePassed(event.nowMs, machine->deadlineMs)) {
        setError(machine, event.nowMs, "AI request timed out");
        return cancelEffects();
      }
      setState(machine, BIKE_MB_AI_SPEAKING, event.nowMs);
      return BIKE_MB_AI_EFFECT_NONE;

    case BIKE_MB_AI_EVENT_PLAYBACK_DONE:
      if (machine->snapshot.state == BIKE_MB_AI_SPEAKING) {
        machine->deadlineMs = 0;
        setState(machine, BIKE_MB_AI_IDLE, event.nowMs);
      }
      return BIKE_MB_AI_EFFECT_NONE;

    case BIKE_MB_AI_EVENT_CANCEL:
      if (machine->snapshot.state == BIKE_MB_AI_DISABLED) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      ++machine->snapshot.requestId;
      machine->buttonHeld = false;
      machine->releaseRequired = false;
      machine->deadlineMs = 0;
      setState(machine, BIKE_MB_AI_IDLE, event.nowMs);
      return cancelEffects();

    case BIKE_MB_AI_EVENT_FAILURE:
      if (!isActive(machine->snapshot.state)) {
        return BIKE_MB_AI_EFFECT_NONE;
      }
      setError(machine, event.nowMs, event.detail);
      return cancelEffects();

    case BIKE_MB_AI_EVENT_TICK:
      if (machine->snapshot.state == BIKE_MB_AI_RECORDING &&
          elapsed(event.nowMs, machine->snapshot.stateSinceMs,
                  BikeMbAiConfig::kMaxRecordingMs)) {
        machine->releaseRequired = machine->buttonHeld;
        setError(machine, event.nowMs, "Recording limit reached");
        return BIKE_MB_AI_EFFECT_CANCEL_AUDIO;
      }
      if (isCloudState(machine->snapshot.state) &&
          deadlinePassed(event.nowMs, machine->deadlineMs)) {
        setError(machine, event.nowMs, "AI request timed out");
        return cancelEffects();
      }
      if (machine->snapshot.state == BIKE_MB_AI_ERROR &&
          !machine->releaseRequired && !machine->buttonHeld &&
          elapsed(event.nowMs, machine->snapshot.stateSinceMs,
                  BikeMbAiConfig::kErrorDisplayMs)) {
        setState(machine, BIKE_MB_AI_IDLE, event.nowMs);
      }
      return BIKE_MB_AI_EFFECT_NONE;
  }

  return BIKE_MB_AI_EFFECT_NONE;
}
```

- [ ] **Step 6: Run reducer and full Python tests**

Run:

```powershell
python tools/tests/test_ai_framework.py
powershell -ExecutionPolicy Bypass -File tools/run-tests.ps1
```

Expected: both native binaries exit 0; all Python contract tests PASS.

- [ ] **Step 7: Commit the state machine**

```bash
git add src/firmware/bikemb/src/ai/ai_types.h src/firmware/bikemb/src/ai/ai_state_machine.h src/firmware/bikemb/src/ai/ai_state_machine.cpp tools/tests/ai_state_machine_test.cpp
git commit -m "feat: add AI assistant state machine"
```

### Task 4: Add the GPIO Adapter, Assistant Task, and Mock Worker

**Files:**
- Create: `src/firmware/bikemb/src/input/ai_button.h`
- Create: `src/firmware/bikemb/src/input/ai_button.cpp`
- Create: `src/firmware/bikemb/src/ai/cloud_worker.h`
- Create: `src/firmware/bikemb/src/ai/cloud_worker.cpp`
- Create: `src/firmware/bikemb/src/ai/ai_assistant.h`
- Create: `src/firmware/bikemb/src/ai/ai_assistant.cpp`
- Modify: `src/firmware/bikemb/src/CMakeLists.txt`
- Test: `tools/tests/test_ai_framework.py`

**Interfaces:**
- Consumes: `BikeMbAiButtonLogic_Update`, `BikeMbAiStateMachine_Dispatch`, and effect flags.
- Produces: the public `BikeMbAiAssistant_*` C API and non-blocking `BikeMbAiButton_Tick(uint32_t)` used by `main.cpp`.

- [ ] **Step 1: Extend the contract test with ownership and API checks**

Add this test to `tools/tests/test_ai_framework.py`:

```python
def test_runtime_ownership_contract():
    assistant = read_text(FIRMWARE_ROOT / "src" / "ai" / "ai_assistant.cpp")
    worker = read_text(FIRMWARE_ROOT / "src" / "ai" / "cloud_worker.cpp")
    button = read_text(FIRMWARE_ROOT / "src" / "input" / "ai_button.cpp")
    header = read_text(FIRMWARE_ROOT / "src" / "ai" / "ai_assistant.h")

    assert '"bikemb_ai"' in assistant
    assert '"bikemb_cloud"' in worker
    assert "xQueueSend" in assistant
    assert "BikeMbAiStateMachine_Dispatch" in assistant
    assert "requestId" in worker
    assert "lv_" not in assistant
    assert "lv_" not in worker
    assert "GPIO0" not in button
    assert "BikeMbAiConfig::kButtonGpio" in button
    assert "BikeMbAiAssistant_GetSnapshot" in header
    assert "BikeMbAiAssistant_Cancel" in header
```

- [ ] **Step 2: Run the test and verify runtime files are missing**

Run: `python -c "import sys; sys.path.insert(0, 'tools/tests'); import test_ai_framework as t; t.test_runtime_ownership_contract()"`

Expected: FAIL reading `ai_assistant.cpp`.

- [ ] **Step 3: Add the public assistant and button APIs**

Create `src/firmware/bikemb/src/ai/ai_assistant.h`:

```cpp
#pragma once

#include <stdbool.h>

#include "ai_types.h"

#ifdef __cplusplus
extern "C" {
#endif

bool BikeMbAiAssistant_Init(void);
void BikeMbAiAssistant_OnButtonPressed(void);
void BikeMbAiAssistant_OnButtonReleased(void);
void BikeMbAiAssistant_Cancel(void);
void BikeMbAiAssistant_SetWifiConnected(bool connected);
void BikeMbAiAssistant_GetSnapshot(BikeMbAiSnapshot *out);

#ifdef __cplusplus
}
#endif
```

Create `src/firmware/bikemb/src/input/ai_button.h`:

```cpp
#pragma once

#include <stdint.h>

void BikeMbAiButton_Init(void);
void BikeMbAiButton_Tick(uint32_t nowMs);
```

- [ ] **Step 4: Implement the GPIO adapter without business state**

In `ai_button.cpp`, configure `BikeMbAiConfig::kButtonGpio` as input with pull-up under both Arduino and ESP-IDF gates. `BikeMbAiButton_Tick` must read the pin once, convert low to `rawPressed=true`, call `BikeMbAiButtonLogic_Update`, and route only `PRESSED` and `RELEASED` to the assistant API. Do not include cloud, audio, state, or LVGL headers other than `ai_assistant.h`.

Use these platform calls exactly:

```cpp
#if defined(ARDUINO) && !defined(BIKE_MB_USE_ESPIDF_RUNTIME)
pinMode(BikeMbAiConfig::kButtonGpio, INPUT_PULLUP);
const bool rawPressed = digitalRead(BikeMbAiConfig::kButtonGpio) == LOW;
#else
gpio_config_t config = {};
config.pin_bit_mask = 1ULL << BikeMbAiConfig::kButtonGpio;
config.mode = GPIO_MODE_INPUT;
config.pull_up_en = GPIO_PULLUP_ENABLE;
config.pull_down_en = GPIO_PULLDOWN_DISABLE;
gpio_config(&config);
const bool rawPressed =
    gpio_get_level(static_cast<gpio_num_t>(BikeMbAiConfig::kButtonGpio)) == 0;
#endif
```

- [ ] **Step 5: Define the mock worker queue contract**

Create `src/firmware/bikemb/src/ai/cloud_worker.h`:

```cpp
#pragma once

#include <stdbool.h>
#include <stdint.h>

enum BikeMbCloudStage {
  BIKE_MB_CLOUD_STAGE_STT = 0,
  BIKE_MB_CLOUD_STAGE_LLM,
  BIKE_MB_CLOUD_STAGE_TTS,
};

struct BikeMbCloudJob {
  BikeMbCloudStage stage;
  uint32_t requestId;
  uint32_t deadlineMs;
};

typedef void (*BikeMbCloudResultSink)(
    BikeMbCloudStage stage,
    uint32_t requestId,
    bool success,
    const char *detail);

bool BikeMbCloudWorker_Init(BikeMbCloudResultSink sink);
bool BikeMbCloudWorker_Submit(const BikeMbCloudJob &job);
void BikeMbCloudWorker_CancelBefore(uint32_t validRequestId);
```

- [ ] **Step 6: Implement the mock worker as the only blocking task**

In `cloud_worker.cpp`, create a queue of four `BikeMbCloudJob` values and one task named `bikemb_cloud` with `BikeMbAiConfig::kCloudWorkerStackBytes`. In mock builds, delay STT by 250 ms, LLM by 400 ms, and TTS by 250 ms. Before and after each delay, compare `job.requestId` with the atomic/critical-section-protected minimum valid request ID; drop stale jobs silently. Return only sanitized stage labels (`mock stt ready`, `mock llm ready`, `mock tts ready`) through the result sink. In non-mock builds, reject submissions and report `provider unavailable`; do not add HTTPS code in this milestone.

- [ ] **Step 7: Implement the assistant actor and immutable snapshot**

In `ai_assistant.cpp`:

- Create an eight-entry command queue and one task named `bikemb_ai` with `BikeMbAiConfig::kAssistantStackBytes`.
- Store the reducer and snapshot only in this translation unit.
- Wrap snapshot copies in a `portMUX_TYPE`; hold the critical section only for `sizeof(BikeMbAiSnapshot)` assignment.
- Public command functions enqueue with zero wait and return immediately.
- On every command, call `BikeMbAiStateMachine_Dispatch`, publish the snapshot, then execute returned effects.
- In this milestone, `START_CAPTURE`, `FINISH_CAPTURE`, and `CANCEL_AUDIO` only update sanitized serial diagnostics; they must not initialize audio hardware.
- `SUBMIT_STT`, `SUBMIT_LLM`, and `SUBMIT_TTS` submit the corresponding `BikeMbCloudJob` with the reducer request ID and shared deadline.
- `CANCEL_CLOUD` calls `BikeMbCloudWorker_CancelBefore(machine.snapshot.requestId)`.
- Convert matching successful worker callbacks into `STT_READY`, `LLM_READY`, and `TTS_STARTED`; after `TTS_STARTED`, enqueue `PLAYBACK_DONE` after the mock worker's result is accepted.
- Enqueue a `TICK` every 20 ms from the assistant task so recording and cloud deadlines advance without main-loop work.

The initial snapshot must be `Disabled` when the compile flag is 0. In an enabled mock build, initialization moves to `Idle` with `wifiConnected=false`; the board test harness explicitly sets mock Wi-Fi connected before testing a recording flow.

Extend the existing `if __name__ == "__main__":` block in `test_ai_framework.py` so `test_runtime_ownership_contract()` runs before the final PASS print.

- [ ] **Step 8: Register every new source in ESP-IDF CMake**

Add these entries to `BIKEMB_SRCS` in `src/firmware/bikemb/src/CMakeLists.txt`:

```cmake
    ai/ai_assistant.cpp
    ai/ai_state_machine.cpp
    ai/cloud_worker.cpp
    input/ai_button.cpp
    input/ai_button_logic.cpp
```

- [ ] **Step 9: Run contracts and compile both framework variants**

Run:

```powershell
python tools/tests/test_ai_framework.py
pio run -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c
pio run -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c-ai-framework-test
pio run -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c-idf
```

Expected: all commands exit 0; default and IDF builds contain no enabled AI task, and the mock environment contains both task names.

- [ ] **Step 10: Commit the actor framework**

```bash
git add src/firmware/bikemb/src/ai src/firmware/bikemb/src/input src/firmware/bikemb/src/CMakeLists.txt tools/tests/test_ai_framework.py
git commit -m "feat: add AI assistant actor framework"
```

### Task 5: Integrate the Feature Gate and Verify Cancellation

**Files:**
- Modify: `src/firmware/bikemb/src/main.cpp`
- Modify: `tools/tests/test_ai_framework.py`
- Modify after successful board verification: `openspec/changes/add-cloud-ai-speaker/tasks.md`

**Interfaces:**
- Consumes: all public APIs from Task 4.
- Produces: a bootable mock AI framework test environment with serial-observable state and cancellation behavior.

- [ ] **Step 1: Add an integration contract before changing main**

Add this test to `tools/tests/test_ai_framework.py`:

```python
def test_main_integration_is_feature_gated():
    main = read_text(FIRMWARE_ROOT / "src" / "main.cpp")
    assert "#ifndef BIKE_MB_ENABLE_AI_ASSISTANT" in main
    assert "BikeMbAiAssistant_Init();" in main
    assert "BikeMbAiButton_Init();" in main
    assert "BikeMbAiButton_Tick(now);" in main
    init_pos = main.index("BikeMbAiAssistant_Init();")
    lvgl_pos = main.index("LvglPort_Init();")
    assert init_pos < lvgl_pos
```

- [ ] **Step 2: Run the integration test and verify it fails**

Run: `python -c "import sys; sys.path.insert(0, 'tools/tests'); import test_ai_framework as t; t.test_main_integration_is_feature_gated()"`

Expected: FAIL because the AI APIs are absent from `main.cpp`.

- [ ] **Step 3: Add the minimum feature-gated main integration**

Add the two headers:

```cpp
#include "ai/ai_assistant.h"
#include "input/ai_button.h"
```

Add the default macro beside existing feature macros:

```cpp
#ifndef BIKE_MB_ENABLE_AI_ASSISTANT
#define BIKE_MB_ENABLE_AI_ASSISTANT 0
#endif
```

Immediately after `BoardSupport_Init()` in Arduino `setup()`, add:

```cpp
#if BIKE_MB_ENABLE_AI_ASSISTANT
  BikeMbAiAssistant_Init();
  BikeMbAiButton_Init();
#endif
```

After `const uint32_t now = millis();` in `loop()`, add:

```cpp
#if BIKE_MB_ENABLE_AI_ASSISTANT
  BikeMbAiButton_Tick(now);
#endif
```

Do not add AI initialization to ESP-IDF `app_main()` in this milestone; the files must compile there, but the runtime integration remains Arduino-only.

Extend the existing `if __name__ == "__main__":` block in `test_ai_framework.py` so `test_main_integration_is_feature_gated()` runs before the final PASS print.

- [ ] **Step 4: Run the complete regression set**

Run:

```powershell
powershell -ExecutionPolicy Bypass -File tools/run-tests.ps1
pio run -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c
pio run -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c-ai-framework-test
pio run -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c-audio-self-test
pio run -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c-mode-prompts-test
pio run -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c-voice-direct-test
pio run -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c-idf
```

Expected: all tests and builds exit 0.

- [ ] **Step 5: Perform the board safety matrix on the mock environment**

Flash and monitor:

```powershell
pio run -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c-ai-framework-test -t upload
pio device monitor -d src/firmware/bikemb -e esp32-s3-touch-lcd-1-85c-ai-framework-test
```

Record these observations in the commit message body or review note:

| Check | Expected observation |
| --- | --- |
| Normal power-on, no key | Dashboard starts; no AI press before or after 3 seconds. |
| Tap BOOT before 3 seconds | No recording/mock request. |
| Hold BOOT across 3 seconds | No request when 3 seconds elapse; release, wait 50 ms, then a new press works. |
| Runtime press under 300 ms | State returns to Idle; no mock STT stage. |
| Runtime hold 300 ms to under 10 s | Recording -> Recognizing -> Thinking -> Synthesizing -> Speaking -> Idle. |
| Hold 10 seconds | Error appears; release does not submit STT; Error remains at least 1500 ms. |
| Press during mock LLM wait | Old request is canceled; stale result does not change the new request state. |
| Hold BOOT and reset | Board may enter ROM download mode; releasing BOOT and resetting restores normal boot. |
| Dashboard navigation during mock waits | Page switching remains responsive. |

- [ ] **Step 6: Mark only the verified hardware task complete**

After every row above has been observed, change this exact OpenSpec task from unchecked to checked:

```markdown
- [x] 上板确认 3000 ms 启动保护、50 ms 释放解锁、30 ms 消抖和按住 BOOT 复位后的恢复行为。
```

- [ ] **Step 7: Commit the integration**

```bash
git add src/firmware/bikemb/src/main.cpp tools/tests/test_ai_framework.py openspec/changes/add-cloud-ai-speaker/tasks.md
git commit -m "feat: integrate guarded AI framework test"
```

## Completion Gate

This framework milestone is complete only when:

- Host reducer tests pass.
- Default, mock AI, audio self-test, prompt, voice-direct, and ESP-IDF environments build.
- The default firmware creates no AI tasks and performs no Wi-Fi or audio initialization for AI.
- Mock blocking work does not delay snapshot cancellation or dashboard navigation.
- All nine board safety matrix rows have recorded results.
- No real credential or private URL appears in `git diff --cached`.

After this gate, write separate implementation plans in this order: `AudioSession` ownership and capture, asynchronous Wi-Fi plus real STT/DeepSeek/TTS adapters, AI dashboard/page integration, then the HTTPS MP3 decoder spike and `MusicService`. Do not combine those subsystems into this framework milestone.
