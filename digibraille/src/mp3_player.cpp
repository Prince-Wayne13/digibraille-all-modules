// ============================================================
//  MP3 PLAYER + VOICERSS — ESP8266Audio only
//  NO AudioTools in this file — ever
//  Uses custom DacOutput class with dacWrite() to avoid
//  DMA conflict with SAM's AudioTools-based DirectDAC
// ============================================================
#include "mp3_player.h"
#include "globals.h"
#include "sam_tts.h"
#include <HTTPClient.h>
#include "AudioFileSourceLittleFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutput.h"

extern volatile bool audioFetchIdle;

#define VRSS_API_KEY  "72f72678e3ac4477a3638b69ef893e2d"
#define VRSS_HOST     "api.voicerss.org"
#define VRSS_VOICE    "Mike"
#define VRSS_LANG     "en-us"
#define VRSS_FORMAT      "22khz_16bit_mono"
#define VRSS_SAMPLE_RATE 22050
// ─── CUSTOM DAC OUTPUT ───────────────────────────────────────
// Uses dacWrite() just like SAM — no DMA, no conflict
class DacOutput : public AudioOutput {
public:
  bool begin() override {
    _logged = false;          // reset so DAC params log on every new play
    dacWrite(25, 128);
    SetRate(22050);
    SetBitsPerSample(16);
    SetChannels(1);
    UpdateDelay();
    return true;
  }

  bool SetRate(int hz) override {
    AudioOutput::SetRate(hz);
    UpdateDelay();
    return true;
  }

  bool ConsumeSample(int16_t sample[2]) override {
    if (!_logged) {
      Serial.print(F("[DAC] hertz="));    Serial.println(hertz);
      Serial.print(F("[DAC] bps="));      Serial.println(bps);
      Serial.print(F("[DAC] channels=")); Serial.println(channels);
      _logged = true;
    }
    uint8_t val = (uint8_t)((sample[LEFTCHANNEL] >> 8) + 128);
    dacWrite(25, val);

    unsigned long now = micros();
    if (now < _nextMicros) {
      delayMicroseconds(_nextMicros - now);
      
    } _nextMicros += _periodUs;
    return true;
  }

  void flush() override {
    dacWrite(25, 128);
  }

  bool stop() override {
    dacWrite(25, 128);
    return true;
  }

private:
  void UpdateDelay() {
    _periodUs   = (hertz > 0) ? (1000000UL / hertz) : 45UL;
    _nextMicros = micros() + _periodUs;
  }

  unsigned long _periodUs  = 45;
  unsigned long _nextMicros = 0;
  bool          _logged     = false;
};


static AudioGeneratorMP3  _mp3;
static DacOutput          _dacOut;
static AudioFileSourceLittleFS* _audioSrc = nullptr;

void mp3Setup() {
  _dacOut.begin();
}

// ─── VALIDATE MP3 FILE ───────────────────────────────────────
bool isValidMp3(String path) {
  if (!LittleFS.exists(path)) return false;
  File f = LittleFS.open(path, FILE_READ);
  if (!f) return false;
  int sz = f.size();
  // Read first 3 bytes — MP3 starts with ID3 (0x49 0x44 0x33) or sync (0xFF 0xFB)
  uint8_t header[3] = {0};
  f.read(header, 3);
  f.close();
  bool sizeOk   = sz > 500;
  bool headerOk = (header[0]==0xFF && (header[1]&0xE0)==0xE0) || // sync word
                  (header[0]==0x49 && header[1]==0x44 && header[2]==0x33); // ID3
  Serial.print(F("[MP3] Validate ")); Serial.print(path);
  Serial.print(F(" sz=")); Serial.print(sz);
  Serial.print(F(" hdr=0x")); Serial.print(header[0],HEX);
  Serial.print(F(" ok=")); Serial.println(sizeOk && headerOk ? "YES" : "NO");
  return sizeOk && headerOk;
}

// ─── PLAY MP3 ────────────────────────────────────────────────
void playMP3(String path) {
  WiFi.setSleep(true);
  Serial.print(F("[MP3] START: ")); Serial.println(path);
  unsigned long t0 = millis();

  // Stop any running decode cleanly
  if (_mp3.isRunning()) { _mp3.stop(); delay(20); }
  _dacOut.stop();
  delay(30);

  // Tear down old source
  if (_audioSrc) { delete _audioSrc; _audioSrc = nullptr; }

  // Open new source
  _audioSrc = new AudioFileSourceLittleFS(path.c_str());
  if (!_audioSrc || !_audioSrc->isOpen()) {
    Serial.println(F("[MP3] Cannot open")); return;
  }

  // Reset DAC — also resets _logged so params print fresh
  _dacOut.begin();

  // Hold DAC at mid-rail while MP3 decoder syncs to first frame
  // Without this the pin floats and picks up noise before audio starts
  for (uint8_t i = 0; i < 40; i++) {
    dacWrite(25, 128);
    delayMicroseconds(500);   // 40 × 500µs = 20ms total silence padding
  }

  _mp3.begin(_audioSrc, &_dacOut);

  int loops = 0;
  while (_mp3.isRunning()) {
    if (!_mp3.loop()) { _mp3.stop(); break; }
    loops++;
  }

  // Explicit silence + settle after last sample
  _dacOut.flush();
  dacWrite(25, 128);
  delay(10);

  // Tear down source
  if (_audioSrc) { delete _audioSrc; _audioSrc = nullptr; }

  Serial.print(F("[MP3] DONE ")); Serial.print(millis() - t0);
  Serial.print(F("ms loops=")); Serial.println(loops);
WiFi.setSleep(false);
}
// ─── VOICERSS FETCH ──────────────────────────────────────────
bool vrssFetch(String text, String path) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[VRSS] No WiFi")); return false;
  }
  Serial.print(F("[VRSS] Fetching: ")); Serial.println(text);

  String url = "http://" + String(VRSS_HOST) +
               "/?key=" + String(VRSS_API_KEY) +
               "&hl="   + String(VRSS_LANG)    +
               "&v="    + String(VRSS_VOICE)   +
               "&src="  + urlEncode(text)      +
               "&c=MP3&f=" + String(VRSS_FORMAT);

  HTTPClient http; http.begin(url); http.setTimeout(10000);
  int code = http.GET();
  if (code != 200) {
    Serial.print(F("[VRSS] HTTP err: ")); Serial.println(code);
    http.end(); return false;
  }

  // Check content type — must be audio not text error
  String ct = http.header("Content-Type");
  Serial.print(F("[VRSS] Content-Type: ")); Serial.println(ct);

  // Peek first bytes before saving
  WiFiClient* stream = http.getStreamPtr();
  uint8_t peek[16] = {0};
  int peeked = 0;
  unsigned long peekTimeout = millis() + 2000;
  while (peeked < 16 && millis() < peekTimeout) {
    if (stream->available()) peek[peeked++] = stream->read();
    else delay(1);
  }

  // Check if response is an error message (starts with "ERROR")
  if (peek[0]=='E' && peek[1]=='R' && peek[2]=='R') {
    char errBuf[64]; memset(errBuf,0,64);
    memcpy(errBuf, peek, min(peeked,63));
    Serial.print(F("[VRSS] API ERROR: ")); Serial.println(errBuf);
    http.end(); return false;
  }

  // Check MP3 sync word or ID3 header
  bool looksLikeMp3 = (peek[0]==0xFF && (peek[1]&0xE0)==0xE0) ||
                      (peek[0]==0x49 && peek[1]==0x44 && peek[2]==0x33);
  if (!looksLikeMp3) {
    Serial.print(F("[VRSS] Bad header: 0x"));
    Serial.print(peek[0],HEX); Serial.print(F(" 0x")); Serial.println(peek[1],HEX);
    http.end(); return false;
  }

  // Create folder if needed
  String folder = path.substring(0, path.lastIndexOf('/'));
  if (folder.length() > 0 && !LittleFS.exists(folder)) LittleFS.mkdir(folder);

  File f = LittleFS.open(path, FILE_WRITE);
  if (!f) { Serial.println(F("[VRSS] Cannot open file")); http.end(); return false; }

  // Write peeked bytes first
  f.write(peek, peeked);
  int total = peeked;

  // Stream rest
  int remaining = http.getSize() - peeked;
  uint8_t buf[512];
  unsigned long timeout = millis() + 10000;
  while ((remaining > 0 || remaining == -1) && millis() < timeout) {
    size_t av = stream->available();
    if (av) {
      int n = stream->readBytes(buf, min((int)av,(int)sizeof(buf)));
      f.write(buf, n); total += n;
      if (remaining > 0) remaining -= n;
      timeout = millis() + 5000;
    } else delay(1);
  }
  f.close(); http.end();
  Serial.print(F("[VRSS] Saved ")); Serial.print(total);
  Serial.print(F("b → ")); Serial.println(path);
  return total > 500;
}

// ─── AUDIO ROUTER ────────────────────────────────────────────
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
    if (WiFi.status() == WL_CONNECTED) {
      AudioFetchJob job;
      path.toCharArray(job.path, sizeof(job.path));
      strncpy(job.text, phraseEN(name), sizeof(job.text));
      xQueueSend(audioFetchQueue, &job, 0);
    }
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
  if (mp3Path.length() > 0 && WiFi.status() == WL_CONNECTED) {
    AudioFetchJob job;
    mp3Path.toCharArray(job.path, sizeof(job.path));
    // Truncate text to fit job buffer
    text.substring(0, sizeof(job.text)-1).toCharArray(job.text, sizeof(job.text));
    xQueueSend(audioFetchQueue, &job, 0);
    Serial.println(F("[TEXT] Note audio queued for Core 1 fetch"));
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
  AudioFetchJob job;
  while (true) {
    audioFetchIdle = (audioFetchQueue != nullptr && uxQueueMessagesWaiting(audioFetchQueue) == 0);
    if (xQueueReceive(audioFetchQueue, &job, portMAX_DELAY) != pdTRUE) continue;
    audioFetchIdle = false;

    // ── First boot: cache all menu phrases ───────────────────
    if (strcmp(job.path, "CACHE_MENU") == 0) {
      c1log("Starting menu audio cache...");
      if (WiFi.status() != WL_CONNECTED) { c1log("No WiFi — skip"); continue; }
      int total = PHRASE_COUNT, success = 0, failed = 0;
      for (int i = 0; i < total; i++) {
        String path = sfxPath(PHRASES[i].name);
        char logBuf[80];
        if (isValidMp3(path)) {
          snprintf(logBuf, sizeof(logBuf), "SKIP %d/%d %s", i+1, total, PHRASES[i].name);
          c1log(logBuf); success++; continue;
        }
        snprintf(logBuf, sizeof(logBuf), "GET %d/%d %s", i+1, total, PHRASES[i].name);
        c1log(logBuf);
        if (vrssFetch(String(PHRASES[i].en), path)) {
          success++;
          snprintf(logBuf, sizeof(logBuf), "OK  %d/%d %s", i+1, total, PHRASES[i].name);
        } else {
          failed++;
          snprintf(logBuf, sizeof(logBuf), "ERR %d/%d %s", i+1, total, PHRASES[i].name);
        }
        c1log(logBuf);
        delay(300);
      }
      if (failed == 0) {
        File marker = LittleFS.open("/sfx/en/cached.ok", FILE_WRITE);
        if (marker) { marker.print("ok"); marker.close(); }
        c1log("Cache complete — marker written");
      } else {
        char buf[48]; snprintf(buf, sizeof(buf), "Cache done %d ok %d failed — retry next boot", success, failed);
        c1log(buf);
      }
      continue;
    }

    // ── Normal job: fetch note or phrase audio ────────────────
    if (WiFi.status() != WL_CONNECTED) {
      char buf[72]; snprintf(buf, sizeof(buf), "No WiFi — skip %s", job.path);
      c1log(buf); continue;
    }
    if (isValidMp3(String(job.path))) {
      char buf[72]; snprintf(buf, sizeof(buf), "Already valid — skip %s", job.path);
      c1log(buf); continue;
    }

    // Retry up to 3 times with delay between attempts
    // Delay also prevents collision with Gemini calls on Core 0
    bool fetchOk = false;
    for (int attempt = 1; attempt <= 3; attempt++) {
      char buf[72];
      snprintf(buf, sizeof(buf), "Attempt %d/3: %s", attempt, job.path);
      c1log(buf);

      // Small delay so Core 0 WiFi calls (Gemini) can complete first
      delay(500 * attempt);

      if (WiFi.status() != WL_CONNECTED) {
        c1log("WiFi lost — aborting fetch");
        break;
      }

      if (vrssFetch(String(job.text), String(job.path))) {
        // Validate the saved file
        if (isValidMp3(String(job.path))) {
          snprintf(buf, sizeof(buf), "SUCCESS attempt %d — %s", attempt, job.path);
          c1log(buf);
          fetchOk = true;
          break;
        } else {
          snprintf(buf, sizeof(buf), "Saved but invalid — retry %d", attempt);
          c1log(buf);
          // Delete bad file so next attempt starts clean
          LittleFS.remove(job.path);
        }
      } else {
        snprintf(buf, sizeof(buf), "Fetch failed attempt %d", attempt);
        c1log(buf);
      }
    }

    if (!fetchOk) {
      char buf[72]; snprintf(buf, sizeof(buf), "All attempts failed: %s", job.path);
      c1log(buf);
    }
  }
}

bool _safeQueue(String path, String text) {
  if (audioFetchQueue == nullptr) return false;
  if (text.length() == 0) return false;
  AudioFetchJob job;
  path.toCharArray(job.path, sizeof(job.path));
  text.substring(0, sizeof(job.text)-1).toCharArray(job.text, sizeof(job.text));
  bool ok = xQueueSend(audioFetchQueue, &job, 0) == pdTRUE;
  Serial.print(F("[QUEUE] ")); Serial.print(ok ? "OK" : "FULL");
  Serial.print(F(" → ")); Serial.println(path);
  return ok;
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
