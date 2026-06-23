#pragma once
#include <Arduino.h>
#include <LittleFS.h>
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
}

void loadConfig() {
  if (!LittleFS.exists(CONFIG_PATH)) return;
  File f = LittleFS.open(CONFIG_PATH, FILE_READ); if (!f) return;
  String lang = f.readString(); f.close(); lang.trim();
  langEnglish = (lang == "en");
}

void saveConfig() {
  File f = LittleFS.open(CONFIG_PATH, FILE_WRITE); if (!f) return;
  f.print(langEnglish ? "en" : "ch"); f.close();
}

void loadNotes() {
  noteCount = 0;
  for (int i = 0; i < MAX_NOTES; i++) {
    if (LittleFS.exists(noteTxtPath(i))) {
      File f = LittleFS.open(noteTxtPath(i), FILE_READ);
      if (f) { noteList[noteCount++] = f.readString(); f.close(); }
    }
  }
  Serial.print(F("[FS] Notes loaded: ")); Serial.println(noteCount);

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
    if (!titleOk) queueNoteTitleAudio(i);
    if (!bodyOk)  queueNoteAudio(i);
  }
}

void writeSeedNotes() {
  if (LittleFS.exists(noteTxtPath(0))) return;
  const char* seeds[] = {
    "Grace\nGrace, aged 18, is a blind girl from Zomba, Malawi. Poverty blocks her path to university, yet she burns with passion to become a disability rights advocate, giving voice to the sidelined.",
    "Second Note\nSecond note is here",
    "Third Note\nThird note saved ok"
};
  for (int i = 0; i < 3; i++) {
    File f = LittleFS.open(noteTxtPath(i), FILE_WRITE);
    if (f) { f.print(seeds[i]); f.close(); }
  }
}

int saveNewNote(String note) {
  int slot = -1;
  for (int i = 0; i < MAX_NOTES; i++) {
    if (!LittleFS.exists(noteTxtPath(i))) { slot = i; break; }
  }
  if (slot == -1) return -1;
  File f = LittleFS.open(noteTxtPath(slot), FILE_WRITE);
  if (f) { f.print(note); f.close(); }
  return slot;
}

void saveImprovedNote(int index, String improved) {
  String fullNote = getNoteTitle(index) + "\n" + improved;
  File f = LittleFS.open(noteTxtPath(index), FILE_WRITE);
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
