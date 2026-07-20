from contract_helpers import REPO_ROOT, check, read_repo_text


MAIN = "src/firmware/bikemb/src/main.cpp"
CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"
PLATFORMIO_INI = "src/firmware/bikemb/platformio.ini"
AUDIO_SESSION_HEADER = "src/firmware/bikemb/src/audio/audio_session.h"
AUDIO_SESSION_SOURCE = "src/firmware/bikemb/src/audio/audio_session.cpp"
CAPTURE_CORE_HEADER = "src/firmware/bikemb/src/audio/audio_capture_core.h"
CAPTURE_CORE_SOURCE = "src/firmware/bikemb/src/audio/audio_capture_core.cpp"
CAPTURE_SELF_TEST_HEADER = "src/firmware/bikemb/src/audio/audio_capture_self_test.h"
CAPTURE_SELF_TEST_SOURCE = "src/firmware/bikemb/src/audio/audio_capture_self_test.cpp"


def _env_body(config: str, env_name: str) -> str:
    marker = f"[env:{env_name}]"
    start = config.find(marker)
    check(start >= 0, f"Missing PlatformIO env: {env_name}")
    next_env = config.find("\n[env:", start + len(marker))
    return config[start:] if next_env < 0 else config[start:next_env]


def test_audio_capture_core_defines_bounded_clip_contract() -> None:
    check((REPO_ROOT / CAPTURE_CORE_HEADER).exists(), "Audio capture core header must exist.")
    check((REPO_ROOT / CAPTURE_CORE_SOURCE).exists(), "Audio capture core source must exist.")

    header = read_repo_text(CAPTURE_CORE_HEADER)
    source = read_repo_text(CAPTURE_CORE_SOURCE)

    check("BIKE_MB_AUDIO_CAPTURE_SAMPLE_RATE_HZ 16000" in header, "Capture must be fixed at 16 kHz.")
    check("BIKE_MB_AUDIO_CAPTURE_MAX_MS 10000" in header, "Capture must cap recordings at 10 seconds.")
    check("BIKE_MB_AUDIO_CAPTURE_MAX_BYTES (384U * 1024U)" in header, "Capture clip must stay within 384 KiB.")
    check("BikeMbAudioCaptureCore_MaxSamplesForMs" in header, "Capture core must expose duration-to-samples sizing.")
    check("BikeMbAudioCaptureCore_DownmixStereoToMono" in header, "Capture core must expose stereo-to-mono conversion.")
    check("BIKE_MB_AUDIO_CAPTURE_MAX_BYTES / sizeof(int16_t)" in source, "Capture sizing must enforce the byte cap.")


def test_audio_session_exposes_ai_capture_clip_api() -> None:
    header = read_repo_text(AUDIO_SESSION_HEADER)
    source = read_repo_text(AUDIO_SESSION_SOURCE)

    check("BikeMbAudioClip" in header, "AudioSession must expose a bounded clip view.")
    check("BikeMbAudioSession_StartCapture" in header, "AudioSession must expose capture start.")
    check("BikeMbAudioSession_PollCapture" in header, "AudioSession must expose non-blocking capture polling.")
    check("BikeMbAudioSession_FinishCapture" in header, "AudioSession must expose capture finish.")
    check("BikeMbAudioSession_ReleaseClip" in header, "AudioSession must expose clip release.")
    check("BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE" in source, "Capture must use the AI_CAPTURE audio owner.")
    check("ps_malloc" in source, "Capture must allocate the bounded clip in PSRAM.")
    check("BikeMbAudioCaptureCore_DownmixStereoToMono" in source, "Capture must convert stereo I2S input to mono PCM.")


def test_audio_capture_self_test_build_env_exists() -> None:
    main = read_repo_text(MAIN)
    cmake = read_repo_text(CMAKE)
    config = read_repo_text(PLATFORMIO_INI)

    check((REPO_ROOT / CAPTURE_SELF_TEST_HEADER).exists(), "Audio capture self-test header must exist.")
    check((REPO_ROOT / CAPTURE_SELF_TEST_SOURCE).exists(), "Audio capture self-test source must exist.")
    check("audio/audio_capture_core.cpp" in cmake, "ESP-IDF CMake source list must include capture core.")
    check("audio/audio_capture_self_test.cpp" in cmake, "ESP-IDF CMake source list must include capture self-test.")
    check('#include "audio/audio_capture_self_test.h"' in main, "Main firmware must include capture self-test.")
    check("BikeMbAudioCaptureSelfTest_Init();" in main, "Arduino setup must initialize capture self-test.")

    env = _env_body(config, "esp32-s3-touch-lcd-1-85c-audio-capture-test")
    check("-D BIKE_MB_ENABLE_AUDIO_SESSION=1" in env, "Capture test env must enable AudioSession.")
    check("-D BIKE_MB_ENABLE_AUDIO_CAPTURE_SELF_TEST=1" in env, "Capture test env must enable capture self-test.")
    check("BIKE_MB_ENABLE_AUDIO_SELF_TEST=1" not in env, "Capture test env must not enable audio self-test.")
    check("BIKE_MB_ENABLE_AUDIO_PROMPTS=1" not in env, "Capture test env must not enable audio prompts.")
    check("BIKE_MB_ENABLE_VOICE_COMMANDS=1" not in env, "Capture test env must not enable voice commands.")


if __name__ == "__main__":
    test_audio_capture_core_defines_bounded_clip_contract()
    test_audio_session_exposes_ai_capture_clip_api()
    test_audio_capture_self_test_build_env_exists()
    print("PASS test_audio_capture_contract")
