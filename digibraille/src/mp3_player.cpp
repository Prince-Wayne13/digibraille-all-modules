// ============================================================
//  MP3 PLAYER + VOICERSS — ESP8266Audio only
//  NO AudioTools in this file — ever
//  Uses I2S output for MAX98357A/MAX9857A-style amplifier boards
// ============================================================
#include "mp3_player.h"
#include "globals.h"
#include "sam_tts.h"
#include <SD.h>
#include "AudioFileSource.h"
#include "AudioFileSourceLittleFS.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

extern volatile bool audioFetchIdle;

static AudioGeneratorMP3  _mp3;
static AudioOutputI2S     _i2sOut;
static AudioFileSource*   _audioSrc = nullptr;

void mp3Setup() {
  _i2sOut.SetPinout(AMP_BCLK_PIN, AMP_LRC_PIN, AMP_DIN_PIN);
  _i2sOut.SetGain(0.6);
  logTs("AUDIO", "MP3 I2S configured");
}

bool isValidMp3(String path) {
  bool fromSd = sdReady && SD.exists(path.c_str());
  bool fromLittleFs = LittleFS.exists(path);
  if (!fromSd && !fromLittleFs) return false;

  File f = fromSd ? SD.open(path.c_str(), FILE_READ) : LittleFS.open(path, FILE_READ);
  if (!f) return false;
  int sz = f.size();
  // Read first 3 bytes — MP3 starts with ID3 (0x49 0x44 0x33) or sync (0xFF 0xFB)
  uint8_t header[3] = {0};
  f.read(header, 3);
  f.close();
  bool sizeOk   = sz > 500;
  bool headerOk = (header[0]==0xFF && (header[1]&0xE0)==0xE0) || // sync word
                  (header[0]==0x49 && header[1]==0x44 && header[2]==0x33); // ID3
  Serial.print(F("[MP3] Validate ")); Serial.print(fromSd ? F("SD ") : F("FS "));
  Serial.print(path);
  Serial.print(F(" sz=")); Serial.print(sz);
  Serial.print(F(" hdr=0x")); Serial.print(header[0],HEX);
  Serial.print(F(" ok=")); Serial.println(sizeOk && headerOk ? "YES" : "NO");
  return sizeOk && headerOk;
}

// ─── PLAY MP3 ────────────────────────────────────────────────
void playMP3(String path) {
  logTsValue("AUDIO", "MP3 START ", path);
  logTestEvent(3, "audio-start", path);
  unsigned long t0 = millis();

  if (_mp3.isRunning()) { _mp3.stop(); delay(20); }
  delay(30);

  if (_audioSrc) { delete _audioSrc; _audioSrc = nullptr; }

  bool fromSd = sdReady && SD.exists(path.c_str());
  _audioSrc = fromSd
    ? (AudioFileSource*)new AudioFileSourceSD(path.c_str())
    : (AudioFileSource*)new AudioFileSourceLittleFS(path.c_str());
  if (!_audioSrc || !_audioSrc->isOpen()) {
    logTsValue("AUDIO", "MP3 OPEN FAILED ", path);
    return;
  }
  logTs("AUDIO", fromSd ? "MP3 source SD" : "MP3 source LittleFS");

  _i2sOut.SetPinout(AMP_BCLK_PIN, AMP_LRC_PIN, AMP_DIN_PIN);
  _i2sOut.SetGain(0.6);
  _mp3.begin(_audioSrc, &_i2sOut);

  int loops = 0;
  while (_mp3.isRunning()) {
    debugLogButtonTransitions("mp3");
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
bool vrssFetch(String text, String path) {
  (void)text;
  (void)path;
  logTs("VRSS", "Removed in offline SD audio mode");
  return false;
}

// Offline audio router
void speakPhrase(const char* name) {
  Serial.print(F("[PHRASE] >> ")); Serial.println(name);
  if (langEnglish) {
    String path = sfxPath(name);
    if (isValidMp3(path)) {
      Serial.println(F("[PHRASE] MP3 valid — playing"));
      playMP3(path); return;
    }
    // Invalid or missing — use SAM immediately, queue fetch for later
    Serial.println(F("[PHRASE] MP3 missing/invalid — SAM now, fetch queued"));
    samSay(phraseEN(name));
    // Queue re-fetch on Core 1
  } else {
    String path = String(SFX_CH_FOLDER) + "/" + name + ".mp3";
    if (isValidMp3(path)) { playMP3(path); return; }
    Serial.println(F("[PHRASE] Chichewa — SAM fallback"));
    samSay(phraseEN(name));
  }
}

// Speak dynamic text — SAM immediately, queue MP3 fetch for next time
void speakText(String text, String mp3Path) {
  if (mp3Path.length() > 0 && isValidMp3(mp3Path)) {
    Serial.println(F("[TEXT] Playing cached MP3"));
    playMP3(mp3Path); return;
  }
  // SAM speaks immediately — no waiting
  Serial.println(F("[TEXT] SAM immediate"));
  samSayChunked(text);
  // Queue MP3 fetch for next time this note is opened
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

bool _safeQueue(String path, String text) {
  (void)text;
  Serial.print(F("[QUEUE] OFFLINE SD mode; not fetching "));
  Serial.println(path);
  return false;
}

void queueNoteAudio(int index) {
  String body = getNoteBody(index);
  if (body.length() == 0) return;
  _safeQueue(noteMp3Path(index), body);
}

void queueNoteTitleAudio(int index) {
  String title = getNoteTitle(index);
  if (title.length() == 0) return;
  _safeQueue(noteTitleMp3Path(index), title);
}
