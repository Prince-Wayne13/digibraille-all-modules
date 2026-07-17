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
#include "AudioFileSource.h"
#include "AudioFileSourceLittleFS.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

extern volatile bool audioFetchIdle;

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

// Resolves a word/letter stem to an MP3 path on the SD card. Note speech
// (note bodies/titles + spelling) is served from SD only — never LittleFS.
// One directory, one probe per word — see audioLangFolder() above for why
// the old 4-folder x 2-filesystem search was removed.
static String resolveAudioAsset(const String& stem) {
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
          // No whole-word clip — spell it letter by letter instead.
          for (unsigned int j = 0; j < token.length() && ok; j++) {
            char tc = token[j];
            if ((tc>='A'&&tc<='Z') || (tc>='a'&&tc<='z') || (tc>='0'&&tc<='9')) {
              ok = writeChainLine(f, "L:" + singleCharStem(tc));
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
    bool ok = playResolvedAsset(line.substring(2));
    producedAudio = ok;
    return ok;
  }
  return false; // unrecognized line — corrupt/foreign file, skip safely
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
static bool playChainAtPath(const String& path) {
  bool onSd = sdReady && SD.exists(path.c_str());
  File f = onSd ? SD.open(path.c_str(), FILE_READ) : LittleFS.open(path, FILE_READ);
  if (!f) return false;

  lastInterruptPin = -1;
  int audioUnitsPlayed = 0;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    bool producedAudio = false;
    playChainUnit(line, producedAudio);
    if (producedAudio) audioUnitsPlayed++;
    if (wasBackInterrupt()) break; // stop the whole chain, not just this unit
  }
  f.close();
  return audioUnitsPlayed > 0;
}

bool playNoteAudioChain(int index) {
  return playChainAtPath(noteChainPath(index));
}

bool playNoteTitleChain(int index) {
  return playChainAtPath(noteTitleChainPath(index));
}