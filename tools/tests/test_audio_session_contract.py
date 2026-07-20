from contract_helpers import REPO_ROOT, check, read_repo_text


MAIN = "src/firmware/bikemb/src/main.cpp"
CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"
PLATFORMIO_INI = "src/firmware/bikemb/platformio.ini"
AUDIO_SESSION_HEADER = "src/firmware/bikemb/src/audio/audio_session.h"
AUDIO_SESSION_SOURCE = "src/firmware/bikemb/src/audio/audio_session.cpp"
AUDIO_SESSION_CORE_HEADER = "src/firmware/bikemb/src/audio/audio_session_core.h"
AUDIO_SESSION_CORE_SOURCE = "src/firmware/bikemb/src/audio/audio_session_core.cpp"


def _env_body(config: str, env_name: str) -> str:
    marker = f"[env:{env_name}]"
    start = config.find(marker)
    check(start >= 0, f"Missing PlatformIO env: {env_name}")
    next_env = config.find("\n[env:", start + len(marker))
    return config[start:] if next_env < 0 else config[start:next_env]


def test_audio_session_boundary_is_present_and_default_off() -> None:
    main = read_repo_text(MAIN)
    cmake = read_repo_text(CMAKE)

    check((REPO_ROOT / AUDIO_SESSION_HEADER).exists(), "AudioSession public header must exist.")
    check((REPO_ROOT / AUDIO_SESSION_SOURCE).exists(), "AudioSession source must exist.")
    check((REPO_ROOT / AUDIO_SESSION_CORE_HEADER).exists(), "AudioSession core header must exist.")
    check((REPO_ROOT / AUDIO_SESSION_CORE_SOURCE).exists(), "AudioSession core source must exist.")
    check("BIKE_MB_ENABLE_AUDIO_SESSION" in main, "main.cpp must define the AudioSession gate.")
    check("#define BIKE_MB_ENABLE_AUDIO_SESSION 0" in main, "AudioSession must default to disabled.")
    check('#include "audio/audio_session.h"' in main, "Main firmware must include AudioSession.")
    check("BikeMbAudioSession_Init();" in main, "Main firmware must initialize AudioSession only through its gate.")
    check("audio/audio_session.cpp" in cmake, "ESP-IDF CMake source list must include AudioSession.")
    check("audio/audio_session_core.cpp" in cmake, "ESP-IDF CMake source list must include AudioSession core.")


def test_audio_session_is_the_hardware_owner() -> None:
    header = read_repo_text(AUDIO_SESSION_HEADER) + read_repo_text(AUDIO_SESSION_CORE_HEADER)
    source = read_repo_text(AUDIO_SESSION_SOURCE)

    check("BikeMbAudioSessionOwner" in header, "AudioSession must expose owner diagnostics.")
    check("BIKE_MB_AUDIO_SESSION_OWNER_AI_CAPTURE" in header, "AudioSession must include AI capture ownership.")
    check("BIKE_MB_AUDIO_SESSION_OWNER_AI_PLAYBACK" in header, "AudioSession must include AI playback ownership.")
    check("BIKE_MB_AUDIO_SESSION_OWNER_PROMPT" in header, "AudioSession must include prompt ownership.")
    check("BIKE_MB_AUDIO_SESSION_OWNER_MUSIC" in header, "AudioSession must include music ownership.")
    check("BikeMbAudioSession_Acquire" in header, "AudioSession must expose acquire.")
    check("BikeMbAudioSession_Release" in header, "AudioSession must expose release.")
    check("BikeMbAudioSession_GetOwner" in header, "AudioSession must expose current owner.")
    check("BikeMbAudioSession_WriteStereoPcm" in header, "AudioSession must expose stereo PCM writes.")
    check(
        "return bytesWritten / (2 * sizeof(samples[0]))" in source,
        "AudioSession stereo writes must return frames written, not raw bytes.",
    )
    check("BikeMbAudioSession_ReadMicBytes" in header, "AudioSession must expose microphone reads.")
    check("I2SClass g_i2s(I2S_NUM_0)" in source, "AudioSession must be the I2S0 owner.")
    check("kAudioSessionReadTimeoutMs = 10" in source, "AudioSession must keep mic reads low-latency.")
    check("kAudioSessionWriteTimeoutMs = 250" in source, "AudioSession must allow playback writes to wait for DMA drain.")
    check("g_i2s.setTimeout(kAudioSessionReadTimeoutMs)" in source, "AudioSession must bound blocking I2S reads.")
    check("g_i2s.setTimeout(kAudioSessionWriteTimeoutMs)" in source, "AudioSession must allow bounded blocking I2S writes.")
    check("kEs8311Address = 0x18" in source, "AudioSession must own the ES8311 speaker codec.")
    check("kEs7210Address = 0x40" in source, "AudioSession must own the ES7210 microphone codec.")
    check("GPIO_NUM_2" in source and "GPIO_NUM_48" in source, "AudioSession must own I2S clock pins.")
    check("GPIO_NUM_47" in source and "GPIO_NUM_39" in source, "AudioSession must own I2S data pins.")


def test_audio_session_build_env_is_mutually_exclusive() -> None:
    config = read_repo_text(PLATFORMIO_INI)
    env = _env_body(config, "esp32-s3-touch-lcd-1-85c-audio-session-test")

    check("-D BIKE_MB_ENABLE_AUDIO_SESSION=1" in env, "AudioSession env must explicitly enable AudioSession.")
    check("BIKE_MB_ENABLE_AUDIO_SELF_TEST=1" not in env, "AudioSession env must not enable audio self-test.")
    check("BIKE_MB_ENABLE_AUDIO_PROMPTS=1" not in env, "AudioSession env must not enable audio prompts.")
    check("BIKE_MB_ENABLE_VOICE_COMMANDS=1" not in env, "AudioSession env must not enable voice commands.")
    check("BIKE_MB_ENABLE_AI_ASSISTANT=1" not in env, "AudioSession env must not enable AI assistant.")


if __name__ == "__main__":
    test_audio_session_boundary_is_present_and_default_off()
    test_audio_session_is_the_hardware_owner()
    test_audio_session_build_env_is_mutually_exclusive()
    print("PASS test_audio_session_contract")
