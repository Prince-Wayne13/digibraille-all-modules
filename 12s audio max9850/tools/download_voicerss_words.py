#!/usr/bin/env python3
"""
Download MP3 pronunciations for words in a text or CSV file using Voice RSS.

Word list format:
  - one word or short phrase per line
  - blank lines are ignored
  - lines starting with # are ignored
  - CSV files use the "english" column by default
  - slash-separated CSV values are expanded into separate words

Examples:
  python tools/download_voicerss_words.py tools/english_common_words_5000.txt --api-key YOUR_KEY
  $env:VOICE_RSS_API_KEY="YOUR_KEY"; python tools/download_voicerss_words.py tools/english_common_words_5000.txt
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path


API_URL = "https://api.voicerss.org/"


def safe_filename(text: str) -> str:
    name = text.strip().lower()
    name = re.sub(r"[^a-z0-9]+", "_", name)
    name = name.strip("_")
    return name or "word"


def add_word(words: list[str], seen: set[str], word: str) -> None:
    word = word.strip()
    if not word or word.startswith("#"):
        return

    key = word.casefold()
    if key in seen:
        return

    seen.add(key)
    words.append(word)


def expand_csv_word(value: str) -> list[str]:
    value = value.strip()
    if "/" not in value:
        return [value]

    expanded: list[str] = []
    for part in value.split("/"):
        expanded.extend(part.strip().split())
    return expanded


def read_words(path: Path, csv_column: str) -> list[str]:
    words: list[str] = []
    seen: set[str] = set()

    if path.suffix.casefold() == ".csv":
        with path.open("r", encoding="utf-8-sig", newline="") as handle:
            reader = csv.DictReader(handle)
            if not reader.fieldnames:
                return words

            if csv_column not in reader.fieldnames:
                columns = ", ".join(reader.fieldnames)
                raise ValueError(f'CSV column "{csv_column}" not found. Available columns: {columns}')

            for row in reader:
                for word in expand_csv_word(row.get(csv_column, "")):
                    add_word(words, seen, word)

        return words

    for raw_line in path.read_text(encoding="utf-8-sig").splitlines():
        add_word(words, seen, raw_line)

    return words


def download_word(
    word: str,
    destination: Path,
    *,
    api_key: str,
    language: str,
    voice: str | None,
    audio_format: str,
    codec: str,
    rate: int,
    timeout: int,
) -> None:
    params = {
        "key": api_key,
        "src": word,
        "hl": language,
        "c": codec,
        "f": audio_format,
        "r": str(rate),
    }

    if voice:
        params["v"] = voice

    url = f"{API_URL}?{urllib.parse.urlencode(params)}"
    request = urllib.request.Request(url, headers={"User-Agent": "digibraille-voicerss-downloader/1.0"})

    with urllib.request.urlopen(request, timeout=timeout) as response:
        data = response.read()
        content_type = response.headers.get("Content-Type", "")

    if data.startswith(b"ERROR") or b"text/plain" in content_type.encode("ascii", "ignore"):
        message = data.decode("utf-8", errors="replace").strip()
        raise RuntimeError(message)

    destination.write_bytes(data)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download MP3 files for a word list from Voice RSS."
    )
    parser.add_argument(
        "wordlist",
        type=Path,
        nargs="?",
        default=Path("tools") / "english_common_words_5000.txt",
        help="Text file or CSV file containing words. Default: tools/english_common_words_5000.txt",
    )
    parser.add_argument(
        "--csv-column",
        default="english",
        help='CSV column to use when wordlist is a CSV file. Default: "english"',
    )
    parser.add_argument(
        "-o",
        "--output-dir",
        type=Path,
        default=Path("data") / "voice",
        help="Folder for downloaded MP3 files. Default: data/voice",
    )
    parser.add_argument(
        "--api-key",
        default=os.getenv("VOICE_RSS_API_KEY"),
        help="Voice RSS API key. Can also be set with VOICE_RSS_API_KEY.",
    )
    parser.add_argument("--language", default="en-us", help="Voice RSS language code. Default: en-us")
    parser.add_argument("--voice", default=None, help="Optional Voice RSS voice name, for example Linda.")
    parser.add_argument("--codec", default="MP3", help="Audio codec. Default: MP3")
    parser.add_argument(
        "--format",
        dest="audio_format",
        default="16khz_16bit_mono",
        help="Audio format. Default: 16khz_16bit_mono",
    )
    parser.add_argument("--rate", type=int, default=0, help="Speech rate from -10 to 10. Default: 0")
    parser.add_argument("--delay", type=float, default=0.25, help="Delay between requests. Default: 0.25")
    parser.add_argument("--timeout", type=int, default=30, help="HTTP timeout in seconds. Default: 30")
    parser.add_argument("--overwrite", action="store_true", help="Download again even if the MP3 exists.")
    parser.add_argument(
        "--resume",
        action="store_true",
        help="Skip existing completed downloads and continue from where the last run left off.",
    )
    parser.add_argument("--dry-run", action="store_true", help="Print planned downloads without requesting audio.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if not args.api_key and not args.dry_run:
        print("Missing Voice RSS API key. Pass --api-key or set VOICE_RSS_API_KEY.", file=sys.stderr)
        return 2

    if not args.wordlist.exists():
        print(f"Word list not found: {args.wordlist}", file=sys.stderr)
        return 2

    try:
        words = read_words(args.wordlist, args.csv_column)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 2
    args.output_dir.mkdir(parents=True, exist_ok=True)

    if not words:
        print("No words found.")
        return 0

    print(f"Found {len(words)} word(s). Output: {args.output_dir}")

    failures = 0
    for index, word in enumerate(words, start=1):
        destination = args.output_dir / f"{safe_filename(word)}.mp3"

        if destination.exists() and not args.overwrite:
            if args.resume and destination.stat().st_size > 0:
                print(f"[{index}/{len(words)}] skip existing: {destination.name}")
                continue
            if destination.stat().st_size == 0:
                print(f"[{index}/{len(words)}] retry incomplete: {destination.name}")
            elif not args.resume:
                print(f"[{index}/{len(words)}] skip existing: {destination.name}")
                continue

        if args.dry_run:
            print(f"[{index}/{len(words)}] would download: {word!r} -> {destination.name}")
            continue

        try:
            download_word(
                word,
                destination,
                api_key=args.api_key,
                language=args.language,
                voice=args.voice,
                audio_format=args.audio_format,
                codec=args.codec,
                rate=args.rate,
                timeout=args.timeout,
            )
            print(f"[{index}/{len(words)}] downloaded: {word!r} -> {destination.name}")
        except Exception as exc:
            failures += 1
            print(f"[{index}/{len(words)}] failed: {word!r}: {exc}", file=sys.stderr)

        if args.delay > 0 and index < len(words):
            time.sleep(args.delay)

    if failures:
        print(f"Done with {failures} failure(s).", file=sys.stderr)
        return 1

    print("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
