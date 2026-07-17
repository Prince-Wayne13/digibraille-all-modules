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