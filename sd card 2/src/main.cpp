#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

#define SD_CS_PIN   5
#define SD_SCK_PIN  14
#define SD_MISO_PIN 4
#define SD_MOSI_PIN 17

// MAX98357A / I2S pins copied from the 12s audio max9850 trial wiring.
#define I2S_BCLK 16
#define I2S_LRC  27
#define I2S_DIN  25

#define VOICE_DIR "/words/en"
#define INTRO_PLAY_COUNT 3

SPIClass sdSpi(HSPI);

AudioFileSourceSD *audioFile = nullptr;
AudioGeneratorMP3 *mp3 = nullptr;
AudioOutputI2S *audioOut = nullptr;

bool sdMounted = false;
String serialLine;

void printHelp();
void printCardInfo();
bool ensureVoiceDir();
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void handleSerialCommand(String command, uint32_t enterMs);
void playWord(String word, uint32_t enterMs);
void playPath(const String &path);
void playIntroTest();
bool gatherMp3Files(fs::FS &fs, const char *dirname, uint8_t levels, String *paths, size_t &count, size_t maxCount);
void playBeep(int toneHz, int durationMs);
void playAttemptBeep();
void playSuccessBeep();
void playFailureBeep();
void playTestTone();
void stopPlayback(const char *reason);
String wordToPath(String word);
String findMp3Path(fs::FS &fs, const char *dirname, const String &targetName, uint8_t levels);
String baseName(String path);
void timestamp(uint32_t atMs, const char *event);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("ESP32 SD voice player");
  Serial.println("---------------------");
  Serial.printf("SD pins: CS=%d SCK=%d MISO=%d MOSI=%d\n", SD_CS_PIN, SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
  Serial.printf("I2S pins: BCLK=%d LRC=%d DIN=%d\n", I2S_BCLK, I2S_LRC, I2S_DIN);

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  sdSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

  audioOut = new AudioOutputI2S();
  audioOut->SetPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  audioOut->SetGain(0.5);

  const uint32_t speeds[] = {4000000, 1000000, 400000};
  for (uint8_t i = 0; i < sizeof(speeds) / sizeof(speeds[0]); i++) {
    Serial.printf("[%lu ms] Trying SD mount at %lu Hz...\n", millis(), speeds[i]);
    if (SD.begin(SD_CS_PIN, sdSpi, speeds[i])) {
      Serial.printf("[%lu ms] SD mounted at %lu Hz.\n", millis(), speeds[i]);
      sdMounted = true;
      break;
    }

    playAttemptBeep();
    SD.end();
    delay(250);
  }

  if (!sdMounted) {
    Serial.printf("[%lu ms] SD card mount failed at all test speeds.\n", millis());
    playFailureBeep();
    printHelp();
    return;
  }

  playSuccessBeep();
  randomSeed(micros());
  printCardInfo();
  ensureVoiceDir();

  printHelp();
}

void loop() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      Serial.println();
      uint32_t enterMs = millis();
      String command = serialLine;
      serialLine = "";
      handleSerialCommand(command, enterMs);
    } else if (c == '\b' || c == 127) {
      if (serialLine.length() > 0) {
        serialLine.remove(serialLine.length() - 1);
        Serial.print("\b \b");
      }
    } else {
      serialLine += c;
      Serial.print(c);
    }
  }

  if (mp3 != nullptr && mp3->isRunning()) {
    if (!mp3->loop()) {
      stopPlayback("audio stopped");
    }
  }
}

void printHelp() {
  Serial.println();
  Serial.println("Type a word and press Enter to play /words/en/<word>.mp3");
  Serial.println("Commands:");
  Serial.println("  ls      - list the /words/en folder");
  Serial.println("  tone    - play a 1 second I2S test tone");
  Serial.println("  stop    - stop current playback");
  Serial.println("  help    - show this help");
  Serial.println();
}

void printCardInfo() {
  uint8_t cardType = SD.cardType();

  Serial.print("Card type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC/SDXC");
  } else {
    Serial.println("Unknown");
  }

  Serial.printf("Card size: %llu MB\n", SD.cardSize() / (1024 * 1024));
  Serial.printf("Filesystem total: %llu MB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Filesystem used: %llu MB\n", SD.usedBytes() / (1024 * 1024));
}

bool ensureVoiceDir() {
  if (SD.exists(VOICE_DIR)) {
    File dir = SD.open(VOICE_DIR, FILE_READ);
    bool ok = dir && dir.isDirectory();
    if (dir) {
      dir.close();
    }

    if (!ok) {
      Serial.printf("[%lu ms] %s exists but is not a directory.\n", millis(), VOICE_DIR);
    }
    return ok;
  }

  Serial.printf("[%lu ms] %s folder missing, creating it...\n", millis(), VOICE_DIR);
  if (SD.mkdir(VOICE_DIR)) {
    Serial.printf("[%lu ms] Created %s. Put MP3 files there, for example %s/hello.mp3\n",
                  millis(), VOICE_DIR, VOICE_DIR);
    return true;
  }

  Serial.printf("[%lu ms] Could not create %s. Create the folder %s on the SD card.\n",
                millis(), VOICE_DIR, VOICE_DIR);
  return false;
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  if (!fs.exists(dirname)) {
    Serial.printf("[%lu ms] Directory missing: %s\n", millis(), dirname);
    return;
  }

  File root = fs.open(dirname);
  if (!root) {
    Serial.printf("[%lu ms] Failed to open directory: %s\n", millis(), dirname);
    return;
  }

  if (!root.isDirectory()) {
    Serial.printf("[%lu ms] Not a directory: %s\n", millis(), dirname);
    return;
  }

  File file = root.openNextFile();
  if (!file) {
    Serial.printf("[%lu ms] Directory is empty: %s\n", millis(), dirname);
  }

  while (file) {
    if (file.isDirectory()) {
      Serial.printf("DIR : %s\n", file.path());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.printf("FILE: %s  SIZE: %u bytes\n", file.path(), static_cast<unsigned int>(file.size()));
    }

    file = root.openNextFile();
  }
}

void handleSerialCommand(String command, uint32_t enterMs) {
  command.trim();

  if (command.length() == 0) {
    return;
  }

  timestamp(enterMs, "enter pressed");
  Serial.printf("[%lu ms] input: %s\n", millis(), command.c_str());

  if (command == "ls") {
    listDir(SD, VOICE_DIR, 1);
  } else if (command == "tone") {
    playTestTone();
  } else if (command == "help") {
    printHelp();
  } else if (command == "stop") {
    stopPlayback("stop command");
  } else {
    playWord(command, enterMs);
  }
}

void playWord(String word, uint32_t enterMs) {
  if (!sdMounted) {
    Serial.printf("[%lu ms] SD not mounted.\n", millis());
    return;
  }

  stopPlayback("new word");

  String path = wordToPath(word);
  Serial.printf("[%lu ms] search started: %s\n", millis(), path.c_str());

  audioFile = new AudioFileSourceSD(path.c_str());
  mp3 = new AudioGeneratorMP3();

  if (!mp3->begin(audioFile, audioOut)) {
    Serial.printf("[%lu ms] search done: not found (%lu ms after enter)\n", millis(), millis() - enterMs);
    Serial.printf("[%lu ms] Type ls to see the exact files under %s.\n", millis(), VOICE_DIR);
    Serial.printf("[%lu ms] Make sure you type the exact file name without .mp3\n", millis());
    stopPlayback("start failed");
    return;
  }

  Serial.printf("[%lu ms] audio started: %s (%lu ms after enter)\n", millis(), path.c_str(), millis() - enterMs);
}

void playPath(const String &path) {
  if (!sdMounted) {
    Serial.printf("[%lu ms] SD not mounted.\n", millis());
    return;
  }

  stopPlayback("new path");
  File probe = SD.open(path.c_str(), FILE_READ);
  if (!probe || probe.isDirectory()) {
    if (probe) {
      probe.close();
    }
    Serial.printf("[%lu ms] intro path not found: %s\n", millis(), path.c_str());
    return;
  }

  size_t bytes = probe.size();
  probe.close();
  Serial.printf("[%lu ms] intro file opened: %s (%u bytes)\n", millis(), path.c_str(), static_cast<unsigned int>(bytes));

  audioFile = new AudioFileSourceSD(path.c_str());
  mp3 = new AudioGeneratorMP3();
  if (!mp3->begin(audioFile, audioOut)) {
    Serial.printf("[%lu ms] intro playback failed: %s\n", millis(), path.c_str());
    stopPlayback("intro start failed");
    return;
  }
}

bool gatherMp3Files(fs::FS &fs, const char *dirname, uint8_t levels, String *paths, size_t &count, size_t maxCount) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    if (root) {
      root.close();
    }
    return false;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (levels > 0) {
        gatherMp3Files(fs, file.path(), levels - 1, paths, count, maxCount);
        if (count >= maxCount) {
          file.close();
          break;
        }
      }
    } else {
      String filePath = file.path();
      String name = baseName(filePath);
      name.toLowerCase();
      if (name.endsWith(".mp3")) {
        if (count < maxCount) {
          paths[count++] = filePath;
        }
      }
    }
    file = root.openNextFile();
  }

  root.close();
  return true;
}

void playIntroTest() {
  if (!sdMounted) {
    return;
  }

  const size_t maxFiles = 64;
  String paths[maxFiles];
  size_t count = 0;
  if (!gatherMp3Files(SD, VOICE_DIR, 2, paths, count, maxFiles) || count == 0) {
    Serial.printf("[%lu ms] intro test skipped: no MP3 files found in %s\n", millis(), VOICE_DIR);
    return;
  }

  Serial.printf("[%lu ms] intro test found %u MP3 files\n", millis(), static_cast<unsigned int>(count));
  size_t playCount = (count < INTRO_PLAY_COUNT) ? count : INTRO_PLAY_COUNT;

  for (size_t i = 0; i < playCount; i++) {
    size_t j = random(count - i);
    String temp = paths[j];
    paths[j] = paths[count - i - 1];
    paths[count - i - 1] = temp;
  }

  for (size_t i = 0; i < playCount; i++) {
    const String path = paths[count - 1 - i];
    Serial.printf("[%lu ms] intro test playing (%u/%u): %s\n", millis(), static_cast<unsigned int>(i + 1), static_cast<unsigned int>(playCount), path.c_str());
    playPath(path);
    while (mp3 != nullptr && mp3->isRunning()) {
      mp3->loop();
    }
    delay(100);
  }

  Serial.printf("[%lu ms] intro test complete\n", millis());
}

void playBeep(int toneHz, int durationMs) {
  if (audioOut == nullptr) {
    return;
  }

  stopPlayback("beep");
  audioOut->SetPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  audioOut->SetRate(44100);
  audioOut->SetBitsPerSample(16);
  audioOut->SetChannels(2);
  audioOut->SetOutputModeMono(true);
  audioOut->SetGain(0.5);

  if (!audioOut->begin()) {
    return;
  }

  const int halfPeriodSamples = 44100 / (toneHz * 2);
  const int16_t amplitude = 10000;
  int32_t samples = (44100LL * durationMs) / 1000;

  for (int32_t i = 0; i < samples; i++) {
    int16_t value = ((i / halfPeriodSamples) % 2 == 0) ? amplitude : -amplitude;
    int16_t sample[2] = {value, value};
    while (!audioOut->ConsumeSample(sample)) {
      delay(1);
    }
  }

  audioOut->flush();
  audioOut->stop();
}

void playAttemptBeep() {
  playBeep(1200, 80);
}

void playSuccessBeep() {
  playBeep(2200, 120);
  delay(70);
  playBeep(2200, 120);
}

void playFailureBeep() {
  for (int i = 0; i < 5; i++) {
    playBeep(300, 120);
    if (i < 4) {
      delay(70);
    }
  }
}

void playTestTone() {
  if (audioOut == nullptr) {
    Serial.printf("[%lu ms] Audio output is not ready.\n", millis());
    return;
  }

  stopPlayback("test tone");

  const int sampleRate = 44100;
  const int toneHz = 440;
  const int halfPeriodSamples = sampleRate / (toneHz * 2);
  const int16_t amplitude = 12000;

  audioOut->SetPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  audioOut->SetRate(sampleRate);
  audioOut->SetBitsPerSample(16);
  audioOut->SetChannels(2);
  audioOut->SetOutputModeMono(true);
  audioOut->SetGain(1);

  if (!audioOut->begin()) {
    Serial.printf("[%lu ms] I2S failed to start.\n", millis());
    return;
  }

  Serial.printf("[%lu ms] test tone started on BCLK=%d LRC=%d DIN=%d\n",
                millis(), I2S_BCLK, I2S_LRC, I2S_DIN);

  for (int i = 0; i < sampleRate; i++) {
    int16_t value = ((i / halfPeriodSamples) % 2 == 0) ? amplitude : -amplitude;
    int16_t sample[2] = {value, value};
    while (!audioOut->ConsumeSample(sample)) {
      delay(1);
    }
  }

  audioOut->flush();
  audioOut->stop();
  Serial.printf("[%lu ms] test tone finished\n", millis());
}

void stopPlayback(const char *reason) {
  bool wasRunning = mp3 != nullptr && mp3->isRunning();

  if (mp3 != nullptr) {
    if (mp3->isRunning()) {
      mp3->stop();
    }
    delete mp3;
    mp3 = nullptr;
  }

  if (audioFile != nullptr) {
    delete audioFile;
    audioFile = nullptr;
  }

  if (wasRunning) {
    Serial.printf("[%lu ms] audio stopped: %s\n", millis(), reason);
  }
}

String wordToPath(String word) {
  word.trim();
  word.toLowerCase();

  String clean;
  clean.reserve(word.length());
  for (uint16_t i = 0; i < word.length(); i++) {
    char c = word.charAt(i);
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_') {
      clean += c;
    }
  }

  String path = VOICE_DIR;
  path += "/";
  path += clean;
  path += ".mp3";
  return path;
}

String findMp3Path(fs::FS &fs, const char *dirname, const String &targetName, uint8_t levels) {
  File root = fs.open(dirname);
  if (!root || !root.isDirectory()) {
    return "";
  }

  File file = root.openNextFile();
  while (file) {
    String path = file.path();
    if (file.isDirectory()) {
      if (levels > 0) {
        String nested = findMp3Path(fs, path.c_str(), targetName, levels - 1);
        if (nested.length() > 0) {
          file.close();
          root.close();
          return nested;
        }
      }
    } else {
      String name = baseName(path);
      name.toLowerCase();
      if (name == targetName) {
        file.close();
        root.close();
        return path;
      }
    }

    file = root.openNextFile();
  }

  root.close();
  return "";
}

String baseName(String path) {
  int slash = path.lastIndexOf('/');
  if (slash >= 0) {
    return path.substring(slash + 1);
  }
  return path;
}

void timestamp(uint32_t atMs, const char *event) {
  Serial.printf("[%lu ms] %s\n", atMs, event);
}
