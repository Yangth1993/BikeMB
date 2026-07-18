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


if __name__ == "__main__":
    test_configuration_contract()
    test_native_ai_reducers()
    test_runtime_ownership_contract()
    print("PASS test_ai_framework")
