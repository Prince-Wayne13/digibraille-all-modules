// ============================================================
//  DIGIBRAILLE v2 — WAYNE INC.
//
//  WIRING:
//  OLED SDA → GPIO 21    OLED SCL → GPIO 22
//  BTN_SELECT → GPIO 14   BTN_BACK   → GPIO 27
//  BTN_DOWN   → GPIO 13   BTN_REREAD → GPIO 12
//  BTN_AISAVE → GPIO 4    BTN_DELETE → GPIO 5
//  SPEAKER    → GPIO 25 → PAM8403
//  BRAILLE DOTS: {32,33,18,19,23,26} → GND
//
//  BEFORE FLASHING: fill in VRSS_API_KEY in mp3_player.cpp
//
//  FILE STRUCTURE — why two .cpp files:
//  sam_tts.cpp   → AudioTools only   (SAM audio)
//  mp3_player.cpp → ESP8266Audio only (MP3 playback)
//  Both define a class called AudioOutput — keeping them in
//  separate .cpp translation units prevents the name clash.
// ============================================================

#include <functional>
#include "globals.h"
#include "esp_log.h"
#include "sam_tts.h"
#include "mp3_player.h"
#include "storage.h"
#include "display.h"
#include "braille.h"
#include "buttons.h"

#define WIFI_SSID     "wayne"
#define WIFI_PASSWORD "iamthegreatest"

// ─── OLED INSTANCE ───────────────────────────────────────────
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ─── ALL GLOBAL DEFINITIONS ──────────────────────────────────
bool          langEnglish      = true;
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
QueueHandle_t audioFetchQueue;
volatile bool  audioFetchIdle   = false;
bool          wifiCachePending = false;
wl_status_t   lastWifiStatus   = WL_DISCONNECTED;
unsigned long lastWifiCheck    = 0;

const int DOT_PINS[6] = {32, 33, 18, 19, 23, 26};

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
  { "undo_sentence",   "Undo sentence.",                "Bwezerani mawu."                 },
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
  { "no_wifi",         "No wifi.",                      "Palibe Wi-Fi."                   },
  { "no_wifi_ai",      "No wifi. Cannot improve note.", "Palibe Wi-Fi."                   },
  { "choose_language", "Choose language.",              "Sankhani chilankhulo."           },
  { "lang_instructions", "Press select for English. Press down for Chichewa.", "Dinani select Chingelezi. Dinani pansi Chichewa." },
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
  esp_log_level_set("wifi",         ESP_LOG_NONE);

  Serial.println(F("\n\n══ DIGIBRAILLE v2 BOOT ══"));

  // ── Pins ─────────────────────────────────────────────────
  pinMode(BTN_SELECT, INPUT_PULLUP); pinMode(BTN_BACK,   INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP); pinMode(BTN_REREAD, INPUT_PULLUP);
  pinMode(BTN_AISAVE, INPUT_PULLUP); pinMode(BTN_DELETE, INPUT_PULLUP);
  for (int i = 0; i < 6; i++) pinMode(DOT_PINS[i], INPUT_PULLUP);
  Serial.println(F("[PINS] OK"));

  // ── OLED ─────────────────────────────────────────────────
  if (!display.begin(i2c_Address, true)) {
    Serial.println(F("[OLED] FATAL - cannot start"));
    while (true);
  }
  display.clearDisplay();
  display.fillScreen(SH110X_WHITE); display.display();
  vTaskDelay(pdMS_TO_TICKS(10));
  display.fillScreen(SH110X_BLACK); display.display();
  Serial.println(F("[OLED] OK"));

  // ── SAM ──────────────────────────────────────────────────
  samSetup();
  Serial.println(F("[SAM] OK"));
  mp3Setup();
  Serial.println(F("[MP3] DAC output ready"));
  mp3Setup();
  Serial.println(F("[MP3] OK"));

  // ── LittleFS ─────────────────────────────────────────────
  showStatus("DigiBraille v2", "Starting up...", "");
  if (!LittleFS.begin(true)) {
    showStatus("ERROR", "LittleFS failed", "Reflash device");
    Serial.println(F("[FS] FATAL - LittleFS failed"));
    while (true);
  }
  ensureFolders();

  // ── Filesystem info ────────────────────────────────────────
  Serial.println(F("\n── LittleFS Info ──────────────────"));
  Serial.print(F("  Total : ")); Serial.print(LittleFS.totalBytes()); Serial.println(F(" bytes"));
  Serial.print(F("  Used  : ")); Serial.print(LittleFS.usedBytes());  Serial.println(F(" bytes"));
  Serial.print(F("  Free  : ")); Serial.print(LittleFS.totalBytes() - LittleFS.usedBytes()); Serial.println(F(" bytes"));
  Serial.print(F("  Used% : ")); Serial.print((LittleFS.usedBytes() * 100) / LittleFS.totalBytes()); Serial.println(F("%"));
  Serial.println(F("── Files ──────────────────────────"));
  std::function<void(File, int)> listDir = [&](File dir, int depth) {
    File f = dir.openNextFile();
    while (f) {
      for (int i = 0; i < depth; i++) Serial.print(F("  "));
      if (f.isDirectory()) {
        Serial.print(F("[")); Serial.print(f.name()); Serial.println(F("]"));
        listDir(f, depth + 1);
      } else {
        Serial.print(f.name());
        Serial.print(F("  ")); Serial.print(f.size()); Serial.println(F(" B"));
      }
      f = dir.openNextFile();
    }
  };
  File fsRoot = LittleFS.open("/");
  listDir(fsRoot, 0);
  Serial.println(F("──────────────────────────────────\n"));

  // ── Config ───────────────────────────────────────────────
  loadConfig();
  Serial.print(F("[CFG] Language: ")); Serial.println(langEnglish ? "en" : "ch");

  // ── Notes ────────────────────────────────────────────────
  // Core 1 queue must be created BEFORE loadNotes()
  // because loadNotes() queues audio fetch jobs
  audioFetchQueue = xQueueCreate(10, sizeof(AudioFetchJob));
  xTaskCreatePinnedToCore(audioFetchTask, "AudioFetch", 8192, nullptr, 1, nullptr, 1);
  Serial.println(F("[C1] Audio fetch task started on Core 1"));

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
/*
  // ── WiFi — start in background to reduce boot delay and current draw
  showStatus("WiFi", "Starting in background", "");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleep(true);

  if (WiFi.status() == WL_CONNECTED) {
    char ipBuf[32]; WiFi.localIP().toString().toCharArray(ipBuf, 32);
    showStatus("WiFi Connected", ipBuf, "");
    Serial.print(F("[WIFI] Connected — ")); Serial.println(WiFi.localIP());
  } else {
    showStatus("WiFi", "Connecting in background", "");
    Serial.println(F("[WIFI] Starting in background"));
  }
*/
  // ── Core 1 — audio fetch task ────────────────────────────
  // ── First run audio cache ────────────────────────────────
  // Core 1 will handle this — signal it via a flag file
  // If /sfx/en/cached.ok does not exist — first run
  if (!LittleFS.exists("/sfx/en/cached.ok")) {
    if (WiFi.status() == WL_CONNECTED) {
      showStatus("First run setup", "Getting audio...", "Please wait");
      Serial.println(F("[BOOT] First run — queueing menu audio cache"));
      AudioFetchJob job;
      strncpy(job.path, "CACHE_MENU", sizeof(job.path));
      strncpy(job.text, "", sizeof(job.text));
      xQueueSend(audioFetchQueue, &job, portMAX_DELAY);
    } else {
      wifiCachePending = true;
      Serial.println(F("[BOOT] Cache pending until WiFi connects"));
      showStatus("No WiFi", "Using SAM voice", "Connect later");
    }
  } else {
    Serial.println(F("[BOOT] Menu audio already cached"));
    showStatus("Audio ready", "All phrases cached", "");
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
  // ── Serial monitor commands ───────────────────────────────
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); cmd.toLowerCase();
    if (cmd == "fs") {
      Serial.println(F("\n── LittleFS Storage ───────────────────"));
      Serial.print(F("  Total : ")); Serial.print(LittleFS.totalBytes()); Serial.println(F(" bytes"));
      Serial.print(F("  Used  : ")); Serial.print(LittleFS.usedBytes());  Serial.println(F(" bytes"));
      Serial.print(F("  Free  : ")); Serial.print(LittleFS.totalBytes() - LittleFS.usedBytes()); Serial.println(F(" bytes"));
      Serial.print(F("  Used% : ")); Serial.print((LittleFS.usedBytes() * 100) / LittleFS.totalBytes()); Serial.println(F("%"));
      Serial.println(F("── Files ───────────────────────────────"));
      std::function<void(File, int)> listDir = [&](File dir, int depth) {
        File f = dir.openNextFile();
        while (f) {
          for (int i = 0; i < depth; i++) Serial.print(F("  "));
          if (f.isDirectory()) {
            Serial.print(F("[")); Serial.print(f.name()); Serial.println(F("]"));
            listDir(f, depth + 1);
          } else {
            Serial.print(f.name());
            Serial.print(F("  ")); Serial.print(f.size()); Serial.println(F(" B"));
          }
          f = dir.openNextFile();
        }
      };
      File fsRoot = LittleFS.open("/");
      listDir(fsRoot, 0);
      Serial.println(F("────────────────────────────────────────\n"));
    }
    else if (cmd == "wipe cache") {
      if (LittleFS.exists("/sfx/en/cached.ok")) {
        LittleFS.remove("/sfx/en/cached.ok");
        Serial.println(F("[CMD] cached.ok removed — menu audio will re-fetch on next boot"));
      } else {
        Serial.println(F("[CMD] cached.ok not found"));
      }
    }
    else if (cmd == "help") {
      Serial.println(F("\n── Serial Commands ─────────────────────"));
      Serial.println(F("  fs          show filesystem storage and files"));
      Serial.println(F("  wipe cache  delete cached.ok to force re-fetch"));
      Serial.println(F("  help        show this list"));
      Serial.println(F("────────────────────────────────────────\n"));
    }
  }

  if (currentState == STATE_STARTUP) {
    if (millis() - startupMillis >= 2000) {
      Serial.println(F("[LOOP] Startup timer done — entering lang select"));
      lastDebounce = millis();
      currentState = STATE_LANG_SELECT;
      drawLangSelect();
      Serial.println(F("[LOOP] drawLangSelect done — about to speak"));
      speakPhrase("choose_language");
      Serial.println(F("[LOOP] choose_language done — about to speak instructions"));
      speakPhrase("lang_instructions");
      Serial.println(F("[LOOP] *** BUTTONS NOW ACTIVE — lang select ready ***"));
    }
    return;
  }

  if (currentState == STATE_WRITE_TITLE || currentState == STATE_NEW_NOTE) {
    handleBraillePad(); return;
  }

  // Periodic WiFi icon refresh
  if (millis() - lastWifiCheck > WIFI_CHECK_MS) {
    lastWifiCheck = millis();
    wl_status_t nowStatus = WiFi.status();
    if (nowStatus != lastWifiStatus) {
      lastWifiStatus = nowStatus;
      // Redraw status bar on whichever screen we are on
      drawStatusBar();
      display.display();

      if (nowStatus == WL_CONNECTED && wifiCachePending) {
        wifiCachePending = false;
        Serial.println(F("[WIFI] Connected — queueing first-run cache"));
        AudioFetchJob job;
        strncpy(job.path, "CACHE_MENU", sizeof(job.path));
        strncpy(job.text, "", sizeof(job.text));
        xQueueSend(audioFetchQueue, &job, portMAX_DELAY);
      }
    }
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
