#!/usr/bin/env python3
from pathlib import Path
import shutil
import sys

BASE_DIR = Path(__file__).resolve().parent
MENU_FILE = BASE_DIR / "tools" / "menu.txt"
VOICE_DIR = BASE_DIR / "data" / "voice"
OUTPUT_DIR = BASE_DIR / "en"


def read_audio_names(menu_path: Path):
    names = []
    for line in menu_path.read_text(encoding="utf-8").splitlines():
        name = line.strip()
        if not name or name.startswith("#"):
            continue
        names.append(name)
    return names


def main() -> None:
    if not MENU_FILE.exists():
        print(f"Menu file not found: {MENU_FILE}")
        sys.exit(1)

    if not VOICE_DIR.exists():
        print(f"Voice directory not found: {VOICE_DIR}")
        sys.exit(1)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    names = read_audio_names(MENU_FILE)
    copied = 0

    for name in names:
        source = VOICE_DIR / f"{name}.mp3"
        if source.exists():
            shutil.copy2(source, OUTPUT_DIR / source.name)
            copied += 1
            print(f"Copied: {source.name}")
        else:
            print(f"Missing: {source.name}")

    print(f"\nDone. Copied {copied} files to {OUTPUT_DIR}")


if __name__ == "__main__":
    main()
