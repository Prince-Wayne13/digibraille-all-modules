#pragma once
#include <Arduino.h>
#include <LittleFS.h>

#define OLED_SDA_PIN      21
#define OLED_SCL_PIN      22

#define AMP_DIN_PIN       25
#define AMP_BCLK_PIN      16
#define AMP_LRC_PIN       27

#define SD_CS_PIN          5
#define SD_SCK_PIN        14
#define SD_MISO_PIN        4
#define SD_MOSI_PIN       17

#define i2c_Address    0x3C
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1

// ─── BUTTONS ─────────────────────────────────────────────────
#define BTN_BACK          13
#define BTN_SELECT        34
#define BTN_DOWN          35
#define BTN_AISAVE        36
#define BTN_DELETE        39
#define BTN_REREAD        -1
#define DEBOUNCE_MS  15

// ─── BRAILLE DOTS ────────────────────────────────────────────
static const int DOT_PINS[6] = {32, 33, 18, 19, 23, 26};
#define DOT_DEBOUNCE_MS 15

#define DEBUG_AUDIO_BUTTONS 1
#define OFFLINE_SD_AUDIO 1

inline bool isValidPin(int pin) {
  return pin >= 0;
}

inline bool buttonPressed(int pin) {
  return isValidPin(pin) && digitalRead(pin) == LOW;
}

inline void logTs(const char* tag, const char* message) {
  Serial.print('[');
  Serial.print(millis());
  Serial.print(F(" ms] "));
  Serial.print(tag);
  Serial.print(F(" "));
  Serial.println(message);
}

inline void logTsValue(const char* tag, const char* message, const String& value) {
  Serial.print('[');
  Serial.print(millis());
  Serial.print(F(" ms] "));
  Serial.print(tag);
  Serial.print(F(" "));
  Serial.print(message);
  Serial.println(value);
}

inline void logTestEvent(int section, const char* event, const String& detail = "") {
  Serial.print('[');
  Serial.print(millis());
  Serial.print(F(" ms] TEST "));
  Serial.print(section);
  Serial.print(F(" "));
  Serial.print(event);
  if (detail.length() > 0) {
    Serial.print(F(" "));
    Serial.print(detail);
  }
  Serial.println();
}

inline const char* buttonNameForPin(int pin) {
  if (pin == BTN_BACK) return "BACK";
  if (pin == BTN_SELECT) return "SELECT";
  if (pin == BTN_DOWN) return "DOWN";
  if (pin == BTN_REREAD) return "REREAD";
  if (pin == BTN_AISAVE) return "AI_SAVE";
  if (pin == BTN_DELETE) return "DELETE";
  return "UNKNOWN";
}

inline void debugLogButtonTransitions(const char* context) {
#if DEBUG_AUDIO_BUTTONS
  const int pins[] = {BTN_BACK, BTN_SELECT, BTN_DOWN, BTN_REREAD, BTN_AISAVE, BTN_DELETE};
  static bool prev[6] = {false, false, false, false, false, false};
  for (int i = 0; i < 6; i++) {
    bool now = buttonPressed(pins[i]);
    if (now != prev[i]) {
      prev[i] = now;
      Serial.print('[');
      Serial.print(millis());
      Serial.print(F(" ms] BUTTON "));
      Serial.print(buttonNameForPin(pins[i]));
      Serial.print(now ? F(" DOWN ") : F(" UP "));
      Serial.println(context);
      logTestEvent(2, now ? "press-detected" : "release-detected", String(buttonNameForPin(pins[i])));
    }
  }
#endif
}

// ─── STORAGE PATHS ───────────────────────────────────────────
#define MAX_NOTES      10
#define MAX_HISTORY   512
#define NOTES_FOLDER  "/notes"
#define SFX_EN_FOLDER "/sfx/en"
#define SFX_CH_FOLDER "/sfx/ch"
#define DRAFT_PATH    "/draft.txt"
#define CONFIG_PATH   "/config.txt"

// ─── TIMING ──────────────────────────────────────────────────
#define BACK_LONG_PRESS_MS   650   // hold BACK this long -> exit pad, save draft
#define LONG_PRESS_WARN_MS   BACK_LONG_PRESS_MS

// BACK double-tap window for "clear word buffer" in the Braille pad.
// A second tap arriving sooner than UNDO_DBLTAP_MIN_MS is treated as bounce
// / an accidental re-trigger on release, not a deliberate second press, and
// falls back to a plain single-tap delete-char.
// A second tap arriving later than UNDO_DBLTAP_MAX_MS is "too late" — the
// first tap has already resolved as a single-tap delete, and this tap
// starts its own fresh single-tap timer instead.
// Only taps landing strictly between the two count as a deliberate double-tap.
#define UNDO_DBLTAP_MIN_MS   150
#define UNDO_DBLTAP_MAX_MS   600
#define AUTOSAVE_MS         5000

// ─── LANGUAGE ────────────────────────────────────────────────
extern bool langEnglish;
extern bool languageConfigured;
extern int  langChoiceIndex;

// ─── STATES ──────────────────────────────────────────────────
enum State {
  STATE_STARTUP,
  STATE_LANG_SELECT,
  STATE_MAIN_MENU,
  STATE_WRITE_TITLE,
  STATE_NEW_NOTE,
  STATE_READ_NOTES,
  STATE_VIEW_NOTE,
  STATE_AI_PREVIEW,
  STATE_DELETE_CONFIRM
};
extern State currentState;

// ─── MENU ────────────────────────────────────────────────────
#define MENU_COUNT 3
extern const char* menuEN[];
extern const char* menuCH[];
extern int menuIndex;

// ─── NOTE DATA ───────────────────────────────────────────────
extern String noteList[MAX_NOTES];
extern int    noteCount;
extern int    selectedNote;
extern String aiImprovedNote;
extern int    noteScrollOffset;

// ─── BRAILLE STATE ───────────────────────────────────────────
// Special 6-dot cell codes. Defined here (not in braille.h) because both
// braille.h and display.h need them, and neither header includes the
// other (see the forward-declaration notes in each — a two-way #include
// between them caused a circular-dependency build failure).
#define BRAILLE_CAPITAL_IND  0b100000
#define BRAILLE_NUMBER_IND   0b111100

extern bool          dots[6];
extern bool          cellBuilding;
extern bool          dotPrev[6];
extern unsigned long dotLastMs[6];
extern bool          capitalNext;
extern bool          numberMode;
struct Unit { char value; };
extern Unit          history[MAX_HISTORY];
extern int           histLen;
extern String        pendingTitle;

// ─── BUTTON STATE ────────────────────────────────────────────
extern unsigned long lastDebounce;
extern unsigned long startupMillis;
extern bool          undoPrev;
extern bool          undoPending;
extern unsigned long undoPendingTime;
extern bool          backLongFired;
extern unsigned long backHeldSince;
extern bool          backWarnSpoken;
extern bool          selectPrev;
extern bool          aiBusy;
extern unsigned long lastAutosave;

// ─── CORE 1 QUEUE ────────────────────────────────────────────
struct AudioFetchJob { char path[64]; char text[512]; };
extern QueueHandle_t audioFetchQueue;
extern volatile bool audioFetchIdle;
extern bool sdReady;
extern bool sdWarningPending;

// ─── PHRASE TABLE ────────────────────────────────────────────
struct Phrase { const char* name; const char* en; const char* ch; };
extern const Phrase PHRASES[];
extern const int    PHRASE_COUNT;

// ─── INLINE HELPERS (no audio library deps) ──────────────────
inline String getNoteTitle(int i) {
  const String& n = noteList[i]; int nl = n.indexOf('\n');
  return (nl < 0) ? n : n.substring(0, nl);
}
inline String getNoteBody(int i) {
  const String& n = noteList[i]; int nl = n.indexOf('\n');
  return (nl < 0) ? "" : n.substring(nl + 1);
}
inline String noteTxtPath(int i) { return String(NOTES_FOLDER) + "/note" + String(i) + ".txt"; }
// Note body audio: NOT a single mp3 file. A "chain" is a small text plan,
// one line per playback unit, built once at save time and replayed on
// every open — see buildNoteAudioChain()/playNoteAudioChain() in
// mp3_player.cpp for the format and why. Replaces the old single-mp3-file
// idea (noteMp3Path), which no code ever actually produced.
inline String noteChainPath(int i) { return String(NOTES_FOLDER) + "/note" + String(i) + "_chain.txt"; }
// Note title audio: same pre-built chain as the body (W:/L:/SP units), so a
// title like "Grace" resolves against the recorded asset library (a whole
// "grace.mp3" if present, else letter-by-letter) instead of relying on a
// never-generated note{N}_title.mp3 file. Built once at save time / migrated
// at boot alongside the body chain.
inline String noteTitleChainPath(int i) { return String(NOTES_FOLDER) + "/note" + String(i) + "_title_chain.txt"; }
inline String sfxPath(const char* name) {
  return String(langEnglish ? SFX_EN_FOLDER : SFX_CH_FOLDER) + "/" + name + ".mp3";
}
inline const char* menuLabel(int i) { return langEnglish ? menuEN[i] : menuCH[i]; }
inline const char* phraseEN(const char* name) {
  for (int i = 0; i < PHRASE_COUNT; i++)
    if (strcmp(PHRASES[i].name, name) == 0) return PHRASES[i].en;
  return name;
}

void enterMainMenu(bool speakHighlighted = true);