#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <SD.h>
#include "globals.h"

// Forward declaration — defined in mp3_player.cpp
void queueNoteAudio(int index);
void queueNoteTitleAudio(int index);
bool isValidMp3(String path);

void ensureFolders() {
  if (!LittleFS.exists(NOTES_FOLDER))   LittleFS.mkdir(NOTES_FOLDER);
  if (!LittleFS.exists("/sfx"))         LittleFS.mkdir("/sfx");
  if (!LittleFS.exists(SFX_EN_FOLDER)) LittleFS.mkdir(SFX_EN_FOLDER);
  if (!LittleFS.exists(SFX_CH_FOLDER)) LittleFS.mkdir(SFX_CH_FOLDER);
  if (sdReady && !SD.exists(NOTES_FOLDER)) SD.mkdir(NOTES_FOLDER);
}

inline bool noteStorageUsesSd() { return sdReady; }
inline bool noteExists(int index) {
  String path = noteTxtPath(index);
  return noteStorageUsesSd() ? SD.exists(path.c_str()) : LittleFS.exists(path);
}
inline File openNoteFile(int index, const char* mode) {
  String path = noteTxtPath(index);
  return noteStorageUsesSd() ? SD.open(path.c_str(), mode) : LittleFS.open(path, mode);
}
inline bool removeNoteFile(int index) {
  String path = noteTxtPath(index);
  return noteStorageUsesSd() ? SD.remove(path.c_str()) : LittleFS.remove(path);
}
inline void removeNoteAudioFiles(int index) {
  String body = noteMp3Path(index);
  String title = noteTitleMp3Path(index);
  if (SD.exists(body.c_str())) SD.remove(body.c_str());
  if (SD.exists(title.c_str())) SD.remove(title.c_str());
  if (LittleFS.exists(body)) LittleFS.remove(body);
  if (LittleFS.exists(title)) LittleFS.remove(title);
}

bool loadConfig() {
  languageConfigured = false;
  if (!LittleFS.exists(CONFIG_PATH)) return false;
  File f = LittleFS.open(CONFIG_PATH, FILE_READ); if (!f) return false;
  String lang = f.readString(); f.close(); lang.trim();
  if (lang != "en" && lang != "ch") return false;
  langEnglish = (lang == "en");
  langChoiceIndex = langEnglish ? 0 : 1;
  languageConfigured = true;
  return true;
}

void saveConfig() {
  File f = LittleFS.open(CONFIG_PATH, FILE_WRITE); if (!f) return;
  f.print(langEnglish ? "en" : "ch"); f.close();
  languageConfigured = true;
}

void loadNotes() {
  noteCount = 0;
  for (int i = 0; i < MAX_NOTES; i++) {
    if (noteExists(i)) {
      File f = openNoteFile(i, FILE_READ);
      if (f) { noteList[noteCount++] = f.readString(); f.close(); }
    }
  }
  Serial.print(F("[FS] Notes loaded: ")); Serial.println(noteCount);
  Serial.print(F("[FS] Note storage: ")); Serial.println(noteStorageUsesSd() ? "SD" : "LittleFS fallback");

  // Queue title + body audio for any note missing valid MP3s
  // Runs every boot — Core 1 fetches silently in background
  // SAM is instant fallback if MP3 not ready yet
  for (int i = 0; i < noteCount; i++) {
    String titleMp3 = noteTitleMp3Path(i);
    String bodyMp3  = noteMp3Path(i);
    bool titleOk = isValidMp3(titleMp3);
    bool bodyOk  = isValidMp3(bodyMp3);
    Serial.print(F("[FS] Note ")); Serial.print(i);
    Serial.print(F(" title=")); Serial.print(titleOk ? "OK" : "MISS");
    Serial.print(F(" body="));  Serial.println(bodyOk  ? "OK" : "MISS");
  }
}

void writeSeedNotes() {
  if (noteExists(0)) return;
  const char* seeds[] = {
    "Grace\nGrace, aged 18, is a blind girl from Zomba, Malawi. Poverty blocks her path to university, yet she burns with passion to become a disability rights advocate, giving voice to the sidelined.",
    "Second Note\nSecond note is here",
    "Third Note\nThird note saved ok"
};
  for (int i = 0; i < 3; i++) {
    File f = openNoteFile(i, FILE_WRITE);
    if (f) { f.print(seeds[i]); f.close(); }
  }
}

int saveNewNote(String note) {
  int slot = -1;
  for (int i = 0; i < MAX_NOTES; i++) {
    if (!noteExists(i)) { slot = i; break; }
  }
  if (slot == -1) return -1;
  File f = openNoteFile(slot, FILE_WRITE);
  if (f) { f.print(note); f.close(); }
  return slot;
}

void saveImprovedNote(int index, String improved) {
  String fullNote = getNoteTitle(index) + "\n" + improved;
  File f = openNoteFile(index, FILE_WRITE);
  if (f) { f.print(fullNote); f.close(); }
  noteList[index] = fullNote;
}

void saveDraft() {
  File f = LittleFS.open(DRAFT_PATH, FILE_WRITE); if (!f) return;
  f.println(pendingTitle);
  for (int i = 0; i < histLen; i++) f.print(history[i].value);
  f.close();
}

bool restoreDraft() {
  if (!LittleFS.exists(DRAFT_PATH)) return false;
  File f = LittleFS.open(DRAFT_PATH, FILE_READ); if (!f) return false;
  pendingTitle = f.readStringUntil('\n');
  pendingTitle.trim();
  String body = f.readString();
  f.close();
  body.trim();
  if (pendingTitle.length() == 0 && body.length() == 0) return false;

  histLen = 0;
  for (int i = 0; i < (int)body.length() && histLen < MAX_HISTORY; i++) {
    history[histLen++] = { body[i] };
  }
  capitalNext = false;
  numberMode = false;
  currentState = pendingTitle.length() ? STATE_NEW_NOTE : STATE_WRITE_TITLE;
  return true;
}

void clearDraft() { if (LittleFS.exists(DRAFT_PATH)) LittleFS.remove(DRAFT_PATH); }
