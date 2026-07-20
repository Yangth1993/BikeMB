#!/usr/bin/env python3
"""Save a CosyVoice SSE response as a local WAV diagnostic file."""

from __future__ import annotations

import argparse
import base64
import json
import os
import re
import ssl
import sys
import urllib.error
import urllib.request
import wave
from pathlib import Path
from typing import Iterable, Iterator


REPO_ROOT = Path(__file__).resolve().parents[1]
LOCAL_SECRETS = REPO_ROOT / "src" / "firmware" / "bikemb" / "include" / "ai_secrets.local.h"
DEFAULT_ENDPOINT = "https://dashscope.aliyuncs.com/api/v1/services/audio/tts/SpeechSynthesizer"
DEFAULT_MODEL = "cosyvoice-v3-flash"
DEFAULT_VOICE = "longanyang"
DEFAULT_SAMPLE_RATE = 16000


def parse_macro(header: str, name: str) -> str | None:
    pattern = re.compile(r"^\s*#define\s+" + re.escape(name) + r'\s+"([^"]*)"', re.MULTILINE)
    match = pattern.search(header)
    if not match:
        return None
    value = match.group(1)
    if not value or value.startswith("CHANGE_ME"):
        return None
    return value


def load_dashscope_token() -> str:
    for name in ("BIKE_MB_AI_DASHSCOPE_TOKEN", "DASHSCOPE_API_KEY", "BIKE_MB_AI_TTS_TOKEN"):
        value = os.environ.get(name)
        if value and not value.startswith("CHANGE_ME"):
            return value

    if LOCAL_SECRETS.exists():
        header = LOCAL_SECRETS.read_text(encoding="utf-8")
        for name in ("BIKE_MB_AI_DASHSCOPE_TOKEN", "BIKE_MB_AI_TTS_TOKEN", "BIKE_MB_AI_STT_TOKEN"):
            value = parse_macro(header, name)
            if value:
                return value

    raise RuntimeError(
        "DashScope token not found. Set DASHSCOPE_API_KEY or create ai_secrets.local.h."
    )


def make_request_json(text: str, model: str, voice: str, sample_rate: int) -> bytes:
    payload = {
        "model": model,
        "input": {
            "text": text,
            "voice": voice,
            "format": "pcm",
            "sample_rate": sample_rate,
        },
    }
    return json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


def iter_audio_data(value) -> Iterator[str]:
    if isinstance(value, dict):
        audio = value.get("audio")
        if isinstance(audio, dict) and isinstance(audio.get("data"), str):
            yield audio["data"]
        for child in value.values():
            yield from iter_audio_data(child)
    elif isinstance(value, list):
        for child in value:
            yield from iter_audio_data(child)


def normalize_audio_data(data: str) -> str:
    if data.startswith("data:") and "," in data:
        return data.split(",", 1)[1]
    return data


def extract_audio_chunks(lines: Iterable[str]) -> Iterator[str]:
    for raw_line in lines:
        line = raw_line.strip()
        if not line or line.startswith("event:"):
            continue
        if line.startswith("data:"):
            line = line[5:].strip()
        if line == "[DONE]":
            continue
        try:
            payload = json.loads(line)
        except json.JSONDecodeError:
            for match in re.finditer(r'"data"\s*:\s*"([^"]+)"', line):
                yield normalize_audio_data(match.group(1))
            continue
        for data in iter_audio_data(payload):
            yield normalize_audio_data(data)


def write_pcm_wav(path: Path, pcm_bytes: bytes, sample_rate: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        wav.writeframes(pcm_bytes)


def request_cosyvoice_lines(
    endpoint: str,
    token: str,
    request_body: bytes,
    timeout: float,
    insecure: bool,
) -> Iterator[str]:
    request = urllib.request.Request(
        endpoint,
        data=request_body,
        method="POST",
        headers={
            "Authorization": f"Bearer {token}",
            "Content-Type": "application/json",
            "Accept": "text/event-stream",
            "X-DashScope-SSE": "enable",
        },
    )
    context = ssl._create_unverified_context() if insecure else None
    with urllib.request.urlopen(request, timeout=timeout, context=context) as response:
        for line in response:
            yield line.decode("utf-8", errors="replace")


def save_cosyvoice_wav(args: argparse.Namespace) -> int:
    token = load_dashscope_token()
    request_body = make_request_json(args.text, args.model, args.voice, args.sample_rate)
    pcm = bytearray()
    chunk_count = 0

    try:
        lines = request_cosyvoice_lines(
            args.endpoint, token, request_body, args.timeout, args.insecure
        )
        for chunk in extract_audio_chunks(lines):
            pcm.extend(base64.b64decode(chunk))
            chunk_count += 1
    except urllib.error.HTTPError as error:
        body = error.read().decode("utf-8", errors="replace")
        print(f"cosyvoice HTTP {error.code}: {body}", file=sys.stderr)
        return 1

    if not pcm:
        print("No audio.data chunks found in CosyVoice response.", file=sys.stderr)
        return 1
    if len(pcm) % 2 != 0:
        print(f"PCM byte count is odd: {len(pcm)}", file=sys.stderr)
        return 1

    args.pcm_out.parent.mkdir(parents=True, exist_ok=True)
    args.pcm_out.write_bytes(pcm)
    write_pcm_wav(args.wav_out, bytes(pcm), args.sample_rate)
    duration_s = len(pcm) / 2 / args.sample_rate
    print(
        "saved "
        f"chunks={chunk_count} pcm_bytes={len(pcm)} duration_s={duration_s:.2f} "
        f"pcm={args.pcm_out} wav={args.wav_out}"
    )
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--text", required=True, help="Text to synthesize.")
    parser.add_argument("--wav-out", type=Path, default=REPO_ROOT / "build" / "cosyvoice-test.wav")
    parser.add_argument("--pcm-out", type=Path, default=REPO_ROOT / "build" / "cosyvoice-test.pcm")
    parser.add_argument("--endpoint", default=DEFAULT_ENDPOINT)
    parser.add_argument("--model", default=DEFAULT_MODEL)
    parser.add_argument("--voice", default=DEFAULT_VOICE)
    parser.add_argument("--sample-rate", type=int, default=DEFAULT_SAMPLE_RATE)
    parser.add_argument("--timeout", type=float, default=60.0)
    parser.add_argument("--insecure", action="store_true", help="Disable TLS verification.")
    return parser.parse_args()


def main() -> int:
    return save_cosyvoice_wav(parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
