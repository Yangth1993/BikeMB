from contract_helpers import REPO_ROOT, check, read_repo_text


CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"
AI_CONFIG = "src/firmware/bikemb/src/ai/ai_config.h"
QWEN_HEADER = "src/firmware/bikemb/src/ai/qwen_chat_adapter.h"
QWEN_SOURCE = "src/firmware/bikemb/src/ai/qwen_chat_adapter.cpp"


def test_qwen_chat_adapter_files_and_config_exist() -> None:
    check((REPO_ROOT / QWEN_HEADER).exists(), "Qwen chat adapter header must exist.")
    check((REPO_ROOT / QWEN_SOURCE).exists(), "Qwen chat adapter source must exist.")

    cmake = read_repo_text(CMAKE)
    config = read_repo_text(AI_CONFIG)

    check("ai/qwen_chat_adapter.cpp" in cmake, "ESP-IDF CMake source list must include Qwen chat adapter.")
    check("kQwenChatEndpoint" in config, "Config must track the non-secret Qwen chat endpoint.")
    check('kQwenChatModel = "qwen-plus"' in config, "Config must default to Qwen chat on Bailian.")
    check("kMaxAnswerBytes = 384" in config, "Config must cap Qwen answers to short spoken replies.")
    check("api_key" not in config.lower(), "Tracked AI config must not contain API keys.")


def test_qwen_chat_adapter_limits_response_and_redacts_logs() -> None:
    header = read_repo_text(QWEN_HEADER)
    source = read_repo_text(QWEN_SOURCE)

    check("BikeMbQwenChatJsonSink" in header, "Qwen chat adapter must expose a streaming JSON sink.")
    check("BikeMbQwenChat_WriteRequestJson" in header, "Qwen chat adapter must expose request JSON writer.")
    check("BikeMbQwenChat_CopyBoundedAnswer" in header, "Qwen chat adapter must expose answer limiter.")
    check("BikeMbQwenChat_RedactedLogLabel" in header, "Qwen chat adapter must expose redacted diagnostics.")
    check("BikeMbAiConfig::kQwenChatModel" in source, "Qwen request must include configured model.")
    check("BikeMbAiConfig::kMaxAnswerBytes" in source, "Qwen adapter must enforce max answer bytes.")
    check("messages" in source and "user" in source, "Qwen request must use chat messages.")
    check("max_tokens" in source, "Qwen request must bound provider output tokens.")
    check("30 Chinese characters" in source, "Qwen system prompt must constrain spoken reply length.")
    check('\\"max_tokens\\":64' in source, "Qwen request must keep spoken replies short.")
    check("question" not in source.lower(), "Qwen diagnostics must not name or expose question text.")
    check("answer text" not in source.lower(), "Qwen diagnostics must not expose answer text.")
    check("qwen chat ready" in source and "qwen chat failed" in source, "Qwen diagnostics must be sanitized.")


if __name__ == "__main__":
    test_qwen_chat_adapter_files_and_config_exist()
    test_qwen_chat_adapter_limits_response_and_redacts_logs()
    print("PASS test_qwen_chat_contract")
