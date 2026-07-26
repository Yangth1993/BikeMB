from contract_helpers import check, read_repo_text


PLATFORMIO_INI = "src/firmware/bikemb/platformio.ini"
CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"
CLOUD_SOURCE = "src/firmware/bikemb/src/ai/cloud_worker.cpp"


def _env_body(config: str, env_name: str) -> str:
    marker = f"[env:{env_name}]"
    start = config.find(marker)
    check(start >= 0, f"Missing PlatformIO env: {env_name}")
    next_env = config.find("\n[env:", start + len(marker))
    return config[start:] if next_env < 0 else config[start:next_env]


def test_idf_cloud_transport_has_non_speaker_board_env() -> None:
    config = read_repo_text(PLATFORMIO_INI)
    env = _env_body(config, "esp32-s3-touch-lcd-1-85c-idf-ai-voice-cloud-test")

    check("framework = espidf" in env, "IDF cloud env must use ESP-IDF.")
    check("board_build.partitions = partitions_idf_16m.csv" in env, "IDF cloud env must keep the 4MB app partition.")
    check("-D BIKE_MB_USE_ESPIDF_RUNTIME=1" in env, "IDF cloud env must keep the dual-core runtime.")
    check("-DBIKE_MB_IDF_ENABLE_AI_ASSISTANT=ON" in env, "IDF cloud env must enable AI Assistant through CMake.")
    check("-DBIKE_MB_IDF_ENABLE_AUDIO_SESSION=ON" in env, "IDF cloud env must enable AudioSession through CMake.")
    check("BIKE_MB_IDF_ENABLE_AUDIO_SELF_TEST" not in env, "IDF cloud env must not enable audible self-test.")
    check("BIKE_MB_ENABLE_AUDIO_SELF_TEST" not in env, "IDF cloud env must not enable audible self-test.")
    check("BIKE_MB_IDF_AI_USE_MOCK_PROVIDERS" not in env, "IDF cloud env must use real providers, not mock providers.")
    check("BIKE_MB_AI_USE_MOCK_PROVIDERS" not in env, "IDF cloud env must use real providers, not mock providers.")


def test_idf_cloud_worker_uses_esp_http_client_for_asr_chat_and_tts() -> None:
    source = read_repo_text(CLOUD_SOURCE)
    cmake = read_repo_text(CMAKE)

    check('#include "esp_http_client.h"' in source, "IDF cloud worker must use esp_http_client.")
    check("esp_http_client_set_header" in source, "IDF cloud worker must set HTTP headers explicitly.")
    check("Authorization" in source and "Bearer " in source, "IDF cloud requests must use bearer auth.")
    check("Content-Type" in source and "application/json" in source, "IDF cloud requests must send JSON.")
    check("esp_http_client_open" in source, "IDF cloud worker must open requests with a known content length.")
    check("esp_http_client_write" in source, "IDF cloud worker must stream generated JSON to the HTTP client.")
    check("esp_http_client_fetch_headers" in source, "IDF cloud worker must fetch response headers.")
    check("esp_http_client_get_status_code" in source, "IDF cloud worker must log sanitized HTTP status.")
    check("esp_http_client_read" in source, "IDF cloud worker must read bounded response bodies.")
    check("esp_http_client_cleanup" in source, "IDF cloud worker must always clean up HTTP handles.")
    check("postQwenAsrIdf" in source, "IDF cloud worker must migrate Qwen ASR first.")
    check("postQwenChatIdf" in source, "IDF cloud worker must migrate Qwen Chat first.")
    check("postCosyVoiceTtsIdf" in source, "IDF cloud worker must migrate CosyVoice TTS after ASR/Chat.")
    check("text/event-stream" in source, "IDF CosyVoice requests must accept SSE responses.")
    check("X-DashScope-SSE" in source, "IDF CosyVoice requests must enable DashScope SSE.")
    check("readSseBodyIdf" in source, "IDF cloud worker must stream and parse CosyVoice SSE bodies.")
    check(
        "BikeMbAudioSession_WriteStereoPcm" in source,
        "IDF CosyVoice playback must use AudioSession, not direct I2S access.",
    )
    check("kTtsPlaybackGain = 2" in source, "IDF CosyVoice playback must apply bounded software gain.")
    check("applyTtsPlaybackGain" in source, "IDF CosyVoice playback must saturate amplified PCM samples.")
    check(
        "idf cosyvoice playback unavailable" not in source,
        "IDF TTS playback must no longer be the silent unavailable stub.",
    )
    check("BIKE_MB_USE_ESPIDF_RUNTIME" in source, "IDF cloud transport must be gated to the ESP-IDF runtime.")
    check("esp_http_client" in cmake, "ESP-IDF CMake component requirements must include esp_http_client.")
    check("Serial.println(BIKE_MB_AI_DASHSCOPE_TOKEN)" not in source, "Tokens must never be logged.")
    check("ESP_LOGI(\"BikeMbCloud\", \"%s\", BIKE_MB_AI_DASHSCOPE_TOKEN)" not in source, "Tokens must never be logged.")


if __name__ == "__main__":
    test_idf_cloud_transport_has_non_speaker_board_env()
    test_idf_cloud_worker_uses_esp_http_client_for_asr_chat_and_tts()
    print("PASS test_idf_cloud_transport_contract")
