import subprocess
import tempfile
from pathlib import Path

from contract_helpers import REPO_ROOT, check, read_repo_text


CMAKE = "src/firmware/bikemb/src/CMakeLists.txt"
AI_CONFIG = "src/firmware/bikemb/src/ai/ai_config.h"
TTS_HEADER = "src/firmware/bikemb/src/ai/cosyvoice_tts_adapter.h"
TTS_SOURCE = "src/firmware/bikemb/src/ai/cosyvoice_tts_adapter.cpp"


def test_cosyvoice_tts_adapter_files_and_config_exist() -> None:
    check((REPO_ROOT / TTS_HEADER).exists(), "CosyVoice TTS adapter header must exist.")
    check((REPO_ROOT / TTS_SOURCE).exists(), "CosyVoice TTS adapter source must exist.")

    cmake = read_repo_text(CMAKE)
    config = read_repo_text(AI_CONFIG)

    check("ai/cosyvoice_tts_adapter.cpp" in cmake, "ESP-IDF CMake source list must include TTS adapter.")
    check("kCosyVoiceTtsEndpoint" in config, "Config must track the non-secret TTS endpoint.")
    check('kCosyVoiceTtsModel = "cosyvoice-v3-flash"' in config, "Config must track the TTS model.")
    check("api_key" not in config.lower(), "Tracked AI config must not contain API keys.")


def test_cosyvoice_tts_streams_sse_base64_pcm() -> None:
    header = read_repo_text(TTS_HEADER)
    source = read_repo_text(TTS_SOURCE)

    check("BikeMbCosyVoiceJsonSink" in header, "TTS adapter must expose a streaming JSON sink.")
    check("BikeMbCosyVoicePcmSink" in header, "TTS adapter must expose a PCM sink.")
    check("BikeMbCosyVoice_WriteRequestJson" in header, "TTS adapter must expose request JSON writer.")
    check("BikeMbCosyVoice_HandleSseLine" in header, "TTS adapter must expose SSE line handler.")
    check("BikeMbAiConfig::kCosyVoiceTtsModel" in source, "TTS request must include the configured model name.")
    check("16000" in source and "pcm" in source.lower(), "TTS request must ask for 16 kHz mono PCM.")
    check('\\"voice\\":\\"longanyang\\"' in source, "TTS request must set a valid Bailian CosyVoice v3 voice.")
    check('\\"input\\":{\\"text\\":\\"' in source, "TTS request must place synthesis options under input.")
    check('\\"parameters\\"' not in source, "CosyVoice HTTP request must not send synthesis options under parameters.")
    check("BikeMbCosyVoice_DecodeBase64Chunk" in source, "TTS adapter must decode Base64 audio chunks.")
    check("BikeMbCosyVoicePcmSink" in source, "TTS adapter must stream decoded PCM to a sink.")
    check('\\"data\\":\\"' in source, "TTS SSE parser must read audio.data chunks.")
    check("String audio" not in source, "TTS adapter must not build a full audio String.")
    check("std::string" not in source, "TTS adapter must not duplicate full TTS audio.")


def test_cosyvoice_tts_native_decoder_preserves_pcm_byte_pairs() -> None:
    with tempfile.TemporaryDirectory(prefix="bikemb-cosyvoice-") as temp_dir:
      output = Path(temp_dir) / "cosyvoice_tts_adapter_test.exe"
      subprocess.run(
          [
              "g++",
              "-std=c++17",
              "-Wall",
              "-Wextra",
              "-Werror",
              str(REPO_ROOT / TTS_SOURCE),
              str(REPO_ROOT / "tools" / "tests" / "cosyvoice_tts_adapter_test.cpp"),
              "-o",
              str(output),
          ],
          check=True,
          cwd=REPO_ROOT,
      )
      subprocess.run([str(output)], check=True, cwd=REPO_ROOT)


if __name__ == "__main__":
    test_cosyvoice_tts_adapter_files_and_config_exist()
    test_cosyvoice_tts_streams_sse_base64_pcm()
    test_cosyvoice_tts_native_decoder_preserves_pcm_byte_pairs()
    print("PASS test_cosyvoice_tts_contract")
