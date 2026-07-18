#pragma once
#include <Arduino.h>

// MP3 + VoiceRSS — function declarations only
// Implementation is in mp3_player.cpp which includes ESP8266Audio in isolation

void mp3Setup();
void playMP3(const String& path);
bool isValidMp3(String path);
bool vrssFetch(const String& text, const String& path);
void speakPhrase(const char* name);
void speakText(const String& text, const String& mp3Path);
void speakCharacter(char c);
void cacheMenuAudio();
void audioFetchTask(void* param);

// Scans the current-language /words/<lang> folder once and keeps every
// filename in an in-RAM sorted index, so word lookups become an instant
// binary search instead of a blocking SD probe. Call once at boot (after
// SD mounts) and again on language change or after adding new SD assets
// (see the "reindex" serial command in main.cpp). See mp3_player.cpp for
// the full rationale (session 9 investigation into per-word playback gaps).
void buildWordAssetIndex();

// Loads the packed word bank (/words/bank_<lang>.bin + bank_<lang>.bidx),
// if present, so word lookups become a small on-card binary search
// instead of a per-word filesystem open. Built by the PC-side
// tools/pack_word_bank.py script — never on-device.
//
// As of session 16, the .bidx index is a SORTED, FIXED-LENGTH BINARY
// record file (32 bytes/entry: 24-byte word + 4-byte offset + 4-byte
// length) that is searched directly ON THE SD CARD via seek()+read() —
// it is NOT parsed into a RAM array. A lookup costs ceil(log2(N)) small
// seek+reads (14 for ~9,629 entries) against an already-open handle,
// rather than one large in-memory list. This removes the RAM ceiling the
// previous (in-RAM vector) version hit — measured on real hardware, a
// 9,629-entry list needed ~301KB, more than the ~247KB typically free at
// boot on this board (see log_v_2.md, sessions 15-16). Vocabulary can now
// grow arbitrarily large with no additional RAM cost, only ~1 extra seek
// per doubling in size.
//
// Purely an accelerator: if the bank files are missing/malformed/wrong
// format, resolution falls back to the per-file index
// (buildWordAssetIndex) automatically. Call once at boot (after
// buildWordAssetIndex()) and again on language change or after copying a
// freshly-repacked bank onto the card (see the "reindex" serial command
// in main.cpp, which calls both). Returns false if no usable bank was
// loaded (not an error — just "not present").
bool loadWordBank();

// Status accessors for the "assets" serial diagnostic command.
bool wordBankIsLoaded();
size_t wordBankEntryCount();

// ─── NOTE AUDIO CHAINS ─────────────────────────────────────────
// A note's body is read back via a pre-built "chain": a small text plan
// (one line per word/letter/space unit) written once at save time,
// listing exactly which asset to play for each unit — resolved once
// against the SD card's asset library instead of being re-resolved live
// on every open. See mp3_player.cpp for the on-disk format.

// Builds and writes the chain file for note `index`'s body text. Call
// this once, right after a note is saved (replaces the old, non-functional
// queueNoteAudio() call for this purpose). Returns false if the chain
// could not be written (e.g. no writable storage available).
bool buildNoteAudioChain(int index, const String& body);

// Same as buildNoteAudioChain() but for a note's title text. Lets a title
// like "Grace" be read from recorded assets (whole-word "grace.mp3" if it
// exists, else spelled letter-by-letter) via the identical chain format,
// instead of depending on a never-generated note{N}_title.mp3.
bool buildNoteTitleChain(int index, const String& title);

// Plays note `index`'s previously-built chain, straight through, unit by
// unit, stopping cleanly if a control button interrupts (same debounced
// interrupt behavior as playMP3 — check lastInterruptPin afterward).
// Returns false if no chain file exists yet for this note (e.g. an older
// note saved before this feature existed) — caller should fall back to
// something else (currently: speak the title only) in that case.
bool playNoteAudioChain(int index);

// Plays note `index`'s previously-built TITLE chain (see
// buildNoteTitleChain). Same interrupt-aware, unit-by-unit behavior as
// playNoteAudioChain. Returns false if no title chain exists yet.
bool playNoteTitleChain(int index);

// True if a chain file exists for this note (regardless of whether it's
// been played). Lets callers decide "is there body audio to play at all"
// without actually starting playback.
bool noteAudioChainExists(int index);

// True if a TITLE chain file exists for this note (see buildNoteTitleChain).
// Title chains are migrated at boot alongside body chains, so this is
// generally true for every saved note; callers use it to decide whether to
// play the title chain or fall back to live asset/SAM spelling.
bool noteTitleChainExists(int index);

// Which control pin (a BTN_* value from globals.h) cut short the most
// recent playMP3() call, or -1 if it finished naturally / hasn't run yet.
// Read this immediately after calling playMP3() — it's overwritten by the
// next call.
extern int lastInterruptPin;