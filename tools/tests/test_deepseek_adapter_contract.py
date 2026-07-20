from contract_helpers import REPO_ROOT, check, read_repo_text


CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"
AI_CONFIG = "src/firmware/bikemb/src/ai/ai_config.h"
DEEPSEEK_HEADER = "src/firmware/bikemb/src/ai/deepseek_adapter.h"
DEEPSEEK_SOURCE = "src/firmware/bikemb/src/ai/deepseek_adapter.cpp"


def test_deepseek_adapter_files_and_config_exist() -> None:
    check((REPO_ROOT / DEEPSEEK_HEADER).exists(), "DeepSeek adapter header must exist.")
    check((REPO_ROOT / DEEPSEEK_SOURCE).exists(), "DeepSeek adapter source must exist.")

    cmake = read_repo_text(CMAKE)
    config = read_repo_text(AI_CONFIG)

    check("ai/deepseek_adapter.cpp" in cmake, "ESP-IDF CMake source list must include DeepSeek adapter.")
    check("kDeepSeekEndpoint" in config, "Config must track the non-secret DeepSeek endpoint.")
    check("kDeepSeekModel" in config, "Config must track the DeepSeek model.")
    check("kMaxAnswerBytes = 384" in config, "Config must cap LLM answers at 384 bytes.")
    check("api_key" not in config.lower(), "Tracked AI config must not contain API keys.")


def test_deepseek_adapter_limits_response_and_redacts_logs() -> None:
    header = read_repo_text(DEEPSEEK_HEADER)
    source = read_repo_text(DEEPSEEK_SOURCE)

    check("BikeMbDeepSeekJsonSink" in header, "DeepSeek adapter must expose a streaming JSON sink.")
    check("BikeMbDeepSeek_WriteRequestJson" in header, "DeepSeek adapter must expose request JSON writer.")
    check("BikeMbDeepSeek_CopyBoundedAnswer" in header, "DeepSeek adapter must expose answer length limiter.")
    check("BikeMbDeepSeek_RedactedLogLabel" in header, "DeepSeek adapter must expose redacted diagnostics.")
    check("BikeMbAiConfig::kMaxAnswerBytes" in source, "DeepSeek adapter must enforce max answer bytes.")
    check("max_tokens" in source, "DeepSeek request must bound provider output tokens.")
    check("messages" in source and "user" in source, "DeepSeek request must use chat messages.")
    check("question" not in source.lower(), "DeepSeek logs and diagnostics must not name or expose question text.")
    check("answer text" not in source.lower(), "DeepSeek logs and diagnostics must not expose answer text.")
    check("deepseek ready" in source and "deepseek failed" in source, "DeepSeek diagnostics must use sanitized labels.")


if __name__ == "__main__":
    test_deepseek_adapter_files_and_config_exist()
    test_deepseek_adapter_limits_response_and_redacts_logs()
    print("PASS test_deepseek_adapter_contract")
