// ============================================================
//  SAM TTS — AudioTools only
//  NO ESP8266Audio in this file — ever
//  DirectDAC uses dacWrite() — same as DacOutput in mp3_player
//  Both use oneshot mode — no DMA conflict
// ============================================================
#include "sam_tts.h"
#include "globals.h"
#include "AudioTools.h"
#include "sam_arduino.h"
#include "esp_log.h"

class DirectDAC : public AudioOutput {
public:
  bool begin(AudioInfo info) override {
    _periodUs = (info.sample_rate > 0) ? (1000000UL / info.sample_rate) : 45UL;
    _nextMicros = micros() + _periodUs;
    setAudioInfo(info);
    return true;
  }

  size_t write(const uint8_t* buf, size_t size) override {
    for (size_t i = 0; i + 1 < size; i += 2) {
      int16_t s = (int16_t)(buf[i] | (buf[i+1] << 8));
      dacWrite(25, (uint8_t)((s >> 8) + 128));
      unsigned long now = micros();
      if (now < _nextMicros) {
        delayMicroseconds(_nextMicros - now);
        _nextMicros += _periodUs;
      } else {
        _nextMicros = now + _periodUs;
      }
    }
    return size;
  }
private:
  unsigned long _periodUs = 45;
  unsigned long _nextMicros = 0;
};

static DirectDAC _samOut;
static SAM       _sam(_samOut);

void samSetup() {
  // Suppress all DAC-related ESP-IDF logs — only our Serial.prints show
  esp_log_level_set("dac_oneshot",   ESP_LOG_NONE);
  esp_log_level_set("dac_common",    ESP_LOG_NONE);
  esp_log_level_set("dac_continuous",ESP_LOG_NONE);

  _sam.setVoice(SAM::Elf);
  _sam.setPitch(64);
  _sam.setThroat(128);
  _sam.setSpeed(48);
  _sam.setMouth(150);
  AudioInfo info;
  info.sample_rate     = 22050;
  info.channels        = 1;
  info.bits_per_sample = 16;
  _samOut.begin(info);
  dacWrite(25, 128); // start at mid-rail
}

void samSay(const char* text) {
  String s = String(text);
  char last = s.length() > 0 ? s.charAt(s.length()-1) : 0;
  if (last != '.' && last != '!' && last != '?') s += '.';
  Serial.print(F("[SAM] START: ")); Serial.println(s);
  unsigned long t0 = millis();
  _sam.say(s.c_str());
  dacWrite(25, 128); // return to silence
  Serial.print(F("[SAM] DONE ")); Serial.print(millis()-t0); Serial.println(F("ms"));
}

void samSayString(String t) { samSay(t.c_str()); }

void samSayChunked(String text) {
  text.trim();
  while (text.length() > 0) {
    String chunk = ""; int words = 0, pos = 0;
    while (pos < (int)text.length() && words < 10) {
      int sp = text.indexOf(' ', pos);
      if (sp < 0) { chunk += text.substring(pos); pos = text.length(); words++; }
      else        { chunk += text.substring(pos, sp+1); pos = sp+1; words++; }
    }
    chunk.trim();
    if (chunk.length() > 0) samSayString(chunk);
    text = (pos < (int)text.length()) ? text.substring(pos) : "";
    text.trim();
  }
}

// Chunked with button interrupt — stops if any button pressed between chunks
void samSayChunkedInterruptible(String text) {
  text.trim();
  while (text.length() > 0) {
    if (!digitalRead(BTN_SELECT) || !digitalRead(BTN_BACK) ||
        !digitalRead(BTN_DOWN)   || !digitalRead(BTN_REREAD) ||
        !digitalRead(BTN_AISAVE) || !digitalRead(BTN_DELETE)) {
      Serial.println(F("[SAM] Interrupted by button"));
      dacWrite(25, 128);
      return;
    }
    String chunk = ""; int words = 0, pos = 0;
    while (pos < (int)text.length() && words < 8) {
      int sp = text.indexOf(' ', pos);
      if (sp < 0) { chunk += text.substring(pos); pos = text.length(); words++; }
      else        { chunk += text.substring(pos, sp+1); pos = sp+1; words++; }
    }
    chunk.trim();
    if (chunk.length() > 0) samSayString(chunk);
    text = (pos < (int)text.length()) ? text.substring(pos) : "";
    text.trim();
  }  // <-- this brace was being choked by the stray text after it
}