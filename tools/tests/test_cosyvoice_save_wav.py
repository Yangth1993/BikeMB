import importlib.util
import wave
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "cosyvoice_save_wav.py"


def load_module():
    spec = importlib.util.spec_from_file_location("cosyvoice_save_wav", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_extracts_audio_data_from_sse_json():
    module = load_module()

    chunks = list(
        module.extract_audio_chunks(
            [
                'event: result\n',
                'data: {"output":{"audio":{"data":"AQACAAMA"}}}\n',
            ]
        )
    )

    assert chunks == ["AQACAAMA"]


def test_writes_16khz_mono_pcm_wav(tmp_path):
    module = load_module()
    wav_path = tmp_path / "tts.wav"

    module.write_pcm_wav(wav_path, b"\x01\x00\x02\x00\x03\x00", 16000)

    with wave.open(str(wav_path), "rb") as wav:
        assert wav.getnchannels() == 1
        assert wav.getsampwidth() == 2
        assert wav.getframerate() == 16000
        assert wav.getnframes() == 3
        assert wav.readframes(3) == b"\x01\x00\x02\x00\x03\x00"


def main():
    test_extracts_audio_data_from_sse_json()
    tmp_dir = REPO_ROOT / "build" / "test-cosyvoice-save-wav"
    tmp_dir.mkdir(parents=True, exist_ok=True)
    test_writes_16khz_mono_pcm_wav(tmp_dir)
    print("PASS cosyvoice_save_wav")


if __name__ == "__main__":
    main()
