from contract_helpers import check, read_repo_text


PLATFORMIO_INI = "src/firmware/bikemb/platformio.ini"
AUDIO_SESSION_SOURCE = "src/firmware/bikemb/src/audio/audio_session.cpp"


def _env_body(config: str, env_name: str) -> str:
    marker = f"[env:{env_name}]"
    start = config.find(marker)
    check(start >= 0, f"Missing PlatformIO env: {env_name}")
    next_env = config.find("\n[env:", start + len(marker))
    return config[start:] if next_env < 0 else config[start:next_env]


def test_idf_audio_session_has_dedicated_build_env() -> None:
    config = read_repo_text(PLATFORMIO_INI)
    env = _env_body(config, "esp32-s3-touch-lcd-1-85c-idf-audio-session-test")

    check("framework = espidf" in env, "IDF AudioSession env must use ESP-IDF.")
    check("board_build.partitions = partitions_idf_16m.csv" in env, "IDF AudioSession env must keep the 4MB app partition.")
    check("-D BIKE_MB_USE_ESPIDF_RUNTIME=1" in env, "IDF AudioSession env must keep the dual-core IDF runtime.")
    check(
        "-DBIKE_MB_IDF_ENABLE_AUDIO_SESSION=ON" in env,
        "IDF AudioSession env must enable AudioSession through CMake.",
    )
    check("BIKE_MB_ENABLE_AUDIO_SELF_TEST=1" not in env, "IDF AudioSession env must not enable self-test yet.")
    check("BIKE_MB_ENABLE_AUDIO_PROMPTS=1" not in env, "IDF AudioSession env must not enable prompts yet.")
    check("BIKE_MB_ENABLE_VOICE_COMMANDS=1" not in env, "IDF AudioSession env must not enable voice commands.")
    check("BIKE_MB_ENABLE_AI_ASSISTANT" not in env, "IDF AudioSession env must not define AI assistant macros.")


def test_idf_audio_session_owns_codec_and_i2s_channels() -> None:
    source = read_repo_text(AUDIO_SESSION_SOURCE)

    check('#include "driver/i2s_std.h"' in source, "IDF AudioSession must use the ESP-IDF standard I2S driver.")
    check("i2s_chan_handle_t" in source, "IDF AudioSession must keep explicit TX/RX channel handles.")
    check("i2s_new_channel" in source, "IDF AudioSession must allocate I2S channels.")
    check("i2s_channel_init_std_mode" in source, "IDF AudioSession must initialize standard I2S mode.")
    check("i2s_channel_enable" in source, "IDF AudioSession must enable I2S channels.")
    check("i2s_channel_write" in source, "IDF AudioSession playback must use IDF channel writes.")
    check("i2s_channel_read" in source, "IDF AudioSession capture must use IDF channel reads.")
    check("heap_caps_malloc" in source, "IDF AudioSession capture must allocate clips through heap capabilities.")
    check("MALLOC_CAP_SPIRAM" in source, "IDF AudioSession capture clips must prefer PSRAM.")
    check("ESP_LOGI" in source, "IDF AudioSession must emit board acceptance logs.")


if __name__ == "__main__":
    test_idf_audio_session_has_dedicated_build_env()
    test_idf_audio_session_owns_codec_and_i2s_channels()
    print("PASS test_idf_audio_session_contract")
