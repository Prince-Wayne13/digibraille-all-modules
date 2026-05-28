import argparse
import shutil
import subprocess
from pathlib import Path


def build_atempo_filter(factor: float) -> str:
    # ffmpeg atempo supports chains of 0.5-2.0 each; build a chain for arbitrary factor
    if factor <= 0:
        raise ValueError("Speed factor must be greater than 0")
    filters = []
    remaining = factor
    while remaining > 2.0:
        filters.append("atempo=2.0")
        remaining /= 2.0
    while remaining < 0.5:
        filters.append("atempo=0.5")
        remaining *= 2.0
    filters.append(f"atempo={remaining:.6f}")
    return ",".join(filters)


def speedup_file(input_path: Path, output_path: Path, factor: float, samplerate: int) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    filter_chain = build_atempo_filter(factor)
    command = [
        "ffmpeg",
        "-y",
        "-i",
        str(input_path),
        "-filter:a",
        filter_chain,
        "-ar",
        str(samplerate),
        "-vn",
        str(output_path),
    ]
    subprocess.run(command, check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description="Speed up MP3 files from data/sfx/ch into data/sfx/chichewa.")
    parser.add_argument(
        "--factor",
        type=float,
        default=1.25,
        help="Speed-up factor (e.g. 1.25 means 25%% faster).",
    )
    parser.add_argument(
        "--samplerate",
        type=int,
        default=22050,
        help="Output sample rate in Hz (default: 22050).",
    )
    parser.add_argument(
        "--source",
        type=Path,
        default=Path("data/sfx/ch"),
        help="Source folder containing original MP3 files.",
    )
    parser.add_argument(
        "--dest",
        type=Path,
        default=Path("data/sfx/chichewa"),
        help="Destination folder for sped-up MP3 files.",
    )
    args = parser.parse_args()

    if shutil.which("ffmpeg") is None:
        raise SystemExit(
            "ffmpeg not found on PATH. Install ffmpeg and make sure it is available as 'ffmpeg'."
        )

    if not args.source.exists() or not args.source.is_dir():
        raise SystemExit(f"Source folder does not exist: {args.source}")

    mp3_files = sorted(args.source.rglob("*.mp3"))
    if not mp3_files:
        raise SystemExit(f"No MP3 files found in {args.source}")

    print(f"Speeding up {len(mp3_files)} MP3 file(s) by factor {args.factor} and resampling to {args.samplerate} Hz...")
    for input_path in mp3_files:
        relative_path = input_path.relative_to(args.source)
        output_path = args.dest / relative_path
        print(f"- {input_path} -> {output_path}")
        speedup_file(input_path, output_path, args.factor, args.samplerate)

    print(f"Done. Sped-up files written to: {args.dest}")


if __name__ == "__main__":
    main()
