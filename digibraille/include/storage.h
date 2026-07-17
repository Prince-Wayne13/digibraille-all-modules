#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <SD.h>
#include "globals.h"
#include "mp3_player.h"

bool verifySdNotesFolder() {
  if (!sdReady) {
    Serial.println(F("[SD] Notes folder check skipped: card not mounted"));
    return false;
  }

  if (!SD.exists(NOTES_FOLDER)) {
    Serial.print(F("[SD] Missing ")); Serial.print(NOTES_FOLDER);
    Serial.println(F("; creating it"));
    if (!SD.mkdir(NOTES_FOLDER)) {
      Serial.print(F("[SD] ERROR mkdir failed for "));
      Serial.println(NOTES_FOLDER);
      return false;
    }
  }

  String probePath = String(NOTES_FOLDER) + "/.probe";
  File probe = SD.open(probePath.c_str(), FILE_WRITE);
  if (!probe) {
    Serial.print(F("[SD] ERROR cannot write probe file "));
    Serial.println(probePath);
    return false;
  }
  probe.print("ok");
  probe.close();

  probe = SD.open(probePath.c_str(), FILE_READ);
  if (!probe) {
    Serial.print(F("[SD] ERROR cannot read probe file "));
    Serial.println(probePath);
    return false;
  }
  String value = probe.readString();
  probe.close();
  SD.remove(probePath.c_str());
  value.trim();
  if (value != "ok") {
    Serial.println(F("[SD] ERROR probe verification failed"));
    return false;
  }

  Serial.print(F("[SD] Notes folder verified: "));
  Serial.println(NOTES_FOLDER);
  return true;
}

void ensureFolders() {
  if (!LittleFS.exists(NOTES_FOLDER))   LittleFS.mkdir(NOTES_FOLDER);
  if (!LittleFS.exists("/sfx"))         LittleFS.mkdir("/sfx");
  if (!LittleFS.exists(SFX_EN_FOLDER)) LittleFS.mkdir(SFX_EN_FOLDER);
  if (!LittleFS.exists(SFX_CH_FOLDER)) LittleFS.mkdir(SFX_CH_FOLDER);
  if (sdReady && !verifySdNotesFolder()) {
    sdReady = false;
    sdWarningPending = true;
    Serial.println(F("[SD] Falling back to LittleFS notes because SD notes folder is not usable"));
  }
}

inline bool noteStorageUsesSd() { return sdReady; }
inline bool noteExists(int index) {
  String path = noteTxtPath(index);
  return noteStorageUsesSd() ? SD.exists(path.c_str()) : LittleFS.exists(path);
}
inline File openNoteFile(int index, const char* mode) {
  String path = noteTxtPath(index);
  File f = noteStorageUsesSd() ? SD.open(path.c_str(), mode) : LittleFS.open(path, mode);
  if (!f) {
    Serial.print(F("[FS] ERROR open note failed storage="));
    Serial.print(noteStorageUsesSd() ? F("SD ") : F("LittleFS "));
    Serial.print(F("path=")); Serial.print(path);
    Serial.print(F(" mode=")); Serial.println(mode);
    if (noteStorageUsesSd() && !SD.exists(NOTES_FOLDER)) {
      Serial.println(F("[FS] SD /notes folder is missing"));
    }
    if (noteStorageUsesSd()) sdWarningPending = true;
  }
  return f;
}
inline bool removeNoteFile(int index) {
  String path = noteTxtPath(index);
  return noteStorageUsesSd() ? SD.remove(path.c_str()) : LittleFS.remove(path);
}
inline void removeNoteAudioFiles(int index) {
  String body = noteChainPath(index);
  String title = noteTitleChainPath(index);
  if (sdReady && SD.exists(body.c_str())) SD.remove(body.c_str());
  if (sdReady && SD.exists(title.c_str())) SD.remove(title.c_str());
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

// Builds body + title chains ONLY for notes that don't have one yet.
// This used to unconditionally rebuild every note's chains on every boot
// (to pick up newly-added SD word assets), but that made boot painfully
// slow: each word in each note pays up to 8 blocking filesystem probes
// (4 candidate folders x 2 filesystems) during resolveAudioAsset(), so a
// 3-note / ~60-word note set added *minutes* of dead time before the menu
// was usable — the exact "stuck at Read Notes" symptom reported in
// test2_v2.txt.
//
// Trade-off accepted: if you add a new word asset to the SD card (e.g.
// grace.mp3) AFTER a note referencing that word was already saved, that
// note's existing chain won't pick it up automatically anymore. Use the
// serial "rechain" command (see main.cpp) to force a rebuild after adding
// assets, or just re-save the note. This is a deliberate, explicit
// trigger instead of a hidden per-boot cost.
//
// Iterates real file slots — not the compacted noteList — so it stays
// correct even if note slots ever have gaps.
void ensureNoteAudioChains(bool force = false) {
  for (int i = 0; i < MAX_NOTES; i++) {
    if (!noteExists(i)) continue;

    File f = openNoteFile(i, FILE_READ);
    if (!f) continue;
    String full = f.readString();
    f.close();
    int nl = full.indexOf('\n');
    String title = (nl < 0) ? full : full.substring(0, nl);
    String body  = (nl < 0) ? "" : full.substring(nl + 1);
    body.trim(); title.trim();

    // A genuinely empty body/title (nothing typed) will correctly produce
    // a chain with zero W:/L: lines — that's not "broken," so don't treat
    // it as forever-missing and rebuild it every boot. Only chains for
    // NON-empty text are expected to contain real word lines.
    bool needBody  = force || (body.length()  > 0 && !noteAudioChainExists(i));
    bool needTitle = force || (title.length() > 0 && !noteTitleChainExists(i));
    if (!needBody && !needTitle) continue;

    bool okB = true, okT = true;
    if (needBody)  okB = buildNoteAudioChain(i, body);
    if (needTitle) okT = buildNoteTitleChain(i, title);
    Serial.print(F("[CHAIN] note ")); Serial.print(i);
    Serial.print(F(" body=")); Serial.print(needBody ? (okB ? "BUILT" : "FAIL") : "SKIP");
    Serial.print(F(" title=")); Serial.println(needTitle ? (okT ? "BUILT" : "FAIL") : "SKIP");
  }
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

  // Build body + title chains ONLY for notes that don't have one yet
  // (new notes / first migration). Does NOT rebuild existing chains —
  // see ensureNoteAudioChains() above for why. This keeps boot fast
  // regardless of note count/length.
  ensureNoteAudioChains();

  // Report title + body audio status for every loaded note
  // SAM is instant fallback if a chain isn't ready yet
  for (int i = 0; i < noteCount; i++) {
    bool titleOk = noteTitleChainExists(i);
    bool bodyOk  = noteAudioChainExists(i);
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