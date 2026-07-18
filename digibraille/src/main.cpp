// ============================================================
//  DIGIBRAILLE v2 — WAYNE INC.
//
//  WIRING:
//  BRAILLE DOTS 1..6 -> GPIO 32, 33, 18, 19, 23, 26
//  BACK              -> GPIO 13 (internal pull-up)
//  SELECT/DOWN/AI/DEL-> GPIO 34, 35, 36, 39 (external pull-ups)
//  OLED SDA/SCL      -> GPIO 21, 22
//  MAX98357A DIN/BCLK/LRC -> GPIO 25, 16, 27
//  SD CS/SCK/MISO/MOSI    -> GPIO 5, 14, 4, 17
//
//  FILE STRUCTURE — why two .cpp files:
//  sam_tts.cpp   → AudioTools only   (SAM audio)
//  mp3_player.cpp → ESP8266Audio only (MP3 playback)
//  Both define a class called AudioOutput — keeping them in
//  separate .cpp translation units prevents the name clash.
// ============================================================

#include <functional>
#include <SPI.h>
#include <SD.h>
#include "globals.h"
#include "esp_log.h"
#include "sam_tts.h"
#include "mp3_player.h"
#include "offline_grammar.h"
#include "storage.h"
#include "display.h"
#include "braille.h"
#include "buttons.h"

// ─── OLED INSTANCE ───────────────────────────────────────────
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ─── ALL GLOBAL DEFINITIONS ──────────────────────────────────
bool          langEnglish      = true;
bool          languageConfigured = false;
int           langChoiceIndex  = 0;
State         currentState     = STATE_STARTUP;
int           menuIndex        = 0;
String        noteList[MAX_NOTES];
int           noteCount        = 0;
int           selectedNote     = 0;
String        aiImprovedNote   = "";
int           noteScrollOffset = 0;
bool          dots[6]          = {false};
bool          cellBuilding     = false;
bool          dotPrev[6]       = {false};
unsigned long dotLastMs[6]     = {0};
bool          capitalNext      = false;
bool          numberMode       = false;
Unit          history[MAX_HISTORY];
int           histLen          = 0;
String        pendingTitle     = "";
unsigned long lastDebounce     = 0;
unsigned long startupMillis    = 0;
bool          undoPrev         = false;
bool          undoPending      = false;
unsigned long undoPendingTime  = 0;
bool          backLongFired    = false;
unsigned long backHeldSince    = 0;
bool          backWarnSpoken   = false;
bool          selectPrev       = false;
bool          aiBusy           = false;
unsigned long lastAutosave     = 0;
QueueHandle_t audioFetchQueue = nullptr;
volatile bool  audioFetchIdle   = true;
bool          sdReady          = false;
bool          sdWarningPending = false;

const char* menuEN[] = { "New note", "Read Notes", "Language" };
const char* menuCH[] = { "Lemba latsopano", "Werenga malemba", "Chilankhulo" };

const Phrase PHRASES[] = {
  { "new_note",        "New note.",                     "Lemba latsopano."                },
  { "read_notes",      "Read notes.",                   "Werenga malemba."                },
  { "language",        "Language.",                     "Chilankhulo."                    },
  { "english",         "English.",                      "Chingelezi."                     },
  { "chichewa",        "Chichewa.",                     "Chichewa."                       },
  { "write_title",     "Write title.",                  "Lemba mutu."                     },
  { "title_saved",     "Title saved. Write note.",      "Mutu wasungidwa. Lemba lembalo." },
  { "note_saved",      "Note saved.",                   "Lembalo lasungidwa."             },
  { "nothing_typed",   "Nothing typed.",                "Palibe chalembedwa."             },
  { "cleared",         "Cleared.",                      "Zachotsedwa."                    },
  { "undo",            "Undo.",                         "Bwezerani."                      },
  { "undo_word",       "Undo word.",                    "Bwezerani mawu."                 },
  { "capital",         "Capital.",                      "Kalata yaikulu."                 },
  { "number",          "Number.",                       "Nambala."                        },
  { "space",           "Space.",                        "Mpata."                          },
  { "empty",           "Empty.",                        "Palibe kanthu."                  },
  { "no_notes",        "No notes.",                     "Palibe malemba."                 },
  { "reading_note",    "Reading note.",                 "Kuwerenga lembalo."              },
  { "end_of_note",     "End of note.",                  "Mapeto a lembalo."               },
  { "deleted",         "Deleted.",                      "Zachotsedwa."                    },
  { "cancelled",       "Cancelled.",                    "Zaletsedwa."                     },
  { "ai",              "A I.",                          "A I."                            },
  { "loading",         "Loading.",                      "Ikukonzekera."                   },
  { "ai_failed",       "A I failed.",                   "A I yalephera."                  },
  { "sd_missing",      "SD card missing. Saving temporarily.", "SD kulibe."                },
  { "choose_language", "Choose language.",              "Sankhani chilankhulo."           },
  { "lang_instructions", "Press down to change. Press select to choose.", "Dinani pansi kusintha. Dinani select kusankha." },
  { "kept",            "Original kept.",                "Lembalo losungidwa."             },
  { "ai_ready",        "A I result ready.",             "A I yapeza zosintha."            },
  { "storage_full",    "Storage full.",                 "Malo odzaza."                    },
};
const int PHRASE_COUNT = sizeof(PHRASES) / sizeof(PHRASES[0]);


// ============================================================
//  SETUP
// ============================================================
// ── OLED status helper ───────────────────────────────────────
void showStatus(const char* line1, const char* line2 = "", const char* line3 = "") {
  display.clearDisplay();
  display.setFont(); display.setTextSize(1); display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);  display.print(line1);
  display.setCursor(0, 18); display.print(line2);
  display.setCursor(0, 36); display.print(line3);
  display.display();
  Serial.print(F("[STATUS] ")); Serial.print(line1);
  if (strlen(line2) > 0) { Serial.print(F(" | ")); Serial.print(line2); }
  if (strlen(line3) > 0) { Serial.print(F(" | ")); Serial.print(line3); }
  Serial.println();
}

// ─── FILESYSTEM DIAGNOSTICS ───────────────────────────────────
// The recursive tree-printer and the totals block above it used to be
// duplicated verbatim in both setup() and the "fs" serial command.
// Consolidated here so there's exactly one implementation to maintain —
// same output as before, called from both places.
static void printFsTree(File dir, int depth) {
  File f = dir.openNextFile();
  while (f) {
    for (int i = 0; i < depth; i++) Serial.print(F("  "));
    if (f.isDirectory()) {
      Serial.print(F("[")); Serial.print(f.name()); Serial.println(F("]"));
      printFsTree(f, depth + 1);
    } else {
      Serial.print(f.name());
      Serial.print(F("  ")); Serial.print(f.size()); Serial.println(F(" B"));
    }
    f = dir.openNextFile();
  }
}

static void dumpFilesystemInfo(const char* headerLabel) {
  Serial.println();
  Serial.print(F("── ")); Serial.print(headerLabel); Serial.println(F(" ──────────────────"));
  Serial.print(F("  Total : ")); Serial.print(LittleFS.totalBytes()); Serial.println(F(" bytes"));
  Serial.print(F("  Used  : ")); Serial.print(LittleFS.usedBytes());  Serial.println(F(" bytes"));
  Serial.print(F("  Free  : ")); Serial.print(LittleFS.totalBytes() - LittleFS.usedBytes()); Serial.println(F(" bytes"));
  Serial.print(F("  Used% : ")); Serial.print((LittleFS.usedBytes() * 100) / LittleFS.totalBytes()); Serial.println(F("%"));
  Serial.println(F("── Files ──────────────────────────"));
  File fsRoot = LittleFS.open("/");
  printFsTree(fsRoot, 0);
  Serial.println(F("──────────────────────────────────\n"));
}

// ─── AUDIO ASSET DIAGNOSTICS ─────────────────────────────────
// The resolver searches word clips across several base folders, in priority
// order: /words/<lang> (canonical SD word audio, changes.txt §6), /voice/<lang>
// (alternate SD naming), /sfx/<lang> (legacy LittleFS letter/system assets),
// /data/<lang> (legacy fallback) — each on SD then LittleFS. This dumps the
// actual contents of those folders and probes a few known words so a
// missing/misplaced asset is obvious.
static void diagListDir(const char* label, fs::FS& fs, const char* path, int maxEntries) {
  File d = fs.open(path);
  if (!d || !d.isDirectory()) {
    Serial.print(F("  [")); Serial.print(label); Serial.print(F("] "));
    Serial.print(path); Serial.println(F(": missing or not a directory"));
    return;
  }
  Serial.print(F("  [")); Serial.print(label); Serial.print(F("] "));
  Serial.print(path); Serial.println(F(":"));
  File f = d.openNextFile();
  int n = 0;
  while (f && n < maxEntries) {
    if (!f.isDirectory()) {
      Serial.print(F("    ")); Serial.print(f.name());
      Serial.print(F(" ")); Serial.println(f.size());
      n++;
    }
    f = d.openNextFile();
  }
  if (n >= maxEntries) { Serial.print(F("    ... (truncated at ")); Serial.print(maxEntries); Serial.println(F(")")); }
}

static void diagProbe(const char* label, const char* path) {
  bool found = (sdReady && SD.exists(path)) || LittleFS.exists(path);
  Serial.print(F("  ")); Serial.print(label); Serial.print(F(" -> "));
  Serial.print(path); Serial.print(F(" : "));
  Serial.println(found ? F("FOUND") : F("MISSING"));
}

static void diagAudioAssets() {
  Serial.println(F("\n── Audio Asset Diagnostics ───────────────"));
  Serial.print(F("  lang=")); Serial.println(langEnglish ? "en" : "ch");
  Serial.print(F("  sdReady=")); Serial.println(sdReady ? "YES" : "NO");
  // NOTE: word resolution is single-directory only as of session 8
  // (/words/<lang> — the /voice, /sfx, /data fallback search was removed
  // as dead overhead). This diagnostic previously still listed all four,
  // which no longer matches how resolveAudioAsset() actually looks things
  // up — updated here to reflect the real current path: the word
  // directory itself, plus bank status (session 10) and index status
  // (session 9), since those are now the two layers actually in front of
  // real SD access.
  Serial.println(F("  -- word directory listing --"));
  diagListDir("SD", SD, langEnglish ? "/words/en" : "/words/ch", 80);
  Serial.println(F("  -- word bank status --"));
  Serial.print(F("  bank loaded: ")); Serial.println(wordBankIsLoaded() ? "YES" : "NO");
  if (wordBankIsLoaded()) Serial.print(F("  bank entries: ")), Serial.println(wordBankEntryCount());
  Serial.println(F("  -- probe specific words --"));
  diagProbe("grace",   langEnglish ? "/words/en/grace.mp3" : "/words/ch/grace.mp3");
  diagProbe("poverty", langEnglish ? "/words/en/poverty.mp3" : "/words/ch/poverty.mp3");
  diagProbe("a",       langEnglish ? "/words/en/a.mp3" : "/words/ch/a.mp3");
  Serial.println(F("──────────────────────────────────────────\n"));
}

void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(10));

  // Suppress ESP-IDF internal noise — only our Serial.print calls will show
  esp_log_level_set("*",            ESP_LOG_NONE);
  esp_log_level_set("dac_oneshot",  ESP_LOG_NONE);
  esp_log_level_set("dac_common",   ESP_LOG_NONE);
  esp_log_level_set("dac_continuous",ESP_LOG_NONE);
  esp_log_level_set("gpio",         ESP_LOG_NONE);
  esp_log_level_set("i2c",          ESP_LOG_NONE);

  Serial.println(F("\n\n══ DIGIBRAILLE v2 BOOT ══"));

  // ── Pins ─────────────────────────────────────────────────
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT);
  pinMode(BTN_DOWN, INPUT);
  pinMode(BTN_AISAVE, INPUT);
  pinMode(BTN_DELETE, INPUT);
  if (isValidPin(BTN_REREAD)) pinMode(BTN_REREAD, INPUT_PULLUP);
  for (int i = 0; i < 6; i++) pinMode(DOT_PINS[i], INPUT_PULLUP);
  logTs("PINS", "OK");

  // ── OLED ─────────────────────────────────────────────────
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  if (!display.begin(i2c_Address, true)) {
    Serial.println(F("[OLED] FATAL - cannot start"));
    while (true);
  }
  display.clearDisplay();
  display.fillScreen(SH110X_WHITE); display.display();
  vTaskDelay(pdMS_TO_TICKS(10));
  display.fillScreen(SH110X_BLACK); display.display();
  Serial.println(F("[OLED] OK"));

  offlineGrammarSetup();
  Serial.println(F("[GRAMMAR] Offline engine ready"));
  mp3Setup();
  Serial.println(F("[MP3] I2S output ready"));

  // ── LittleFS ─────────────────────────────────────────────
  showStatus("DigiBraille v2", "Starting up...", "");
  if (!LittleFS.begin(true)) {
    showStatus("ERROR", "LittleFS failed", "Reflash device");
    Serial.println(F("[FS] FATAL - LittleFS failed"));
    while (true);
  }
  ensureFolders();

  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  sdReady = SD.begin(SD_CS_PIN, SPI);
  if (sdReady) {
    logTs("SD", "Card mounted");
  } else {
    logTs("SD", "Card not mounted; LittleFS/SAM fallback active");
    sdWarningPending = true;
  }
  ensureFolders();

  if (!sdReady) {
    samSetup();
    Serial.println(F("[SAM] OK - no SD fallback enabled"));
  } else {
    Serial.println(F("[SAM] skipped - SD audio mode"));
  }

  // ── Filesystem info ────────────────────────────────────────
  dumpFilesystemInfo("LittleFS Info");

  // ── Config ───────────────────────────────────────────────
  loadConfig();
  Serial.print(F("[CFG] Language: ")); Serial.println(langEnglish ? "en" : "ch");

  // ── Word bank (optional, opt-in accelerator) — now tried FIRST ──────
  // Session 14 finding: with a large word library (thousands of clips),
  // buildWordAssetIndex()'s directory scan (openNextFile(), one entry at a
  // time) showed clearly quadratic growth in practice — each successive
  // 1000 files took over 3x longer than the previous 1000 on real hardware
  // (measured: 155s for files 1-1000, then 504s for files 1000-2000),
  // projecting to roughly FOUR HOURS to finish scanning ~9,629 files. The
  // bank index (bank.idx), by contrast, is one plain-text file read top to
  // bottom — ordinary sequential I/O with none of that per-position cost —
  // and it already contains the complete word list. So: try the bank
  // first; only pay for the slow full-folder scan if no usable bank exists
  // (e.g. this language hasn't been packed yet). If the bank loads, the
  // per-file index below is skipped entirely — resolveAudioAsset() checks
  // the bank first anyway, so the index would have been redundant work.
  bool bankLoaded = loadWordBank();

  // ── Word asset index — SKIPPED if the bank already covers this
  // language; otherwise the same fallback scan as before. ─────────────
  // Must come after loadConfig() (needs the real language) and before
  // loadNotes()/writeSeedNotes() (chain building/migration below uses the
  // index too). See mp3_player.cpp for the full rationale — this is what
  // eliminates the redundant existence-probe SD call per word, and lets
  // the numeral fix skip lookups for characters known not to exist.
  if (!bankLoaded) {
    Serial.println(F("[INDEX] No word bank loaded - falling back to per-file index scan"));
    buildWordAssetIndex();
  } else {
    Serial.println(F("[INDEX] Word bank loaded - skipping slow per-file directory scan"));
  }

  // ── Notes ────────────────────────────────────────────────
  writeSeedNotes();
  loadNotes();
  // List all cached SFX files for debugging
  Serial.println(F("[FS] SFX cache contents:"));
  File sfxRoot = LittleFS.open("/sfx/en");
  if (sfxRoot && sfxRoot.isDirectory()) {
    File sf = sfxRoot.openNextFile();
    while (sf) {
      Serial.print(F("  ")); Serial.print(sf.name());
      Serial.print(F(" ")); Serial.print(sf.size()); Serial.println(F("b"));
      sf = sfxRoot.openNextFile();
    }
  } else { Serial.println(F("  (empty or missing)")); }
  Serial.print(F("[FS] Notes loaded: ")); Serial.println(noteCount);

  // NOTE: diagAudioAssets() used to run here unconditionally on every boot.
  // It's a diagnostic-only scan (lists 6 folders + probes several words),
  // and each probe is a real blocking filesystem call — dozens of them,
  // each also spamming a Serial line. It added real boot time for zero
  // runtime benefit. Still available on demand via the "assets" serial
  // command below if you need to debug a missing/misplaced clip.

  // WiFi bring-up intentionally removed — device now runs fully offline
  // from SD/LittleFS audio assets (see mp3_player.cpp). Left out entirely
  // rather than commented-out so this file doesn't carry rotting dead code;
  // reintroducing online mode should restore this block from version
  // control history rather than uncommenting a stale copy.

  if (sdReady) {
    showStatus("Audio ready", "SD voice enabled", "");
  } else {
    showStatus("No SD card", "Using SAM voice", "");
  }

  // ── Ready ────────────────────────────────────────────────
  showStatus("Ready!", "", "");
  startupMillis = millis();
  currentState  = STATE_STARTUP;
  drawStartup();
  Serial.println(F("[BOOT] Complete — loop() starting"));
}


// ============================================================
//  LOOP
// ============================================================
void loop() {
  debugLogButtonTransitions("loop");
  // ── Serial monitor commands ───────────────────────────────
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); cmd.toLowerCase();
    if (cmd == "fs") {
      dumpFilesystemInfo("LittleFS Storage");
    }
    else if (cmd == "sd") {
      Serial.print(F("[SD] mounted=")); Serial.println(sdReady ? "YES" : "NO");
      if (sdReady) {
        Serial.print(F("[SD] type=")); Serial.println(SD.cardType());
        Serial.print(F("[SD] size MB=")); Serial.println((uint32_t)(SD.cardSize() / (1024 * 1024)));
        Serial.print(F("[SD] used MB=")); Serial.println((uint32_t)(SD.usedBytes() / (1024 * 1024)));
      }
    }
    else if (cmd == "assets") {
      diagAudioAssets();
    }
    else if (cmd == "rechain") {
      Serial.println(F("[CHAIN] Forcing full rebuild of all note chains..."));
      unsigned long t0 = millis();
      ensureNoteAudioChains(true);
      Serial.print(F("[CHAIN] Rebuild complete in "));
      Serial.print(millis() - t0); Serial.println(F(" ms"));
    }
    else if (cmd == "reindex") {
      Serial.println(F("[BANK] Reloading word bank (if present)..."));
      bool bankLoaded = loadWordBank();
      if (!bankLoaded) {
        Serial.println(F("[INDEX] No word bank loaded - rebuilding per-file index (this can take a long time with a large word library — see log session 14)..."));
        buildWordAssetIndex();
      } else {
        Serial.println(F("[INDEX] Word bank loaded - skipping slow per-file directory scan"));
        Serial.println(F("[INDEX] (if you added NEW word clips not yet in the bank, repack bank_en.idx/.bin on your PC first, then run reindex again)"));
      }
    }
    else if (cmd == "help") {
      Serial.println(F("\n── Serial Commands ─────────────────────"));
      Serial.println(F("  fs          show filesystem storage and files"));
      Serial.println(F("  sd          show SD card status"));
      Serial.println(F("  assets      list audio asset folders + probe words"));
      Serial.println(F("  rechain     force-rebuild all note audio chains"));
      Serial.println(F("  reindex     rebuild the word asset index + reload the word bank"));
      Serial.println(F("                (run this + rechain after adding new SD word"));
      Serial.println(F("                 clips OR copying a freshly-packed bank.bin/bank.idx)"));
      Serial.println(F("  help        show this list"));
      Serial.println(F("────────────────────────────────────────\n"));
    }
  }

  if (currentState == STATE_STARTUP) {
    if (millis() - startupMillis >= 2000) {
      lastDebounce = millis();
      if (languageConfigured) {
        Serial.println(F("[LOOP] Startup timer done - entering main menu"));
        menuIndex = 0;
        enterMainMenu();
      } else {
        Serial.println(F("[LOOP] Startup timer done - first setup language select"));
        langChoiceIndex = langEnglish ? 0 : 1;
        currentState = STATE_LANG_SELECT;
        drawLangSelect();
        speakPhrase("choose_language");
        speakPhrase(langChoiceIndex == 0 ? "english" : "chichewa");
        speakPhrase("lang_instructions");
      }
    }
    return;
  }
  if (currentState == STATE_WRITE_TITLE || currentState == STATE_NEW_NOTE) {
    handleBraillePad(); return;
  }

  switch (currentState) {
    case STATE_LANG_SELECT:    handleLangSelect();    break;
    case STATE_MAIN_MENU:      handleMainMenu();      break;
    case STATE_READ_NOTES:     handleReadNotes();     break;
    case STATE_VIEW_NOTE:      handleViewNote();      break;
    case STATE_AI_PREVIEW:     handleAIPreview();     break;
    case STATE_DELETE_CONFIRM: handleDeleteConfirm(); break;
    default: break;
  }
}