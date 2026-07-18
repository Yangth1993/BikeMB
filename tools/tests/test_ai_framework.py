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
