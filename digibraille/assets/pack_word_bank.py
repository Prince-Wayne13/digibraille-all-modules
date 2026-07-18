#!/usr/bin/env python3
"""
pack_word_bank.py — PC-side word bank packer for DigiBraille v2.

Run as:
    python3 pack_word_bank.py <folder>

<folder> is a directory of individual word .mp3 clips, e.g. the SD card's
/words/en (or /words/ch) folder — one file per word, named "<word>.mp3".

Produces TWO files, written into <folder>'s PARENT directory (the SAME
level as the "en/"/"ch/" subfolder, NOT inside it):

    bank_<lang>.bin   every valid clip's bytes concatenated back-to-back
    bank_<lang>.bidx  a SORTED, FIXED-LENGTH BINARY index: one 32-byte
                       record per word, packed as:
                           24 bytes : word, ASCII, lowercase,
                                      NUL-padded/truncated to 23 chars
                                      + 1 terminator byte
                           4 bytes  : uint32 offset into bank.bin (little-endian)
                           4 bytes  : uint32 length in bytes      (little-endian)

<lang> is taken from the basename of the folder you point this at (e.g.
pointing it at ".../words/en" gives lang="en").

WHY BINARY, NOT TEXT (session 16 rework):
Earlier versions of this tool wrote bank.idx as plain CSV text
("word,offset,length" per line), and the ESP32 parsed the whole file into
a RAM array at boot for fast lookup. That RAM array does NOT scale: on
real hardware (327,680 bytes total RAM, ~247,580 free at boot on the
reference board), a ~9,629-word vocabulary needed ~301KB just for the
index array — more than was actually free, causing either a hard
out-of-memory crash-reboot loop (see log_v_2.md session 15) or forced
partial/truncated coverage even after that crash was fixed.

The fixed-length BINARY format lets the device binary-search the index
FILE DIRECTLY ON THE SD CARD (seek to record N*32, read 32 bytes, compare,
repeat) instead of loading anything into RAM at all. RAM cost becomes a
small constant regardless of vocabulary size. This is the same principle
already used for bank.bin's AUDIO data (seek+read from an open handle
rather than loading it whole) — session 16 just applies it to the index
too.

WHY A PC-SIDE TOOL, NEVER ON-DEVICE:
Same reasoning as every other build artifact in this project (note audio
chains, etc.): building bank.bin/bank.bidx requires writing large files
in one pass; doing that on the ESP32 itself risks silent corruption if
power is lost mid-write. This tool is meant to be run on a PC, with the
two output files then copied onto the SD card by hand (or via a script)
before the device boots.

CLIP SIZE CAP: any clip over MAX_CLIP_BYTES (or exactly 0 bytes) is
SKIPPED from the bank (with a warning, not an abort) — it stays as an
individual file only, and the on-device per-file fallback still finds and
plays it normally, just without the bank's speed benefit for that one
word. MAX_CLIP_BYTES here MUST stay in sync with WORD_CLIP_MAX_BYTES in
mp3_player.cpp — the packer decides what goes in the bank, the device
decides what fits in its RAM playback buffer; a mismatch would let the
packer include a clip the device then can't safely play from the bank.
"""

import os
import sys
import struct

MAX_CLIP_BYTES = 15360        # 15KB — MUST match WORD_CLIP_MAX_BYTES in mp3_player.cpp
BANK_WORD_MAX_LEN = 23        # usable characters; +1 terminator byte = 24-byte field
BANK_RECORD_SIZE = 24 + 4 + 4 # word field + offset (uint32) + length (uint32) = 32 bytes

RECORD_STRUCT = struct.Struct("<24sII")  # little-endian: 24-byte field, uint32, uint32
assert RECORD_STRUCT.size == BANK_RECORD_SIZE


def pack_word_bank(folder: str):
    folder = os.path.abspath(folder)
    if not os.path.isdir(folder):
        print(f"ERROR: not a directory: {folder}")
        sys.exit(1)

    lang = os.path.basename(folder.rstrip(os.sep))
    if not lang:
        print(f"ERROR: could not derive a language tag from folder path: {folder}")
        sys.exit(1)

    parent = os.path.dirname(folder)
    bin_path = os.path.join(parent, f"bank_{lang}.bin")
    bidx_path = os.path.join(parent, f"bank_{lang}.bidx")

    entries = []  # (word, offset, length)
    skipped_oversize = []
    skipped_empty = []
    skipped_too_long_name = []

    offset = 0
    clip_files = sorted(f for f in os.listdir(folder) if f.lower().endswith(".mp3"))

    if not clip_files:
        print(f"ERROR: no .mp3 files found in {folder}")
        sys.exit(1)

    print(f"Packing {len(clip_files)} clip(s) from {folder} ...")

    with open(bin_path, "wb") as bin_out:
        for fname in clip_files:
            word = os.path.splitext(fname)[0].lower()
            fpath = os.path.join(folder, fname)
            size = os.path.getsize(fpath)

            if size == 0:
                skipped_empty.append(fname)
                continue
            if size > MAX_CLIP_BYTES:
                skipped_oversize.append((fname, size))
                continue
            if len(word) > BANK_WORD_MAX_LEN:
                skipped_too_long_name.append(word)
                continue

            with open(fpath, "rb") as f:
                data = f.read()
            bin_out.write(data)
            entries.append((word, offset, size))
            offset += size

    # Sort by word (ASCII/byte order) so the device can binary-search the
    # index file directly — this ordering is load-bearing, not cosmetic.
    entries.sort(key=lambda e: e[0])

    with open(bidx_path, "wb") as idx_out:
        for word, entry_offset, entry_length in entries:
            word_bytes = word.encode("ascii", errors="ignore")[:BANK_WORD_MAX_LEN]
            # Pad/truncate to exactly 24 bytes, NUL-terminated within that field.
            field = word_bytes + b"\x00" * (24 - len(word_bytes))
            idx_out.write(RECORD_STRUCT.pack(field, entry_offset, entry_length))

    # ── Summary ──────────────────────────────────────────────
    print(f"\nWrote {bin_path} ({offset} bytes)")
    print(f"Wrote {bidx_path} ({len(entries)} entries, {len(entries) * BANK_RECORD_SIZE} bytes, "
          f"{BANK_RECORD_SIZE} bytes/record)")

    if skipped_oversize:
        print(f"\nSkipped {len(skipped_oversize)} clip(s) over {MAX_CLIP_BYTES} bytes "
              f"(stay as individual files, no bank speed benefit):")
        for fname, size in skipped_oversize:
            print(f"  {fname}  ({size} bytes)")

    if skipped_empty:
        print(f"\nSkipped {len(skipped_empty)} empty (0-byte) clip(s):")
        for fname in skipped_empty:
            print(f"  {fname}")

    if skipped_too_long_name:
        print(f"\nSkipped {len(skipped_too_long_name)} word(s) longer than "
              f"{BANK_WORD_MAX_LEN} characters (name won't fit the fixed record field):")
        for word in skipped_too_long_name:
            print(f"  {word}")

    print(f"\n── Next steps ──────────────────────────────────────────")
    print(f"Copy BOTH files to the SD card's /words/ folder — the SAME level")
    print(f"as the '{lang}/' subfolder, NOT inside it:")
    print(f"  {os.path.basename(bin_path)}")
    print(f"  {os.path.basename(bidx_path)}")
    print(f"Then on the device: reboot (reloads the bank) or run the")
    print(f"'reindex' serial command, then 'rechain' if you added NEW words")
    print(f"not previously covered by any chain.")


def _self_test():
    """
    Lightweight correctness check, run with --self-test. Packs a small
    synthetic folder and verifies every .bidx record's offset/length
    slices .bin into bytes identical to the source clip, and that a
    manual binary search over the written records finds the right word.
    Does not touch real project files.
    """
    import tempfile
    import random

    with tempfile.TemporaryDirectory() as tmp:
        src_dir = os.path.join(tmp, "en")
        os.makedirs(src_dir)

        words = {
            "grace": 4535,
            "poverty": 4859,
            "a": 3671,
            "zomba": 6000,
            "toolongwordthatexceedsthetwentythreecharacterlimitxx": 1000,  # too long
        }
        expected = {}
        for word, size in words.items():
            data = bytes(random.randint(0, 255) for _ in range(size))
            with open(os.path.join(src_dir, f"{word}.mp3"), "wb") as f:
                f.write(data)
            if len(word) <= BANK_WORD_MAX_LEN:
                expected[word] = data

        # oversized clip — should be skipped
        big_data = bytes(random.randint(0, 255) for _ in range(MAX_CLIP_BYTES + 1))
        with open(os.path.join(src_dir, "toobig.mp3"), "wb") as f:
            f.write(big_data)

        pack_word_bank(src_dir)

        bin_path = os.path.join(tmp, "bank_en.bin")
        bidx_path = os.path.join(tmp, "bank_en.bidx")

        with open(bidx_path, "rb") as f:
            idx_bytes = f.read()
        assert len(idx_bytes) % BANK_RECORD_SIZE == 0, "index size not a multiple of record size"
        count = len(idx_bytes) // BANK_RECORD_SIZE
        assert count == len(expected), f"expected {len(expected)} entries, got {count}"

        with open(bin_path, "rb") as f:
            bin_bytes = f.read()

        records = []
        for i in range(count):
            rec = idx_bytes[i * BANK_RECORD_SIZE:(i + 1) * BANK_RECORD_SIZE]
            word_field, off, length = RECORD_STRUCT.unpack(rec)
            word = word_field.split(b"\x00", 1)[0].decode("ascii")
            records.append((word, off, length))
            assert bin_bytes[off:off + length] == expected[word], f"byte mismatch for {word}"

        # sortedness check — required for on-device binary search to work
        words_only = [r[0] for r in records]
        assert words_only == sorted(words_only), "records not sorted by word"

        # simple binary search re-implementation, mirroring the device logic
        def binary_search(target):
            lo, hi = 0, len(records)
            while lo < hi:
                mid = (lo + hi) // 2
                w = records[mid][0]
                if w == target:
                    return records[mid]
                elif target < w:
                    hi = mid
                else:
                    lo = mid + 1
            return None

        for word in expected:
            found = binary_search(word)
            assert found is not None and found[0] == word, f"binary search failed for {word}"
        assert binary_search("nonexistentword") is None

        print("\nSelf-test PASSED: offsets/lengths byte-exact, sorted, binary search correct.")


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--self-test":
        _self_test()
    elif len(sys.argv) == 2:
        pack_word_bank(sys.argv[1])
    else:
        print(__doc__)
        sys.exit(1)