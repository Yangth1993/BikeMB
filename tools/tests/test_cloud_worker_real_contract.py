from contract_helpers import REPO_ROOT, check, find_function_body, read_repo_text


CLOUD_HEADER = "src/firmware/bikemb/src/ai/cloud_worker.h"
CLOUD_SOURCE = "src/firmware/bikemb/src/ai/cloud_worker.cpp"
ASSISTANT_SOURCE = "src/firmware/bikemb/src/ai/ai_assistant.cpp"
PLATFORMIO = "src/firmware/bikemb/platformio.ini"
SECRETS_EXAMPLE = "src/firmware/bikemb/include/ai_secrets.example.h"


def test_real_cloud_worker_accepts_capture_clip_and_uses_bailian() -> None:
    header = read_repo_text(CLOUD_HEADER)
    source = read_repo_text(CLOUD_SOURCE)
    assistant = read_repo_text(ASSISTANT_SOURCE)
    example = read_repo_text(SECRETS_EXAMPLE)

    check("BikeMbCloudWorker_SetCaptureClip" in header, "Cloud worker must accept captured audio clips.")
    check("BikeMbCloudWorker_SetCaptureClip" in assistant, "Assistant must hand finished clips to cloud worker.")
    check("BikeMbAudioSession_ReleaseClip" in source, "Cloud worker must release owned clips.")
    check("qwen_asr_adapter.h" in source, "Cloud worker must use Qwen ASR for Bailian STT.")
    check("qwen_chat_adapter.h" in source, "Cloud worker must use Qwen chat for Bailian LLM.")
    check("cosyvoice_tts_adapter.h" in source, "Cloud worker must use CosyVoice for Bailian TTS.")
    check("BIKE_MB_AI_DASHSCOPE_TOKEN" in source, "Cloud worker must use the shared Bailian token macro.")
    check("BIKE_MB_AI_DASHSCOPE_TOKEN" in example, "Secrets example must document the Bailian token.")
    check("BIKE_MB_AI_DEEPSEEK_TOKEN" not in source, "Bailian-only worker must not require DeepSeek token.")


def test_real_cloud_worker_submit_queues_jobs_instead_of_immediate_failure() -> None:
    source = read_repo_text(CLOUD_SOURCE)
    init_body = find_function_body(source, "bool BikeMbCloudWorker_Init")
    submit_body = find_function_body(source, "bool BikeMbCloudWorker_Submit")

    check("xTaskCreate" in init_body, "Cloud worker must start its worker task in real and mock modes.")
    check("xQueueSend" in submit_body, "Cloud worker submit must queue jobs in real and mock modes.")
    check("provider unavailable" not in submit_body, "Real cloud submit must not immediately fail before Qwen ASR.")


def test_real_cloud_worker_has_https_and_streaming_contract() -> None:
    source = read_repo_text(CLOUD_SOURCE)
    platformio = read_repo_text(PLATFORMIO)

    check("WiFiClientSecure" in source, "Real cloud worker must use HTTPS client.")
    check("Authorization: Bearer " in source, "Real cloud requests must use Bearer auth.")
    check("Content-Type: application/json" in source, "Real cloud requests must send JSON.")
    check("X-DashScope-SSE: enable" in source, "TTS streaming must enable DashScope SSE.")
    check("Accept: " in source and "text/event-stream" in source, "TTS streaming must request SSE responses.")
    check("application/json" in source, "ASR and chat requests must keep JSON accept headers.")
    check("kTlsHandshakeTimeoutSeconds = 8" in source, "TLS handshake must be bounded below the AI cloud deadline.")
    check("setHandshakeTimeout(kTlsHandshakeTimeoutSeconds)" in source, "HTTPS client must not use the Arduino TLS default 120-second handshake timeout.")
    check("connect start" in source, "Cloud diagnostics must log before blocking in TLS connect.")
    check("connect ok" in source, "Cloud diagnostics must log successful TLS connect.")
    check("BikeMbQwenAsr_WriteRequestJson" in source, "ASR request body must use Qwen ASR adapter.")
    check("BikeMbQwenChat_WriteRequestJson" in source, "LLM request body must use Qwen chat adapter.")
    check("BikeMbCosyVoice_WriteRequestJson" in source, "TTS request body must use CosyVoice adapter.")
    check("BufferedClientSinkContext" in source, "HTTPS JSON body writes must be buffered.")
    check("kHttpWriteBufferBytes = 1024" in source, "HTTPS body write buffer must batch small Base64 chunks.")
    check("flushBufferedClientSink" in source, "HTTPS body writer must flush buffered bytes before reading the response.")
    check("bufferedClientSink" in source, "Provider adapters must write through the buffered HTTPS sink.")
    check("BikeMbCosyVoice_HandleSseLineWithStream" in source, "TTS response must keep PCM byte alignment across SSE chunks.")
    check("BikeMbCosyVoice_PcmStreamFinish" in source, "TTS response must finish the PCM stream after all SSE chunks.")
    check("TtsPcmBufferContext" in source, "TTS PCM must be buffered before playback to avoid network underruns.")
    check("kMaxTtsPcmSamples = 320000" in source, "TTS PCM buffer must be bounded in PSRAM.")
    check("kMaxTtsPlaybackSamples = 96000" in source, "TTS playback must be capped near six seconds for repeatable assistant turns.")
    check("cosyvoice playback clipped" in source, "TTS diagnostics must log when long replies are clipped.")
    check("cosyvoice pcm missing" in source, "TTS diagnostics must report 200 responses without playable PCM.")
    check("kTtsMaxAttempts = 2" in source, "TTS must retry once when a 200 response has no playable PCM.")
    check("cosyvoice retry" in source, "TTS diagnostics must log retry attempts.")
    check("cosyvoice sse no audio data" in source, "TTS diagnostics must distinguish SSE responses without audio.data.")
    check("cosyvoice sse pcm overflow" in source, "TTS diagnostics must distinguish oversized TTS PCM responses.")
    check("cosyvoice sse pcm finish failed" in source, "TTS diagnostics must distinguish malformed PCM byte alignment.")
    check("ttsPcmBufferSink" in source, "TTS response must collect PCM through a bounded buffer sink.")
    check("playTtsPcmBuffer" in source, "TTS playback must drain buffered PCM through AudioSession.")
    check("cosyvoice pcm samples=" in source, "TTS diagnostics must log buffered PCM size.")
    check("kMaxSseLineBytes = 12 * 1024" in source, "TTS SSE buffer must hold real CosyVoice audio.data lines.")
    check("ps_malloc(kMaxSseLineBytes)" in source, "TTS SSE line buffer must be allocated in PSRAM.")
    check("char line[1024]" not in source, "TTS SSE body must not truncate real CosyVoice audio.data lines.")
    check("logCloudStatus" in source, "Cloud worker must log sanitized stage/status diagnostics.")
    check("logCloudLabel" in source, "Cloud worker must log sanitized stage boundary diagnostics.")
    check("logCloudDuration" in source, "Cloud worker must log stage timing diagnostics.")
    check('"request sent"' in source, "Cloud worker must time request send latency.")
    check('"headers"' in source, "Cloud worker must time response header latency.")
    check('"body"' in source, "Cloud worker must time response body latency.")
    check("qwen asr stage start" in source, "ASR diagnostics must log stage start.")
    check("\"qwen asr\"" in source and "connect failed" in source, "ASR diagnostics must log connection failures.")
    check("\"qwen asr\"" in source and "post failed" in source, "ASR diagnostics must log request write failures.")
    check("qwen asr" in source and "http status=" in source, "ASR diagnostics must include sanitized HTTP status.")
    check("qwen chat" in source and "http status=" in source, "Chat diagnostics must include sanitized HTTP status.")
    check("cosyvoice" in source and "http status=" in source, "TTS diagnostics must include sanitized HTTP status.")
    check("logCloudText" in source, "Cloud worker must log bounded ASR/chat text diagnostics.")
    check('logCloudText("qwen asr text", out)' in source, "ASR diagnostics must log recognized text.")
    check('logCloudText("qwen chat answer", out)' in source, "Chat diagnostics must log generated answer text.")
    check('logCloudError("cosyvoice", body)' in source, "TTS diagnostics must log sanitized non-2xx error details.")
    check("logCloudError" in source, "Cloud worker must log sanitized non-2xx error details.")
    check('\\"message\\":\\"' in source and '\\"request_id\\":\\"' in source, "Cloud error diagnostics must extract safe server error fields.")
    check("logCloudMemory" in source, "Cloud worker must log sanitized heap diagnostics before TLS connect.")
    check("heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)" in source, "Cloud diagnostics must include internal heap.")
    check("heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)" in source, "Cloud diagnostics must include largest internal block.")
    check("heap_caps_get_free_size(MALLOC_CAP_SPIRAM)" in source, "Cloud diagnostics must include PSRAM free bytes.")
    check("Serial.println(BIKE_MB_AI_DASHSCOPE_TOKEN)" not in source, "Token must never be logged.")
    check("ESP_LOGI(\"BikeMbCloud\", \"%s\", BIKE_MB_AI_DASHSCOPE_TOKEN)" not in source, "Token must never be logged.")
    check("[env:esp32-s3-touch-lcd-1-85c-ai-voice-cloud-test]" in platformio, "PlatformIO must include real cloud test env.")
    cloud_env = platformio.split("[env:esp32-s3-touch-lcd-1-85c-ai-voice-cloud-test]", 1)[1]
    check("-D BIKE_MB_ENABLE_AI_ASSISTANT=1" in cloud_env, "Cloud env must enable AI assistant.")
    check("-D BIKE_MB_ENABLE_AUDIO_SESSION=1" in cloud_env, "Cloud env must enable AudioSession.")
    check("-D BIKE_MB_LVGL_BUFFER_LINES=20" in cloud_env, "Cloud env must free internal RAM by using a smaller LVGL draw buffer.")
    check(
        "framework-arduinoespressif32-libs/esp32s3/include/esp_wifi/include" in cloud_env,
        "Cloud env must expose ESP-IDF WiFi headers needed by Arduino WiFi libraries.",
    )
    check(
        "framework-arduinoespressif32-libs/esp32s3/include/esp_wifi/include/local" in cloud_env,
        "Cloud env must expose target-local WiFi type definitions for Arduino AP sources.",
    )
    check("-D BIKE_MB_AI_USE_MOCK_PROVIDERS=1" not in cloud_env, "Cloud env must disable mock providers.")


if __name__ == "__main__":
    test_real_cloud_worker_accepts_capture_clip_and_uses_bailian()
    test_real_cloud_worker_submit_queues_jobs_instead_of_immediate_failure()
    test_real_cloud_worker_has_https_and_streaming_contract()
    print("PASS test_cloud_worker_real_contract")
