from contract_helpers import REPO_ROOT, check, read_repo_text


CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"
AI_CONFIG = "src/firmware/bikemb/src/ai/ai_config.h"
QWEN_HEADER = "src/firmware/bikemb/src/ai/qwen_asr_adapter.h"
QWEN_SOURCE = "src/firmware/bikemb/src/ai/qwen_asr_adapter.cpp"


def test_qwen_asr_adapter_files_and_config_exist() -> None:
    check((REPO_ROOT / QWEN_HEADER).exists(), "Qwen ASR adapter header must exist.")
    check((REPO_ROOT / QWEN_SOURCE).exists(), "Qwen ASR adapter source must exist.")

    cmake = read_repo_text(CMAKE)
    config = read_repo_text(AI_CONFIG)

    check("ai/qwen_asr_adapter.cpp" in cmake, "ESP-IDF CMake source list must include Qwen ASR adapter.")
    check('kQwenAsrModel = "qwen3-asr-flash"' in config, "Config must track the Qwen ASR model name.")
    check("kQwenAsrEndpoint" in config, "Config must track the non-secret Qwen ASR endpoint.")
    check("api_key" not in config.lower(), "Tracked AI config must not contain API keys.")


def test_qwen_asr_adapter_streams_wav_base64_json() -> None:
    header = read_repo_text(QWEN_HEADER)
    source = read_repo_text(QWEN_SOURCE)

    check("BikeMbQwenAsrJsonSink" in header, "Qwen ASR adapter must expose a streaming JSON sink.")
    check("BikeMbQwenAsr_WriteRequestJson" in header, "Qwen ASR adapter must expose request JSON writer.")
    check("BikeMbQwenAsr_WriteWavHeader" in source, "Qwen ASR adapter must write a WAV header.")
    check("data:audio/wav;base64," in source, "Qwen ASR request must use a Base64 WAV data URL.")
    check(
        '\\"type\\":\\"input_audio\\"' in source and '\\"input_audio\\":{\\"data\\":\\"' in source,
        "Qwen ASR OpenAI-compatible request must use input_audio.data content items.",
    )
    check(
        '\\"asr_options\\":{\\"language\\":\\"zh\\",\\"enable_itn\\":false}' in source,
        "Qwen ASR OpenAI-compatible request must specify Chinese language and ASR options at top level.",
    )
    check("BikeMbQwenAsrBase64State" in source, "Qwen ASR adapter must keep Base64 carry across WAV and PCM chunks.")
    check("BikeMbQwenAsr_WriteBase64Chunk" in source, "Qwen ASR adapter must write Base64 in chunks.")
    check("BikeMbQwenAsr_WriteBase64Final" in source, "Qwen ASR adapter must only write Base64 padding at the end.")
    check(
        "BikeMbQwenAsr_WriteWavHeaderBase64" in source,
        "Qwen ASR adapter must Base64-encode the WAV header instead of writing binary JSON.",
    )
    check("BikeMbAiConfig::kQwenAsrModel" in source, "Qwen ASR request must include the configured model name.")
    check('\\"modalities\\"' not in source, "Qwen ASR request must not send chat-only modalities.")
    check('\\"audio\\":{\\"format\\":\\"wav\\"}' not in source, "Qwen ASR request must not send chat audio output options.")
    check("String base64" not in source, "Qwen ASR adapter must not build a full Base64 String.")
    check("std::string" not in source, "Qwen ASR adapter must not duplicate the full clip into std::string.")


if __name__ == "__main__":
    test_qwen_asr_adapter_files_and_config_exist()
    test_qwen_asr_adapter_streams_wav_base64_json()
    print("PASS test_qwen_asr_contract")
