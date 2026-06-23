/*
  TRIAL PROJECT — VoiceRSS TTS -> I2S playback through MAX98357A
  -----------------------------------------------------------------
  Standalone trial built with PlatformIO. Separate from the main
  DigiBraille firmware — DigiBraille intentionally removed WiFi and
  VoiceRSS, so this code should not be merged into that project.
  It exists only to confirm the I2S + MAX98357A wiring works and that
  online TTS playback functions in isolation.

  Library used: "ESP8266Audio" by Earle Philhower (declared in
  platformio.ini under lib_deps — PlatformIO downloads it automatically
  on first build, no manual install needed).

  Flow:
    1. Connect to WiFi.
    2. Build a VoiceRSS request URL with API key + text.
    3. Download the returned MP3 into LittleFS as a temp file.
    4. Play that temp file out through I2S to the MAX98357A.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <LittleFS.h>

#include "AudioFileSourceLittleFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

#include "config.h"

// ---------- Audio objects ----------
AudioFileSourceLittleFS *file;
AudioGeneratorMP3       *mp3;
AudioOutputI2S          *out;

String urlEncode(const char* text) {
  const char hex[] = "0123456789ABCDEF";
  String encoded;

  while (*text) {
    const uint8_t c = static_cast<uint8_t>(*text++);
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' ||
        c == '.' || c == '~') {
      encoded += static_cast<char>(c);
    } else {
      encoded += '%';
      encoded += hex[c >> 4];
      encoded += hex[c & 0x0F];
    }
  }

  return encoded;
}

bool fetchVoiceRSS(const char* text, const char* path) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot fetch audio.");
    return false;
  }

  // Build the request URL. VoiceRSS takes the text in the "src" parameter.
  String url = "http://api.voicerss.org/?key=";
  url += VRSS_API_KEY;
  url += "&hl=en-us&c=MP3&f=16khz_16bit_mono&src=";
  url += urlEncode(text);

  Serial.println("Requesting: " + url);

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.printf("HTTP GET failed, code: %d\n", httpCode);
    http.end();
    return false;
  }

  // VoiceRSS returns raw MP3 bytes in the response body when the request succeeds.
  // If the key/params are wrong, it returns a short text error instead of MP3 —
  // worth checking file size if playback fails.
  File f = LittleFS.open(path, "w");
  if (!f) {
    Serial.println("Failed to open temp file for writing.");
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  uint8_t buf[512];
  int totalBytes = 0;
  while (http.connected() && (http.getSize() > 0 || http.getSize() == -1)) {
    size_t avail = stream->available();
    if (avail) {
      int c = stream->readBytes(buf, ((avail > sizeof(buf)) ? sizeof(buf) : avail));
      f.write(buf, c);
      totalBytes += c;
    } else {
      if (!http.connected()) break;
      delay(10);
    }
    if (http.getSize() > 0 && totalBytes >= http.getSize()) break;
  }

  f.close();
  http.end();

  Serial.printf("Downloaded %d bytes to %s\n", totalBytes, path);
  return totalBytes > 0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed.");
    return;
  }

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected.");

  // Step 1: fetch the audio from VoiceRSS into LittleFS
  bool ok = fetchVoiceRSS(TEXT_TO_SPEAK, TEMP_MP3_PATH);
  if (!ok) {
    Serial.println("Audio fetch failed, nothing to play.");
    return;
  }

  // Step 2: set up I2S output to the MAX98357A
  out = new AudioOutputI2S();
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  out->SetGain(0.5);  // 0.0 to 4.0 — start low, raise carefully if using headphones

  file = new AudioFileSourceLittleFS(TEMP_MP3_PATH);
  mp3  = new AudioGeneratorMP3();
  mp3->begin(file, out);

  Serial.println("Playback starting...");
}

void loop() {
  if (mp3 != nullptr && mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      Serial.println("Playback finished.");
    }
  }
}
