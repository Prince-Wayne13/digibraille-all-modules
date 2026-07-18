// ============================================================
//  MP3 PLAYER + VOICERSS — ESP8266Audio only
//  NO AudioTools in this file — ever
//  Uses I2S output for MAX98357A/MAX9857A-style amplifier boards
// ============================================================
#include "mp3_player.h"
#include "globals.h"
#include "sam_tts.h"
#include <SD.h>
#include <functional>
#include "freertos/semphr.h"
#include "AudioFileSource.h"
#include "AudioFileSourceLittleFS.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

extern volatile bool audioFetchIdle;

#include <vector>
#include <algorithm>
#include <cstring>

// Forward declaration — buildWordAssetIndex() (defined below, ahead of the
// rest of the file) needs this, but its real definition lives further down
// near the other asset-resolution helpers (audioLangFolder/resolveAudioAsset
// etc.), after the word-bank and word-asset-index sections. Declaring it
// here once means neither section has to be reordered to satisfy the other.
static String audioLangFolder();

// ─── WORD BANK (packed blob storage, on-SD binary-search index) ─────
// Optional, opt-in file pair: /words/bank_<lang>.bin (every word clip's
// bytes concatenated back-to-back) + /words/bank_<lang>.bidx (a SORTED,
// FIXED-LENGTH BINARY record file — NOT text — one 32-byte record per
// word: char word[24] + uint32_t offset + uint32_t length). Built by a
// PC-side script (tools/pack_word_bank.py) — never on-device, same
// reasoning as every other build-artifact in this project: avoid
// power-loss-mid-write corruption risk on the ESP32 itself.
//
// Why the bank exists at all: each word used to be a SEPARATE file, and
// opening a NEW file by name pays a directory-position-dependent cost
// (measured 35ms-6500ms on this card) every time a word hasn't already
// been prefetched. The bank collapses "hundreds of small files" into
// "one file, opened once at boot" for the audio itself (bank.bin).
//
// Why the index is now searched ON THE SD CARD instead of loaded into
// RAM (session 16 rework — see log_v_2.md): the previous version parsed
// the whole word list into a std::vector<WordBankEntry> at boot. On a
// real board (measured: 327,680 bytes total RAM, ~247,580 free at boot),
// the full 9,629-entry list needed ~301KB (32 bytes/entry) — more than
// the free heap, even before the 40KB safety margin. That either crashed
// (session 15's original bug) or silently truncated coverage (session
// 15's fix). A RAM array fundamentally cannot scale past whatever heap
// happens to be free at boot.
//
// The fix: keep the index AS a sorted binary file on the SD card and
// binary-search it directly via seek()+read(), the same way bank.bin's
// AUDIO is already accessed — never load the whole list into memory.
// Each fixed-size record is 32 bytes, so "record N" is always at byte
// offset N*32 — no scanning, no parsing, just arithmetic. A lookup costs
// ceil(log2(entryCount)) seek+reads (14 for 9,629 entries) against an
// already-open file handle, each a small, fixed-offset, non-directory-walk
// access — nothing like the slow filename-based opens session 9 measured.
// RAM cost for the whole index is now a small constant (one 32-byte
// record buffer for the binary search), regardless of vocabulary size —
// removing the RAM ceiling entirely. Vocabulary can grow to 50,000+ words
// with no additional RAM impact, only ~1 extra seek per doubling.
//
// Falls back cleanly: if bank.bin/bank.bidx are missing, malformed, or a
// specific word simply isn't in the index (e.g. added to the card after
// the last repack), resolution and playback continue working exactly as
// before via the existing per-file path. The bank is purely an
// accelerator, never a hard requirement — individual word .mp3 files do
// not need to be deleted from the card.
#define BANK_WORD_MAX_LEN 23
#define BANK_RECORD_SIZE  32  // char word[24] + uint32_t offset(4) + uint32_t length(4), packed

static File _bankFile;    // bank.bin — audio clip bytes (unchanged from before)
static File _bankIdxFile; // bank.bidx — sorted fixed-length binary index (kept open, searched via seek)
static bool _wordBankLoaded = false;
static uint32_t _wordBankEntryCount = 0; // derived from file size / BANK_RECORD_SIZE, not a scan

// Reads binary record #recordIndex directly from the open index file into
// word/offset/length. One seek + one read, both against a known byte
// position — no scanning. Returns false only on a real I/O failure.
static bool readBankRecord(uint32_t recordIndex, char outWord[BANK_WORD_MAX_LEN + 1], uint32_t& outOffset, uint32_t& outLength) {
  uint8_t buf[BANK_RECORD_SIZE];
  if (!_bankIdxFile.seek((uint32_t)recordIndex * BANK_RECORD_SIZE)) return false;
  if (_bankIdxFile.read(buf, BANK_RECORD_SIZE) != BANK_RECORD_SIZE) return false;
  memcpy(outWord, buf, BANK_WORD_MAX_LEN + 1);
  outWord[BANK_WORD_MAX_LEN] = '\0'; // defensive - packer should already null-terminate
  memcpy(&outOffset, buf + (BANK_WORD_MAX_LEN + 1), 4);
  memcpy(&outLength, buf + (BANK_WORD_MAX_LEN + 1) + 4, 4);
  return true;
}

// Opens /words/bank_<lang>.bin + /words/bank_<lang>.bidx and keeps both
// handles open for the device's lifetime. Does NOT read the index into
// RAM — only validates the file sizes are a whole multiple of
// BANK_RECORD_SIZE (a cheap sanity check that the file is the binary
// format expected, not a leftover text .idx from before this rework) and
// derives the entry count from file size alone (one getSize() call, no
// scanning). Safe to call again later (e.g. after copying a freshly
// repacked bank onto the card) — closes and reopens both handles.
// Returns false (and leaves the device on the per-file fallback path) if
// either file is missing or malformed.
bool loadWordBank() {
  _wordBankLoaded = false;
  _wordBankEntryCount = 0;
  if (_bankFile) _bankFile.close();
  if (_bankIdxFile) _bankIdxFile.close();

  if (!sdReady) { logTs("BANK", "Skipped - SD not mounted"); return false; }

  // Bank files live at /words/bank_<lang>.bin + /words/bank_<lang>.bidx —
  // the PARENT of /words/<lang>/, not inside it (keeps generated/derived
  // data separate from the source-of-truth word clips it was packed
  // from). ".bidx" (not ".idx") marks the new fixed-length binary format
  // explicitly, so an old text-format .idx left on a card from before
  // this rework is never accidentally read as binary records.
  String lang = langEnglish ? "en" : "ch";
  String idxPath = "/words/bank_" + lang + ".bidx";
  String binPath = "/words/bank_" + lang + ".bin";

  if (!SD.exists(idxPath.c_str()) || !SD.exists(binPath.c_str())) {
    logTs("BANK", "No bank files present - using per-file lookup");
    return false;
  }

  Serial.print(F("[BANK] Free heap before load: ")); Serial.print(ESP.getFreeHeap()); Serial.println(F(" bytes"));

  File idxF = SD.open(idxPath.c_str(), FILE_READ);
  if (!idxF) { logTs("BANK", "Failed to open bank index"); return false; }

  uint32_t idxSize = idxF.size();
  if (idxSize == 0 || (idxSize % BANK_RECORD_SIZE) != 0) {
    Serial.print(F("[BANK] Index file size (")); Serial.print(idxSize);
    Serial.print(F(" bytes) is not a whole multiple of the ")); Serial.print(BANK_RECORD_SIZE);
    Serial.println(F("-byte record size - malformed or wrong format (expected the new .bidx binary format, not old text .idx). Rejecting."));
    idxF.close();
    return false;
  }
  _wordBankEntryCount = idxSize / BANK_RECORD_SIZE;
  _bankIdxFile = idxF; // keep open for lifetime - every lookup seeks on this handle directly

  _bankFile = SD.open(binPath.c_str(), FILE_READ);
  if (!_bankFile) {
    logTs("BANK", "Failed to open bank.bin");
    _bankIdxFile.close();
    _wordBankEntryCount = 0;
    return false;
  }

  _wordBankLoaded = true;
  Serial.print(F("[BANK] Ready - ")); Serial.print(_wordBankEntryCount);
  Serial.print(F(" entries in ")); Serial.print(idxPath);
  Serial.println(F(" (searched on-card via seek, nothing parsed into RAM)"));
  Serial.print(F("[BANK] Free heap after load: ")); Serial.print(ESP.getFreeHeap()); Serial.println(F(" bytes"));
  return true;
}

// Guards every access to the shared bank.bin / bank.bidx file handles
// and, when the prefetch system further down is active, the shared clip
// buffers too. Recursive because bank-file access can be reached both
// directly (a single blocking load) and from within an already-locked
// outer scope (the prefetch task wraps its whole load in this same
// lock) — a plain mutex would deadlock a task against itself in that
// case. Defined HERE (moved up from the RAM-backed-playback section)
// because bankLookup() below needs it too: unlike the old pure-RAM
// index, the on-SD binary-search lookup does real seek()+read() I/O
// against a handle shared across the foreground task and the background
// prefetch task, so it needs the identical protection already used for
// bank.bin's audio reads.
static SemaphoreHandle_t _prefetchMutex = nullptr;
static void ensurePrefetchMutex() {
  if (!_prefetchMutex) _prefetchMutex = xSemaphoreCreateRecursiveMutex();
}

// Binary search directly against the open index file — zero RAM array,
// a handful of seek()+read() calls (ceil(log2(N)) — 14 for 9,629
// entries) instead of one large in-memory list. Returns true and fills
// in the byte offset/length within bank.bin if `stem` is in the index.
// Mutex-guarded for the same reason bank.bin access is: this now does
// real SD I/O on a handle shared across tasks.
static bool bankLookup(const String& stem, uint32_t& outOffset, uint32_t& outLength) {
  if (!_wordBankLoaded || _wordBankEntryCount == 0) return false;
  if (stem.length() == 0 || (int)stem.length() > BANK_WORD_MAX_LEN) return false;

  ensurePrefetchMutex();
  xSemaphoreTakeRecursive(_prefetchMutex, portMAX_DELAY);

  uint32_t lo = 0, hi = _wordBankEntryCount; // [lo, hi)
  char word[BANK_WORD_MAX_LEN + 1];
  uint32_t offset, length;
  bool found = false;
  while (lo < hi) {
    uint32_t mid = lo + (hi - lo) / 2;
    if (!readBankRecord(mid, word, offset, length)) break; // I/O failure - fail safe, fall back
    int cmp = strcmp(stem.c_str(), word);
    if (cmp == 0) { outOffset = offset; outLength = length; found = true; break; }
    if (cmp < 0) hi = mid; else lo = mid + 1;
  }

  xSemaphoreGiveRecursive(_prefetchMutex);
  return found;
}

bool wordBankIsLoaded() { return _wordBankLoaded; }
size_t wordBankEntryCount() { return (size_t)_wordBankEntryCount; }




// ─── WORD ASSET INDEX (built once at boot) ──────────────────────
// Investigation (session 9, see log_v_2.md) into multi-second gaps between
// words during note playback found the delay is neither file-size-related
// nor a cold-cache thing: the SAME file opened twice, minutes apart, costs
// the IDENTICAL latency both times (e.g. m.mp3 ~4360ms both plays,
// grace.mp3 ~3780ms both plays), while a.mp3 is ~35ms every time. That
// rules out caching and file size, and points at something fixed per file
// — almost certainly where its directory entry / FAT chain sits on the
// card, which the simple Arduino SD library re-walks on every open() by
// name (it doesn't expose a way to cache a direct "go straight there"
// handle the way raw SdFat does).
//
// This index does NOT fix that per-file open cost for files that DO
// exist — that needs either low-level indexed opening (a bigger change,
// tied to the exact SdFat version bundled with the ESP32 core) or
// restructuring how assets are stored (see the "single indexed blob"
// recommendation in log_v_2.md). What this DOES fix:
//   - Every word used to pay a full SD.open() JUST to check existence
//     (resolveAudioAsset -> fileExistsReadOnly), THEN a second SD.open()
//     moments later to actually read it (openAndValidateMp3) — i.e. every
//     word paid the slow, position-dependent open cost TWICE. Trusting
//     the index for existence removes the first one entirely.
//   - A word we already know has no clip (every digit 0-9 — none exist on
//     this card; see the numeral fix in buildChainToFile below) previously
//     still cost a full doomed SD.open() attempt, EVERY time it was
//     needed, forever. With the index that's a zero-cost in-RAM check.
static std::vector<String> _wordAssetIndex;
static bool _wordAssetIndexBuilt = false;

// Scans the current-language word-audio folder ONCE, recording every
// filename (lowercased, no ".mp3") into a sorted in-RAM list. Call at
// boot (after SD mounts) and again whenever the language changes or new
// assets are added to the card (see the "reindex" serial command).
void buildWordAssetIndex() {
  _wordAssetIndex.clear();
  _wordAssetIndexBuilt = false;
  if (!sdReady) { logTs("INDEX", "Skipped - SD not mounted"); return; }

  String folder = audioLangFolder();
  File dir = SD.open(folder.c_str());
  if (!dir || !dir.isDirectory()) {
    logTsValue("INDEX", "FAILED to open ", folder);
    return;
  }

  unsigned long t0 = millis();
  Serial.print(F("[INDEX] Scanning ")); Serial.print(folder);
  Serial.println(F(" ... this can take a while with a large word library, printing progress every 1000 files"));
  uint32_t filesSeen = 0;
  File f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      String name = String(f.name());
      int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      if (name.endsWith(".mp3")) {
        name = name.substring(0, name.length() - 4);
        name.toLowerCase();
        _wordAssetIndex.push_back(name);
      }
      filesSeen++;
      if (filesSeen % 1000 == 0) {
        Serial.print(F("[INDEX] ...still scanning, ")); Serial.print(filesSeen);
        Serial.print(F(" files so far (")); Serial.print(millis() - t0); Serial.println(F(" ms elapsed)"));
      }
    }
    f = dir.openNextFile();
  }
  dir.close();

  std::sort(_wordAssetIndex.begin(), _wordAssetIndex.end());
  _wordAssetIndexBuilt = true;

  Serial.print(F("[INDEX] Built ")); Serial.print(_wordAssetIndex.size());
  Serial.print(F(" word entries from ")); Serial.print(folder);
  Serial.print(F(" in ")); Serial.print(millis() - t0); Serial.println(F(" ms"));
}

// True if `stem` (lowercase, no extension) is known to exist on the card,
// per the boot-time index — a pure in-RAM binary search, zero SD access.
static bool assetKnownToExist(const String& stem) {
  return std::binary_search(_wordAssetIndex.begin(), _wordAssetIndex.end(), stem);
}

// Which control pin caused the most recent playMP3() call to be cut short,
// or -1 if the last call finished naturally / no clip has played yet.
// Set at the top of every playMP3() call (reset to -1) and updated only if
// a debounced interrupt actually fires. Callers (menu handlers) read this
// right after playMP3() returns to decide what "cut to next" means for
// their screen — mp3_player.cpp only reports the fact, not the UI action.
int lastInterruptPin = -1;

static AudioGeneratorMP3  _mp3;
static AudioOutputI2S     _i2sOut;
static AudioFileSource*   _audioSrc = nullptr;
static bool                _i2sConfigured = false;

// Buttons that interrupt playback when pressed mid-clip. Declared once here
// instead of being re-declared inline at every call site.
static const int kInterruptPins[5] = {BTN_BACK, BTN_SELECT, BTN_DOWN, BTN_AISAVE, BTN_DELETE};
static const int kInterruptPinCount = 5;

static String audioLangFolder() {
  // The ONLY word-audio location. All note/title/letter/space clips live
  // here, one flat file per word: /words/<lang>/<word>.mp3. Previously the
  // resolver tried this folder, then /voice/<lang>, then /sfx/<lang>, then
  // /data/<lang> — each on SD AND LittleFS — meaning every single word in
  // a note could cost up to 8 blocking filesystem probes before it played.
  // That's real, audible dead air between words. Since every asset actually
  // lives in this one folder, the other three were pure search overhead
  // with no upside — removed.
  return String(langEnglish ? "/words/en" : "/words/ch");
}

// Note audio is served entirely from the SD card, never LittleFS — matching
// the standalone /words/en voice player. Require sdReady so that, with no
// SD, note audio resolves to nothing and the caller falls back to SAM.
static bool fileExistsReadOnly(const String& path) {
  if (!sdReady) return false;
  File f = SD.open(path.c_str(), FILE_READ);
  if (f) { f.close(); return true; }
  return false;
}

static String wordAssetName(const String& word) {
  String clean;
  clean.reserve(word.length());
  for (unsigned int i = 0; i < word.length(); i++) {
    char c = word[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      clean += (char)tolower(c);
    }
  }
  return clean;
}

// Resolves a word/letter stem to a playable path. Note speech (note
// bodies/titles + spelling) is served from SD only — never LittleFS.
//
// Resolution order:
//   1. Word bank, if loaded — returns a synthetic "bank:<stem>" path.
//      playMP3()/loadMp3ToRam() understand this prefix and do a
//      seek+read on the single already-open bank.bin handle. No
//      per-word filesystem open at all.
//   2. Per-file index (see buildWordAssetIndex()) — a real filesystem
//      path, but skips the redundant existence-probe SD call since the
//      index already confirmed the file exists.
//   3. If neither the bank nor the index are loaded, falls back to a
//      real SD probe (original pre-session-9 behavior), so resolution
//      still works, just slower, if SD wasn't ready at boot.
static String resolveAudioAsset(const String& stem) {
  uint32_t bankOffset, bankLength;
  if (bankLookup(stem, bankOffset, bankLength)) {
    return "bank:" + stem;
  }
  if (_wordAssetIndexBuilt) {
    return assetKnownToExist(stem) ? (audioLangFolder() + "/" + stem + ".mp3") : "";
  }
  String p = audioLangFolder() + "/" + stem + ".mp3";
  return fileExistsReadOnly(p) ? p : "";
}

static bool playResolvedAsset(const String& stem) {
  String path = resolveAudioAsset(stem);
  if (path.length() == 0) return false;
  playMP3(path);
  return true;
}

// Builds a one-character stem ("a", "5", ...) without heap churn from String
// concatenation — single character assignment is cheap, but this keeps the
// call sites uniform and avoids repeating the same 3-line pattern 4 times.
static inline String singleCharStem(char c) {
  String stem;
  stem += (char)tolower(c);
  return stem;
}

static bool canSpellFromAssets(const String& text) {
  if (text.length() == 0) return false;
  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text[i];
    if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
      if (resolveAudioAsset("space").length() == 0) return false;
    } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      if (resolveAudioAsset(singleCharStem(c)).length() == 0) return false;
    }
  }
  return true;
}

// True if the most recent playMP3() clip was cut short specifically by
// BACK. Checked between clips in any multi-clip speak sequence (spelling a
// word letter-by-letter, reading a whole sentence word-by-word) so BACK
// stops the WHOLE sequence immediately instead of just the current clip
// and rolling on to the next one.
static inline bool wasBackInterrupt() {
  return lastInterruptPin == BTN_BACK;
}

// Spells a word out letter by letter. Returns false if BACK interrupted
// partway through, so the caller can stop the whole read-back instead of
// continuing to the next word.
static bool spellWithAssets(const String& word) {
  for (unsigned int i = 0; i < word.length(); i++) {
    char c = word[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      playResolvedAsset(singleCharStem(c));
      if (wasBackInterrupt()) return false;
    }
  }
  return true;
}

static bool speakTextFromAssets(const String& text);

static bool speakTextFromAssets(const String& text) {
  bool anyMissing = false;
  String token;
  for (unsigned int i = 0; i <= text.length(); i++) {
    char c = (i < text.length()) ? text[i] : ' ';
    bool isSpace = (c == ' ' || c == '\n' || c == '\t' || c == '\r');
    if (isSpace) {
      if (token.length() > 0) {
        String stem = wordAssetName(token);
        if (stem.length() == 0 || !playResolvedAsset(stem)) {
          if (!spellWithAssets(token)) return true; // BACK interrupted mid-word — stop, and treat as "handled" (not "assets missing") so no SAM fallback kicks in afterward
        } else if (wasBackInterrupt()) {
          return true; // whole-word clip itself was cut by BACK — stop here too
        }
        token = "";
      }
      if (i < text.length()) {
        if (!playResolvedAsset("space")) anyMissing = true;
        else if (wasBackInterrupt()) return true; // BACK cut the inter-word pause — stop
      }
    } else {
      token += c;
    }
  }
  return !anyMissing;
}

// ─── SINGLE SOURCE OF TRUTH: SD / SAM GATE ────────────────────
// SAM is only ever allowed when there is no SD card AND the LittleFS asset
// set cannot fully spell the requested text. Every speak path funnels
// through this one function so the "never use SAM if SD is present"
// invariant can't drift out of sync between call sites.
static bool samAllowedFor(const String& text) {
  return !sdReady && !canSpellFromAssets(text);
}

// Shared fallback chain used by every "speak this text" entry point.
// samFn is the specific SAM call to use (samSay vs samSayChunked). The two
// take different parameter types (const char* vs String), so this is a
// small std::function wrapper rather than a raw function pointer — lets
// call sites pass either one via a lambda without changing samSay's or
// samSayChunked's own signatures in sam_tts.h.
static void speakWithFallback(const String& text, const std::function<void(const String&)>& samFn, const char* logTag) {
  if (speakTextFromAssets(text)) return;
  if (samAllowedFor(text)) {
    Serial.print(F("[")); Serial.print(logTag);
    Serial.println(F("] LittleFS assets incomplete and no SD - SAM fallback"));
    samFn(text);
  } else {
    Serial.print(F("[")); Serial.print(logTag);
    Serial.println(F("] Asset incomplete - SD mounted, SAM forbidden"));
  }
}

// How long a control pin must read "pressed" continuously, mid-playback,
// before it's treated as a deliberate press rather than a bounce/glitch.
// Short enough that a real press feels instant; long enough that the
// microsecond-scale bounce on a mechanical button doesn't fire early.
static const unsigned long kInterruptDebounceMs = 20;

// Per-pin "when did this pin first start reading pressed" timestamps, used
// only during the debounce window of a single playMP3() call. 0 means "not
// currently being timed" (either not pressed, or already confirmed/reset).
static unsigned long _interruptPressStart[kInterruptPinCount] = {0};

static void resetInterruptDebounce() {
  for (int i = 0; i < kInterruptPinCount; i++) _interruptPressStart[i] = 0;
}

// Returns the pin index that just crossed the debounce threshold, or -1.
// A pin that was already held when playback started (initial[i] == true)
// never counts, so resuming playback right after a long-press doesn't
// immediately re-trigger on the same still-held button.
static int debouncedInterruptPin(const bool initial[kInterruptPinCount]) {
  unsigned long now = millis();
  for (int i = 0; i < kInterruptPinCount; i++) {
    if (initial[i]) continue; // was already held before this clip started — ignore
    bool pressedNow = buttonPressed(kInterruptPins[i]);
    if (!pressedNow) {
      _interruptPressStart[i] = 0; // released (or bounced low) — reset the timer
      continue;
    }
    if (_interruptPressStart[i] == 0) {
      _interruptPressStart[i] = now; // press just started — start timing
      continue;
    }
    if (now - _interruptPressStart[i] >= kInterruptDebounceMs) {
      return i; // held long enough — this is a real press
    }
  }
  return -1;
}

static bool controlButtonPressedAfterStart(const bool initial[kInterruptPinCount]) {
  int idx = debouncedInterruptPin(initial);
  if (idx < 0) return false;
  lastInterruptPin = kInterruptPins[idx];
  return true;
}

static void configureI2SOnce() {
  if (_i2sConfigured) return;
  _i2sOut.SetPinout(AMP_BCLK_PIN, AMP_LRC_PIN, AMP_DIN_PIN);
  _i2sOut.SetGain(0.6);
  _i2sConfigured = true;
}

void mp3Setup() {
  configureI2SOnce();
  logTs("AUDIO", "MP3 I2S configured");
}

// ─── RAM-BACKED PLAYBACK (unconditional — used by both live speech and
// chain playback, not just the prefetch machinery below) ────────────────
// Word clips are small enough (observed 3.6-8.9KB) to hold entirely in
// RAM. AudioFileSourceRAM feeds the MP3 decoder from a RAM buffer instead
// of streaming from a file — needed for both the word bank (a "file" is
// really just a byte range inside one shared open handle) and the chain
// prefetch system further down.
#define WORD_CLIP_MAX_BYTES 15360  // 15KB cap — generous vs. observed 3.6-8.9KB clips; must match MAX_CLIP_BYTES in tools/pack_word_bank.py

class AudioFileSourceRAM : public AudioFileSource {
public:
  AudioFileSourceRAM(const uint8_t* data, uint32_t len) : _data(data), _len(len), _pos(0) {}
  bool isOpen() override { return _data != nullptr; }
  uint32_t read(void* dst, uint32_t len) override {
    if (!_data) return 0;
    uint32_t remain = (_pos < _len) ? (_len - _pos) : 0;
    uint32_t n = (len < remain) ? len : remain;
    if (n > 0) memcpy(dst, _data + _pos, n);
    _pos += n;
    return n;
  }
  bool seek(int32_t pos, int origin) override {
    long target;
    if (origin == SEEK_CUR) target = (long)_pos + pos;
    else if (origin == SEEK_END) target = (long)_len + pos;
    else target = pos; // SEEK_SET
    if (target < 0 || (uint32_t)target > _len) return false;
    _pos = (uint32_t)target;
    return true;
  }
  bool close() override { return true; } // buffer lifetime owned externally
  uint32_t getSize() override { return _len; }
  uint32_t getPos() override { return _pos; }
private:
  const uint8_t* _data;
  uint32_t _len, _pos;
};

// Small dedicated buffer for LIVE (non-chain) bank-backed playback —
// menu prompt fallback spelling, un-chained speech, etc. Separate from
// the chain system's double-buffer (below) so live speech never contends
// with chain prefetching for the same memory.
static uint8_t _liveBankBuf[WORD_CLIP_MAX_BYTES];

// Plays a "bank:<word>" path directly and synchronously (no prefetch —
// live speech has no lookahead structure to prefetch against). Returns
// false if the word isn't in the loaded bank, letting the caller
// (playMP3) fall back to nothing further being possible for that path
// (the caller only ever passes a bank: path here if resolveAudioAsset()
// already confirmed it via the bank, so a false here means something
// changed underneath — e.g. bank was reloaded mid-lookup — treat as a
// simple playback failure, same as any other missing-asset case).
static bool playBankPathDirect(const String& path) {
  String stem = path.substring(5); // strip "bank:"
  uint32_t offset, length;
  if (!bankLookup(stem, offset, length)) return false;
  if (length == 0 || length > WORD_CLIP_MAX_BYTES) return false;

  ensurePrefetchMutex();
  xSemaphoreTakeRecursive(_prefetchMutex, portMAX_DELAY);
  bool ok = _bankFile.seek(offset) && (_bankFile.read(_liveBankBuf, length) == length);
  xSemaphoreGiveRecursive(_prefetchMutex);
  if (!ok) return false;

  if (_audioSrc) { delete _audioSrc; _audioSrc = nullptr; }
  _audioSrc = new AudioFileSourceRAM(_liveBankBuf, length);
  logTs("AUDIO", "MP3 source BANK");

  configureI2SOnce();
  _mp3.begin(_audioSrc, &_i2sOut);

  bool initialButtons[kInterruptPinCount];
  for (int i = 0; i < kInterruptPinCount; i++) initialButtons[i] = buttonPressed(kInterruptPins[i]);
  resetInterruptDebounce();

  while (_mp3.isRunning()) {
    debugLogButtonTransitions("mp3");
    if (controlButtonPressedAfterStart(initialButtons)) {
      logTs("BUTTON", "MP3 interrupted by button");
      _mp3.stop();
      break;
    }
    if (!_mp3.loop()) { _mp3.stop(); break; }
  }
  _i2sOut.stop();
  if (_audioSrc) { delete _audioSrc; _audioSrc = nullptr; }
  return true;
}

// Opens the given path (SD preferred, LittleFS fallback) and validates the
// MP3 header/size. On success, hands back an already-open AudioFileSource
// via outSrc so callers don't have to re-open the same file a second time
// to actually play it. Caller owns the returned pointer.
static bool openAndValidateMp3(const String& path, AudioFileSource** outSrc, bool* outFromSd) {
  *outSrc = nullptr;
  bool fromSd = sdReady && SD.exists(path.c_str());
  bool fromLittleFs = !fromSd && LittleFS.exists(path);
  if (!fromSd && !fromLittleFs) return false;

  AudioFileSource* src = fromSd
    ? (AudioFileSource*)new AudioFileSourceSD(path.c_str())
    : (AudioFileSource*)new AudioFileSourceLittleFS(path.c_str());

  if (!src || !src->isOpen()) {
    delete src;
    return false;
  }

  uint32_t sz = src->getSize();
  uint8_t header[3] = {0};
  src->read(header, 3);
  src->seek(0, SEEK_SET);

  bool sizeOk   = sz > 500;
  bool headerOk = (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0) || // sync word
                  (header[0] == 0x49 && header[1] == 0x44 && header[2] == 0x33); // ID3

  // Per-word Serial logging removed from the hot path — with a note chain
  // playing dozens of single-word clips back to back, printing a full
  // validate line for every one of them (at 115200 baud) becomes real,
  // audible-gap-causing overhead. Failures still print below.

  if (!sizeOk || !headerOk) {
    delete src;
    return false;
  }

  *outSrc = src;
  *outFromSd = fromSd;
  return true;
}

// Public validity check. Kept for callers elsewhere in the project that
// only need a yes/no answer (signature unchanged). Internally this now
// reuses openAndValidateMp3 instead of duplicating the header-check logic,
// and closes the file immediately since this path doesn't play it.
bool isValidMp3(String path) {
  if (path.startsWith("bank:")) {
    // Not a real filesystem path — validity here just means "resolvable",
    // which resolveAudioAsset() already confirmed before producing this
    // path. No caller in this codebase currently passes a bank: path
    // here (isValidMp3 is only called with sfxPath()/caller-supplied
    // paths, never resolveAudioAsset()'s output), but this guard is
    // cheap and keeps the function correct if that ever changes.
    uint32_t offset, length;
    return bankLookup(path.substring(5), offset, length);
  }
  AudioFileSource* src = nullptr;
  bool fromSd = false;
  bool ok = openAndValidateMp3(path, &src, &fromSd);
  if (src) { src->close(); delete src; }
  return ok;
}

// ─── PLAY MP3 ────────────────────────────────────────────────
// path passed by const-ref to avoid a String copy; signature otherwise
// unchanged so external callers (speakPhrase/speakText/other files) don't
// need updates.
void playMP3(const String& path) {
  logTsValue("AUDIO", "MP3 START ", path);
  logTestEvent(3, "audio-start", path);
  unsigned long t0 = millis();
  lastInterruptPin = -1;

  if (_mp3.isRunning()) { _mp3.stop(); delay(15); }

  if (_audioSrc) { delete _audioSrc; _audioSrc = nullptr; }

  // Bank-backed path (see resolveAudioAsset()) — seek+read from the single
  // shared bank.bin handle instead of a real filesystem open. Handled as
  // its own branch rather than folded into openAndValidateMp3() because
  // it isn't a filesystem path at all, and doesn't need the SD-vs-LittleFS
  // fallback logic that function exists for.
  if (path.startsWith("bank:")) {
    if (!playBankPathDirect(path)) {
      logTsValue("AUDIO", "MP3 BANK LOOKUP FAILED ", path);
      return;
    }
    Serial.print('['); Serial.print(millis()); Serial.print(F(" ms] AUDIO MP3 DONE duration="));
    Serial.print(millis() - t0); Serial.println(F("ms (bank)"));
    logTestEvent(3, "audio-complete", String("duration_ms=") + String(millis() - t0));
    return;
  }

  bool fromSd = false;
  AudioFileSource* src = nullptr;
  if (!openAndValidateMp3(path, &src, &fromSd)) {
    logTsValue("AUDIO", "MP3 OPEN/VALIDATE FAILED ", path);
    return;
  }
  _audioSrc = src;
  logTs("AUDIO", fromSd ? "MP3 source SD" : "MP3 source LittleFS");

  configureI2SOnce();
  _mp3.begin(_audioSrc, &_i2sOut);

  bool initialButtons[kInterruptPinCount];
  for (int i = 0; i < kInterruptPinCount; i++) initialButtons[i] = buttonPressed(kInterruptPins[i]);
  resetInterruptDebounce();

  int loops = 0;
  while (_mp3.isRunning()) {
    debugLogButtonTransitions("mp3");
    if (controlButtonPressedAfterStart(initialButtons)) {
      logTs("BUTTON", "MP3 interrupted by button");
      _mp3.stop();
      break;
    }
    if (!_mp3.loop()) { _mp3.stop(); break; }
    loops++;
  }

  _i2sOut.stop();
  delay(10);

  if (_audioSrc) { delete _audioSrc; _audioSrc = nullptr; }

  Serial.print('['); Serial.print(millis()); Serial.print(F(" ms] AUDIO MP3 DONE duration="));
  Serial.print(millis() - t0); Serial.print(F("ms loops=")); Serial.println(loops);
  logTestEvent(3, "audio-complete", String("duration_ms=") + String(millis() - t0));
}

// ─── VOICERSS FETCH ──────────────────────────────────────────
// Online TTS fetch path. Retained only as a stub because other files in
// the project still reference this symbol; the device now runs fully
// offline from SD/LittleFS assets. Do not resurrect without re-adding the
// WiFi bring-up this project intentionally removed.
bool vrssFetch(const String& text, const String& path) {
  (void)text;
  (void)path;
  logTs("VRSS", "Removed in offline SD audio mode");
  return false;
}

// Offline audio router
void speakPhrase(const char* name) {
  Serial.print(F("[PHRASE] >> ")); Serial.println(name);
  String path = sfxPath(name);
  if (isValidMp3(path)) {
    Serial.println(F("[PHRASE] MP3 valid - playing"));
    playMP3(path); return;
  }
  if (playResolvedAsset(name)) return;

  Serial.println(F("[PHRASE] MP3 missing - stringing phrase"));
  String text = String(phraseEN(name));
  speakWithFallback(text, [](const String& t) { samSay(t.c_str()); }, "PHRASE");
}

// Speak dynamic text via cached MP3, then word/space/letter assets, then SAM only without SD.
void speakText(const String& text, const String& mp3Path) {
  if (mp3Path.length() > 0 && isValidMp3(mp3Path)) {
    Serial.println(F("[TEXT] Playing cached MP3"));
    playMP3(mp3Path); return;
  }
  Serial.println(F("[TEXT] Stringing text from assets"));
  speakWithFallback(text, samSayChunked, "TEXT");
}

void speakCharacter(char c) {
  String text;
  text += c;
  if (c == ' ') {
    if (playResolvedAsset("space")) return;
  } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
    if (playResolvedAsset(singleCharStem(c))) return;
  }
  if (samAllowedFor(text)) {
    samSay(text.c_str());
  } else {
    Serial.println(F("[CHAR] Asset missing - SAM forbidden or not needed"));
  }
}

// ─── FIRST BOOT CACHE ────────────────────────────────────────
void cacheMenuAudio() {
  // Handled entirely by Core 1 via CACHE_MENU queue signal
}

// ─── CORE 1 TASK ─────────────────────────────────────────────
void c1log(const char* msg) {
  Serial.print(F("[C1] ")); Serial.println(msg);
}

void audioFetchTask(void* param) {
  (void)param;
  audioFetchIdle = true;
  vTaskDelete(nullptr);
}

// Offline build: queueing never succeeds. Kept named/visible (rather than
// silently no-op) so anything calling it logs why cached audio never shows
// up, instead of looking like a working feature that mysteriously fails.
bool _safeQueue(const String& path, const String& text) {
  (void)text;
  Serial.print(F("[QUEUE] OFFLINE SD mode; not fetching "));
  Serial.println(path);
  return false;
}

// queueNoteAudio() removed — fully superseded by buildNoteAudioChain()/
// playNoteAudioChain() below, which actually work (this old function was
// a dead stub that referenced a note.mp3 file nothing ever produced).
// Titles now use the same chain mechanism (buildNoteTitleChain()/
// playNoteTitleChain()) instead of a cached note{N}_title.mp3, so the old
// queueNoteTitleAudio() stub is gone too.

// ─── NOTE AUDIO CHAINS ─────────────────────────────────────────
// On-disk format: one unit per line.
//   W:<stem>   whole-word asset (lowercased, punctuation-stripped, via
//              the same wordAssetName() used by live speech)
//   L:<letter> single-character asset (spelling fallback for a word with
//              no whole-word clip)
//   SP         the space/pause clip between words
// Built once at save time by resolving against whatever's on the SD card
// right then; replayed later without re-resolving. If the asset library
// changes afterward, older notes' chains won't pick that up until resaved
// — a known tradeoff of pre-building rather than resolving live.

static bool writeChainLine(File& f, const String& line) {
  size_t written = f.println(line);
  return written > 0;
}

// Writes a chain (same W:/L:/SP format) for `text` to `path`. Shared by the
// body and title chain builders so the two can never drift apart. Resolves
// each word against the SD asset library once, via the same wordAssetName()
// lowercasing/punctuation-stripping used by live speech (so "Poverty" and
// "poverty" resolve identically), spelling letter-by-letter when no whole
// word clip exists.
static bool buildChainToFile(const String& path, const String& text) {
  // Overwrite any previous chain at this path outright — FILE_WRITE
  // truncates on most Arduino-style filesystem libraries, but we don't
  // rely on that silently: an old chain here would otherwise mix stale
  // and fresh lines if this note is ever rebuilt shorter than before.
  bool onSd = sdReady;
  File f = onSd ? SD.open(path.c_str(), FILE_WRITE) : LittleFS.open(path, FILE_WRITE);
  if (!f) {
    logTsValue("CHAIN", "build FAILED to open ", path);
    return false;
  }

  // Investigation (session 9) found notes containing digits (e.g. "18")
  // were producing chain lines like L:1, L:8 UNCONDITIONALLY — with no
  // check that a "1.mp3"/"8.mp3" clip actually exists. It doesn't; this
  // card only has letter clips (a-z), no digit clips (0-9). The result:
  // every note containing a number paid a doomed SD lookup for that digit
  // on EVERY future playback, forever, producing silence for that
  // character. Fixed here, upstream, at build time: only write an L: line
  // if the index confirms that character's clip actually exists. Missing
  // characters are skipped (silence for just that character, same
  // "asset missing" convention used elsewhere) and reported ONCE here,
  // not rediscovered via a failed SD open on every future read of the note.
  int missingCount = 0;
  String missingChars = "";

  String token;
  bool ok = true;
  for (unsigned int i = 0; i <= text.length() && ok; i++) {
    char c = (i < text.length()) ? text[i] : ' ';
    bool isSpace = (c == ' ' || c == '\n' || c == '\t' || c == '\r');
    bool sentenceEnd = false;
    if (isSpace) {
      if (token.length() > 0) {
        // Strip trailing sentence punctuation before resolving the word
        // asset (wordAssetName already strips punctuation for lookup, but
        // we need to know here whether it WAS there, to choose SP vs SPS).
        char lastCh = token[token.length() - 1];
        if (lastCh == '.' || lastCh == '!' || lastCh == '?') sentenceEnd = true;

        // Poverty / poverty must resolve identically — wordAssetName()
        // lowercases and strips punctuation before we check what's on
        // the SD card, matching how live playback already resolves words.
        String stem = wordAssetName(token);
        if (stem.length() > 0 && resolveAudioAsset(stem).length() > 0) {
          ok = writeChainLine(f, "W:" + stem);
        } else {
          // No whole-word clip — spell it letter by letter instead, but
          // only for characters that actually have a clip (see comment
          // above). A character with no clip is skipped, not written as
          // a doomed L: line.
          for (unsigned int j = 0; j < token.length() && ok; j++) {
            char tc = token[j];
            if ((tc>='A'&&tc<='Z') || (tc>='a'&&tc<='z') || (tc>='0'&&tc<='9')) {
              String charStem = singleCharStem(tc);
              if (resolveAudioAsset(charStem).length() > 0) {
                ok = writeChainLine(f, "L:" + charStem);
              } else {
                missingCount++;
                if (missingChars.indexOf(charStem) < 0) { missingChars += charStem; missingChars += " "; }
              }
            }
          }
        }
        token = "";
      }
      if (i < text.length() && ok) ok = writeChainLine(f, sentenceEnd ? "SPS" : "SP");
    } else {
      token += c;
    }
  }
  f.close();

  if (!ok) {
    logTsValue("CHAIN", "build INCOMPLETE for ", path);
    return false;
  }
  logTsValue("CHAIN", "built ", path);
  if (missingCount > 0) {
    Serial.print(F("[CHAIN] ")); Serial.print(path);
    Serial.print(F(" — ")); Serial.print(missingCount);
    Serial.print(F(" character(s) have no audio clip and will be silent: "));
    Serial.println(missingChars);
  }
  return true;
}

bool buildNoteAudioChain(int index, const String& body) {
  return buildChainToFile(noteChainPath(index), body);
}

bool buildNoteTitleChain(int index, const String& title) {
  return buildChainToFile(noteTitleChainPath(index), title);
}

// Checks whether a chain file at `path` exists AND contains at least one
// real W:/L: word/letter line — not just SP/SPS silence or nothing at all.
// A chain file can exist but be empty (0 bytes) or contain only pause
// markers if something went wrong during a previous build (e.g. power loss
// mid-write, or a save that produced no resolvable words). Treating mere
// file-presence as "chain is ready" let those broken files sit there
// forever silently doing nothing, since ensureNoteAudioChains() only
// rebuilds chains that are "missing" — an empty file isn't missing, it's
// just useless. This makes "ready" mean "has real content."
static bool chainFileHasContent(const String& path) {
  bool onSd = sdReady && SD.exists(path.c_str());
  if (!onSd && !LittleFS.exists(path)) return false;
  File f = onSd ? SD.open(path.c_str(), FILE_READ) : LittleFS.open(path, FILE_READ);
  if (!f) return false;
  bool hasWordLine = false;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.startsWith("W:") || line.startsWith("L:")) { hasWordLine = true; break; }
  }
  f.close();
  return hasWordLine;
}

bool noteAudioChainExists(int index) {
  return chainFileHasContent(noteChainPath(index));
}

bool noteTitleChainExists(int index) {
  return chainFileHasContent(noteTitleChainPath(index));
}

// Word-boundary gap between chain units. This is DELIBERATE SILENCE, not
// an audio clip — there is no space.mp3 on the card, so searching for one
// on every single word boundary was pure wasted SD/lookup time for zero
// benefit (it always missed, then fell back to this same delay anyway).
// SP/SPS now skip the lookup entirely and just wait — a naturally spaced
// silence instead of paying for a failed search AND a fixed delay on top.
static const unsigned long kSpacePauseMs = 120;    // gap between words, same sentence
static const unsigned long kSentencePauseMs = 260; // gap after . ! ? — audible sentence break

// ─── LOOKAHEAD PREFETCH FOR CHAIN WORD PLAYBACK ─────────────────
// Session 9 investigation (log_v_2.md) established two things:
//  1. The SAME file opened twice, minutes apart, costs the IDENTICAL
//     open latency both times (m.mp3 ~4360ms both plays, grace.mp3
//     ~3780ms both plays, a.mp3 ~35ms every time). There is no caching
//     layer to exploit by merely "touching" a file early — the only way
//     to hide the latency is to actually pull the file's bytes into RAM
//     before we need them.
//  2. Actual decode+I2S playback time is consistently ~1.2-1.5s per word
//     clip regardless of which word — it's the open/locate step that
//     varies from ~35ms to 6000+ms. That ~1.2-1.5s of playback is exactly
//     the window a background task can use to fetch the NEXT word.
//
// Design: word clips are tiny (observed 3.6-8.9KB) — small enough to
// hold entirely in RAM. So instead of streaming each clip live from SD
// during playback (which keeps the SD/SPI bus busy for the whole
// playback window), we read each clip FULLY into a RAM buffer, then
// play it from RAM. That frees the SD bus during playback — which is
// exactly when a background FreeRTOS task (pinned to the other core)
// can safely prefetch the NEXT word's bytes with no contention.
//
// Feature-flagged: if this doesn't compile/behave correctly against your
// exact ESP8266Audio version (the AudioFileSourceRAM class below mirrors
// the virtual interface already used elsewhere in this file — see
// openAndValidateMp3's calls to read()/seek()/getSize()/isOpen()/close()
// — but hasn't been verified against your installed library source),
// set this to 0 to fall back instantly to the old, proven
// streaming-from-SD path with no other code changes needed.
#define ENABLE_WORD_CLIP_PREFETCH 1

#if ENABLE_WORD_CLIP_PREFETCH

struct WordClipBuffer {
  uint8_t  data[WORD_CLIP_MAX_BYTES];
  uint32_t len = 0;
  String   path;          // which file this buffer holds ("" = none)
  bool     valid = false; // true once the load completed successfully
};

static WordClipBuffer  _clipBufA;
static WordClipBuffer  _clipBufB;
static WordClipBuffer* _playBuf     = &_clipBufA; // buffer currently playing / about to play
static WordClipBuffer* _prefetchBuf = &_clipBufB; // buffer being filled in background

static TaskHandle_t     _prefetchTaskHandle = nullptr;
static volatile bool  _prefetchRequested = false;
static volatile bool  _prefetchInFlightIsValid = false;
static String         _prefetchRequestPath;

// Reads one small MP3 file fully into a fixed RAM buffer. This is the
// ONE place actual SD access happens for a chain word clip now.
// Understands two kinds of `path`:
//   "bank:<word>"  — look up the word in the loaded bank index, seek+read
//                    from the single already-open bank.bin handle. No
//                    per-word open() at all.
//   a real filesystem path — legacy per-file behavior, unchanged.
static bool loadMp3ToRam(const String& path, WordClipBuffer* buf) {
  buf->valid = false;
  buf->len = 0;
  if (path.length() == 0) return false;

  if (path.startsWith("bank:")) {
    if (!_wordBankLoaded) return false;
    String stem = path.substring(5);
    uint32_t offset, length;
    if (!bankLookup(stem, offset, length)) return false;
    if (length == 0 || length > WORD_CLIP_MAX_BYTES) return false;
    // _bankFile is one handle shared between this call (which may run on
    // either the foreground task or the background prefetch task) and
    // every other caller — always mutex-guard the actual seek+read.
    ensurePrefetchMutex();
    xSemaphoreTakeRecursive(_prefetchMutex, portMAX_DELAY);
    bool ok = _bankFile.seek(offset) && (_bankFile.read(buf->data, length) == length);
    xSemaphoreGiveRecursive(_prefetchMutex);
    if (!ok) return false;
    buf->len = length;
    buf->path = path;
    buf->valid = true;
    return true;
  }

  if (!sdReady) return false;
  File f = SD.open(path.c_str(), FILE_READ);
  if (!f) return false;
  uint32_t sz = f.size();
  if (sz == 0 || sz > WORD_CLIP_MAX_BYTES) {
    // Too big for the buffer (or empty) — caller falls back to the
    // original streaming playMP3() path for just this one clip.
    f.close();
    return false;
  }
  uint32_t got = f.read(buf->data, sz);
  f.close();
  if (got != sz) return false;
  buf->len = sz;
  buf->path = path;
  buf->valid = true;
  return true;
}

// Background task: waits for a prefetch request, loads that file into the
// prefetch buffer, signals done, repeats. Pinned to core 0 so it never
// competes with the main loop()/button-polling/I2S path on core 1. The
// mutex guards _prefetchBuf's contents/metadata against the foreground
// task reading or swapping it mid-write — a bare volatile flag isn't a
// real memory barrier across cores, so this closes that gap properly
// rather than assuming timing alone keeps it safe.
static void prefetchTaskFn(void* param) {
  (void)param;
  ensurePrefetchMutex();
  for (;;) {
    if (_prefetchRequested) {
      String p = _prefetchRequestPath;
      xSemaphoreTakeRecursive(_prefetchMutex, portMAX_DELAY);
      bool ok = loadMp3ToRam(p, _prefetchBuf);
      _prefetchInFlightIsValid = ok;
      xSemaphoreGiveRecursive(_prefetchMutex);
      _prefetchRequested = false;
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

static void ensurePrefetchTaskStarted() {
  if (_prefetchTaskHandle) return;
  ensurePrefetchMutex();
  xTaskCreatePinnedToCore(prefetchTaskFn, "audioPrefetch", 4096, nullptr, 1, &_prefetchTaskHandle, 0);
}

// Kicks off a background load of `path` into the prefetch buffer. Safe to
// call repeatedly — loadMp3ToRam only ever runs on the background task,
// never concurrently with itself, so there's no real racing SD access.
static void requestPrefetch(const String& path) {
  if (path.length() == 0) return;
  ensurePrefetchTaskStarted();
  bool alreadyHave = false;
  xSemaphoreTakeRecursive(_prefetchMutex, portMAX_DELAY);
  alreadyHave = (_prefetchBuf->valid && _prefetchBuf->path == path);
  xSemaphoreGiveRecursive(_prefetchMutex);
  if (alreadyHave) return;
  _prefetchRequestPath = path;
  _prefetchRequested = true;
}

// Plays a word/letter clip from RAM, using the prefetch buffer if it's
// ready (or finishes waiting for it if it's still in flight for exactly
// this file), otherwise blocking on a direct load — same cost as before,
// but now only for words that had no chance to be prefetched (typically
// just the first word of a chain).
static bool playChainWordFromRam(const String& path) {
  ensurePrefetchMutex();
  WordClipBuffer* useBuf = nullptr;

  xSemaphoreTakeRecursive(_prefetchMutex, portMAX_DELAY);
  bool prefetchReady = (_prefetchBuf->valid && _prefetchBuf->path == path);
  bool prefetchInFlightForThis = (!prefetchReady && _prefetchRequested && _prefetchRequestPath == path);
  xSemaphoreGiveRecursive(_prefetchMutex);

  if (prefetchReady) {
    useBuf = _prefetchBuf;
  } else if (prefetchInFlightForThis) {
    // Prefetch is in flight for exactly this file — wait for it. This is
    // the only case that can still block, and only for whatever time
    // remains beyond the previous unit's playback window.
    unsigned long waitStart = millis();
    while (_prefetchRequested && millis() - waitStart < 8000) vTaskDelay(pdMS_TO_TICKS(2));
    xSemaphoreTakeRecursive(_prefetchMutex, portMAX_DELAY);
    if (_prefetchInFlightIsValid && _prefetchBuf->path == path) useBuf = _prefetchBuf;
    xSemaphoreGiveRecursive(_prefetchMutex);
  }

  if (!useBuf) {
    // No usable prefetch (first word of a chain, prefetch missed the
    // window, or file too big for the buffer) — load directly now.
    if (!loadMp3ToRam(path, _playBuf)) return false;
    useBuf = _playBuf;
  } else if (useBuf == _prefetchBuf) {
    xSemaphoreTakeRecursive(_prefetchMutex, portMAX_DELAY);
    WordClipBuffer* tmp = _playBuf;
    _playBuf = _prefetchBuf;
    _prefetchBuf = tmp;
    _prefetchBuf->valid = false; // now idle, ready for the next prefetch request
    useBuf = _playBuf;
    xSemaphoreGiveRecursive(_prefetchMutex);
  }

  logTsValue("AUDIO", "MP3 START (RAM) ", useBuf->path);
  unsigned long t0 = millis();
  lastInterruptPin = -1;
  if (_mp3.isRunning()) { _mp3.stop(); delay(15); }
  if (_audioSrc) { delete _audioSrc; _audioSrc = nullptr; }

  _audioSrc = new AudioFileSourceRAM(useBuf->data, useBuf->len);
  configureI2SOnce();
  _mp3.begin(_audioSrc, &_i2sOut);

  bool initialButtons[kInterruptPinCount];
  for (int i = 0; i < kInterruptPinCount; i++) initialButtons[i] = buttonPressed(kInterruptPins[i]);
  resetInterruptDebounce();

  while (_mp3.isRunning()) {
    debugLogButtonTransitions("mp3");
    if (controlButtonPressedAfterStart(initialButtons)) {
      logTs("BUTTON", "MP3 interrupted by button");
      _mp3.stop();
      break;
    }
    if (!_mp3.loop()) { _mp3.stop(); break; }
  }
  _i2sOut.stop();
  if (_audioSrc) { delete _audioSrc; _audioSrc = nullptr; }

  Serial.print('['); Serial.print(millis()); Serial.print(F(" ms] AUDIO MP3 DONE(RAM) duration="));
  Serial.println(millis() - t0);
  return true;
}
#endif // ENABLE_WORD_CLIP_PREFETCH

// Plays one resolved chain line. Returns true if this unit produced AUDIBLE
// output (a word/letter clip actually played) — NOT true merely for a
// pause. This distinction matters: playChainAtPath uses the count of
// audio-producing units to decide whether the chain "worked" at all. A
// chain whose every W:/L: word lookup fails must report failure even
// though it still contains valid SP/SPS silence markers.
static bool playChainUnit(const String& line, bool& producedAudio) {
  producedAudio = false;
  if (line == "SP")  { delay(kSpacePauseMs);    return true; }
  if (line == "SPS") { delay(kSentencePauseMs); return true; }
  if (line.startsWith("W:") || line.startsWith("L:")) {
    String path = resolveAudioAsset(line.substring(2));
    if (path.length() == 0) return false;
#if ENABLE_WORD_CLIP_PREFETCH
    bool ok = playChainWordFromRam(path);
#else
    bool ok = playResolvedAsset(line.substring(2));
#endif
    producedAudio = ok;
    return ok;
  }
  return false; // unrecognized line — corrupt/foreign file, skip safely
}

// Reads an entire chain file into memory as individual lines. Chains are
// tiny (a few dozen to ~100 short lines), so holding the whole thing in
// RAM is trivial and makes lookahead (peeking at upcoming lines to know
// what to prefetch) simple instead of juggling a streaming file handle.
static std::vector<String> readChainLines(const String& path) {
  std::vector<String> lines;
  bool onSd = sdReady && SD.exists(path.c_str());
  File f = onSd ? SD.open(path.c_str(), FILE_READ) : LittleFS.open(path, FILE_READ);
  if (!f) return lines;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) lines.push_back(line);
  }
  f.close();
  return lines;
}

// Plays a chain file at `path` unit-by-unit, stopping the whole chain (not
// just the current unit) if a control button interrupts. Shared by the body
// and title chain players.
//
// lastInterruptPin is reset here, not just inside playMP3(). Reason: SP/SPS
// units can resolve to a plain delay() instead of a playMP3() call (no
// space.mp3 asset present), so if the note has any leading space/pause
// units, or if the FIRST unit happens to be an SP, playMP3() may not run
// until partway through the chain — meanwhile wasBackInterrupt() is
// checked after every unit. Without this reset, a stale BACK interrupt
// left over from an unrelated earlier action (e.g. BACK used to navigate
// menus) would cause the very first wasBackInterrupt() check to trip
// immediately, aborting the whole chain before anything audible plays —
// which is exactly what silent note playback looks like.
//
// Returns false (not true) if the file existed but produced zero AUDIBLE
// units — e.g. an empty/stale chain, or one whose every W:/L: word lookup
// fails (asset moved/renamed since the chain was built). Pause-only lines
// (SP/SPS) do NOT count toward success — a chain that's nothing but
// failed-lookup words plus pauses must report failure, not silently
// "succeed" by playing nothing but gaps. Callers use this to fall back
// rather than reporting silent success. This is also what lets a broken
// chain self-heal: see speakNoteTitle()/speakNoteBody() in buttons.h, which
// rebuild-and-retry once when this returns false.
//
// Also drives the lookahead prefetch (session 9): before playing unit i,
// it scans forward past any SP/SPS to find the next real W:/L: unit and
// requests that file be loaded in the background WHILE unit i plays —
// hiding that file's open latency behind unit i's ~1.2-1.5s playback
// window. See ENABLE_WORD_CLIP_PREFETCH above for the full rationale.
static bool playChainAtPath(const String& path) {
  std::vector<String> lines = readChainLines(path);
  if (lines.empty()) return false;

  lastInterruptPin = -1;
  int audioUnitsPlayed = 0;

#if ENABLE_WORD_CLIP_PREFETCH
  // Give the very first word a head start too, if there's any time before
  // we reach it (there usually isn't much, but it's free to try).
  for (size_t i = 0; i < lines.size(); i++) {
    String p0 = resolveAudioAsset(
      (lines[i].startsWith("W:") || lines[i].startsWith("L:")) ? lines[i].substring(2) : "");
    if (p0.length() > 0) { requestPrefetch(p0); break; }
  }
#endif

  for (size_t i = 0; i < lines.size(); i++) {
    const String& line = lines[i];

#if ENABLE_WORD_CLIP_PREFETCH
    // Look past this unit for the next real word/letter and start
    // fetching it now, before we block on playing the current unit.
    for (size_t j = i + 1; j < lines.size(); j++) {
      if (lines[j].startsWith("W:") || lines[j].startsWith("L:")) {
        String nextPath = resolveAudioAsset(lines[j].substring(2));
        if (nextPath.length() > 0) requestPrefetch(nextPath);
        break;
      }
      if (lines[j] != "SP" && lines[j] != "SPS") break; // unrecognized line - stop scanning ahead
    }
#endif

    bool producedAudio = false;
    playChainUnit(line, producedAudio);
    if (producedAudio) audioUnitsPlayed++;
    if (wasBackInterrupt()) break; // stop the whole chain, not just this unit
  }
  return audioUnitsPlayed > 0;
}

bool playNoteAudioChain(int index) {
  return playChainAtPath(noteChainPath(index));
}

bool playNoteTitleChain(int index) {
  return playChainAtPath(noteTitleChainPath(index));
}