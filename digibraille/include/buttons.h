#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include "globals.h"
#include "sam_tts.h"
#include "mp3_player.h"
#include "offline_grammar.h"
#include "storage.h"
#include "display.h"

// draw functions declared in display.h — included via globals chain
bool isValidMp3(String path);
void playMP3(String path);

void handleLangSelect() {
  // Uncomment below to flood serial with button state — useful for debugging
  // Serial.print(F("[BTN] SEL=")); Serial.print(buttonPressed(BTN_SELECT));
  // Serial.print(F(" DOWN=")); Serial.println(buttonPressed(BTN_DOWN));

  unsigned long now = millis();
  unsigned long debounceLeft = (now - lastDebounce < DEBOUNCE_MS) ? (DEBOUNCE_MS - (now - lastDebounce)) : 0;
  if (debounceLeft > 0) {
    // Only print occasionally so serial is not flooded
    static unsigned long lastDebugPrint = 0;
    if (now - lastDebugPrint > 500) {
      lastDebugPrint = now;
      Serial.print(F("[LANG] Debounce blocking — ")); Serial.print(debounceLeft); Serial.println(F("ms left"));
    }
    return;
  }
  if (buttonPressed(BTN_SELECT)) {
    Serial.println(F("[LANG] SELECT pressed → English"));
    lastDebounce=millis(); langEnglish=true; saveConfig(); speakPhrase("english");
    currentState=STATE_MAIN_MENU; menuIndex=0; drawMainMenu();
  } else if (buttonPressed(BTN_DOWN)) {
    Serial.println(F("[LANG] DOWN pressed → Chichewa"));
    lastDebounce=millis(); langEnglish=false; saveConfig(); speakPhrase("chichewa");
    currentState=STATE_MAIN_MENU; menuIndex=0; drawMainMenu();
  }
}

void handleMainMenu() {
  if (millis()-lastDebounce<DEBOUNCE_MS) return;
  if (buttonPressed(BTN_DOWN)) {
    lastDebounce=millis(); menuIndex=(menuIndex+1)%MENU_COUNT; drawMainMenu();
    if (menuIndex==0) speakPhrase("new_note");
    else if (menuIndex==1) speakPhrase("read_notes");
    else speakPhrase("language");
  } else if (buttonPressed(BTN_SELECT)) {
    lastDebounce=millis();
    if (menuIndex==0) {
      histLen=0; pendingTitle=""; capitalNext=false; numberMode=false;
      for (int i=0;i<6;i++) dots[i]=false; cellBuilding=false;
      selectPrev=false; undoPrev=false; undoPending=false; backWarnSpoken=false; lastAutosave=millis();
      lastDebounce=millis();
      if (restoreDraft()) {
        brailleClearCell();
        drawBraillePad(pendingTitle.length() ? "Note:" : "Title:");
        samSayString("Draft restored.");
        if (histLen > 0) samSayChunked(brailleBufferString());
        return;
      }
      currentState=STATE_WRITE_TITLE;
      Serial.println(F("[STATE] Entering WRITE_TITLE"));
      drawBraillePad("Title:"); speakPhrase("write_title");
    } else if (menuIndex==1) {
      loadNotes(); selectedNote=0;
      lastDebounce=millis();
      currentState=STATE_READ_NOTES;
      Serial.print(F("[STATE] Entering READ_NOTES — notes: ")); Serial.println(noteCount);
      drawReadNotes();
      if (noteCount==0) speakPhrase("no_notes"); else {
      String _tp = noteTitleMp3Path(0);
      if (isValidMp3(_tp)) { playMP3(_tp); }
      else { samSayString(getNoteTitle(0)); queueNoteTitleAudio(0); }
    }
    } else {
      currentState=STATE_LANG_SELECT; drawLangSelect();
      lastDebounce=millis();
      speakPhrase("choose_language");
      speakPhrase("lang_instructions");
    }
  }
}

void handleReadNotes() {
  if (millis()-lastDebounce<DEBOUNCE_MS) return;
  if (buttonPressed(BTN_DOWN)) {
    lastDebounce=millis();
    if (noteCount>0) { selectedNote=(selectedNote+1)%noteCount; drawReadNotes(); {
      String _tp = noteTitleMp3Path(selectedNote);
      if (isValidMp3(_tp)) { playMP3(_tp); }
      else { samSayString(getNoteTitle(selectedNote)); queueNoteTitleAudio(selectedNote); }
    } }
  } else if (buttonPressed(BTN_SELECT)) {
    lastDebounce=millis();
    if (noteCount>0) {
      noteScrollOffset=0; currentState=STATE_VIEW_NOTE; drawViewNote(selectedNote);
      String _nmp3 = noteMp3Path(selectedNote);
      if (isValidMp3(_nmp3)) {
        Serial.println(F("[VIEW] Valid MP3 — playing full note"));
        speakPhrase("reading_note"); playMP3(_nmp3); speakPhrase("end_of_note");
      } else {
        Serial.println(F("[VIEW] No valid MP3 — SAM reads title only"));
        samSayString(getNoteTitle(selectedNote));
        queueNoteAudio(selectedNote);
      }
    }
  } else if (buttonPressed(BTN_REREAD)) {
    lastDebounce=millis(); if (noteCount>0) {
      String _tp = noteTitleMp3Path(selectedNote);
      if (isValidMp3(_tp)) { playMP3(_tp); }
      else { samSayString(getNoteTitle(selectedNote)); queueNoteTitleAudio(selectedNote); }
    }
  } else if (buttonPressed(BTN_DELETE)) {
    lastDebounce=millis();
    if (noteCount>0) { currentState=STATE_DELETE_CONFIRM; drawDeleteConfirm(); samSayString("delete "+getNoteTitle(selectedNote)+"?"); }
  } else if (buttonPressed(BTN_BACK)) {
    lastDebounce=millis(); menuIndex=0; currentState=STATE_MAIN_MENU; drawMainMenu(); speakPhrase("new_note");
  }
}

void handleViewNote() {
  if (millis()-lastDebounce<DEBOUNCE_MS) return;
  if (buttonPressed(BTN_DOWN)) {
    lastDebounce=millis(); noteScrollOffset++; drawViewNote(selectedNote);
    // Only speak line if no MP3 — if MP3 was played the whole note was heard already
    if (!isValidMp3(noteMp3Path(selectedNote))) {
      String body=getNoteBody(selectedNote); String lines[40]; int total=0; String tmp=body;
      while (tmp.length()>0&&total<40) {
        int len=min((int)tmp.length(),21);
        if (len==21&&tmp.length()>21) { int sp=tmp.lastIndexOf(' ',20); if(sp>8) len=sp+1; }
        lines[total++]=tmp.substring(0,len); tmp=tmp.substring(len); tmp.trim();
      }
      if (noteScrollOffset<total) samSayString(lines[noteScrollOffset]);
      else speakPhrase("end_of_note");
    }
  } else if (buttonPressed(BTN_REREAD)) {
    lastDebounce=millis();
    String mp3p = noteMp3Path(selectedNote);
    if (isValidMp3(mp3p)) {
      Serial.println(F("[REREAD] MP3 — playing"));
      speakPhrase("reading_note"); playMP3(mp3p); speakPhrase("end_of_note");
    } else {
      Serial.println(F("[REREAD] No MP3 — SAM interruptible"));
      speakPhrase("reading_note");
      samSayChunkedInterruptible(getNoteBody(selectedNote));
      speakPhrase("end_of_note");
      queueNoteAudio(selectedNote);
    }
  } else if (buttonPressed(BTN_AISAVE)) {
    lastDebounce=millis();
    if (aiBusy) { speakPhrase("loading"); return; }
    aiBusy=true; speakPhrase("ai"); speakPhrase("loading"); drawAILoading();
    String improved=improveNoteOffline(getNoteBody(selectedNote)); aiBusy=false;
    if (improved.length()>0) {
      aiImprovedNote=improved; currentState=STATE_AI_PREVIEW; drawAIPreview(improved);
      speakPhrase("ai_ready"); samSayChunked(improved);
    } else { speakPhrase("ai_failed"); samSayString("Keep writing or try again."); drawToast("Grammar failed!"); drawViewNote(selectedNote); }
  } else if (buttonPressed(BTN_DELETE)) {
    lastDebounce=millis(); currentState=STATE_DELETE_CONFIRM; drawDeleteConfirm();
    samSayString("delete "+getNoteTitle(selectedNote)+"?");
  } else if (buttonPressed(BTN_BACK)) {
    lastDebounce=millis(); noteScrollOffset=0; currentState=STATE_READ_NOTES; drawReadNotes(); {
      String _tp = noteTitleMp3Path(selectedNote);
      if (isValidMp3(_tp)) { playMP3(_tp); }
      else { samSayString(getNoteTitle(selectedNote)); queueNoteTitleAudio(selectedNote); }
    }
  }
}

void handleAIPreview() {
  if (millis()-lastDebounce<DEBOUNCE_MS) return;
  if (buttonPressed(BTN_REREAD)) { lastDebounce=millis(); samSayChunked(aiImprovedNote); }
  else if (buttonPressed(BTN_SELECT)) {
    lastDebounce=millis();
    String fullNote=getNoteTitle(selectedNote)+"\n"+aiImprovedNote;
    File f=LittleFS.open(noteTxtPath(selectedNote),FILE_WRITE); if(f){f.print(fullNote);f.close();}
    noteList[selectedNote]=fullNote; loadNotes();
    if (LittleFS.exists(noteMp3Path(selectedNote))) LittleFS.remove(noteMp3Path(selectedNote));
    queueNoteAudio(selectedNote); speakPhrase("note_saved"); drawToast("Updated!");
    noteScrollOffset=0; currentState=STATE_VIEW_NOTE; drawViewNote(selectedNote);
  } else if (buttonPressed(BTN_BACK)) {
    lastDebounce=millis(); aiImprovedNote=""; noteScrollOffset=0;
    currentState=STATE_VIEW_NOTE; drawViewNote(selectedNote); speakPhrase("kept");
  } else if (buttonPressed(BTN_DELETE)) {
    lastDebounce=millis(); aiImprovedNote=""; noteScrollOffset=0;
    currentState=STATE_VIEW_NOTE; drawViewNote(selectedNote); speakPhrase("cancelled");
  }
}

void handleDeleteConfirm() {
  if (millis()-lastDebounce<DEBOUNCE_MS) return;
  if (buttonPressed(BTN_SELECT)) {
    lastDebounce=millis();
    if (LittleFS.exists(noteTxtPath(selectedNote))) LittleFS.remove(noteTxtPath(selectedNote));
    if (LittleFS.exists(noteMp3Path(selectedNote))) LittleFS.remove(noteMp3Path(selectedNote));
    loadNotes();
    for (int i=selectedNote;i<noteCount-1;i++) noteList[i]=noteList[i+1];
    noteCount--;
    for (int i=0;i<MAX_NOTES;i++) if (LittleFS.exists(noteTxtPath(i))) LittleFS.remove(noteTxtPath(i));
    for (int i=0;i<noteCount;i++) { File f=LittleFS.open(noteTxtPath(i),FILE_WRITE); if(f){f.print(noteList[i]);f.close();} }
    if (selectedNote>=noteCount&&selectedNote>0) selectedNote--;
    speakPhrase("deleted"); drawToast("Deleted!"); currentState=STATE_READ_NOTES; drawReadNotes();
    if (noteCount==0) speakPhrase("no_notes"); else {
      String _tp = noteTitleMp3Path(selectedNote);
      if (isValidMp3(_tp)) { playMP3(_tp); }
      else { samSayString(getNoteTitle(selectedNote)); queueNoteTitleAudio(selectedNote); }
    }
  } else if (buttonPressed(BTN_BACK)) {
    lastDebounce=millis(); speakPhrase("cancelled");
    currentState=STATE_READ_NOTES; drawReadNotes();
    if (noteCount>0) {
      String _tp = noteTitleMp3Path(selectedNote);
      if (isValidMp3(_tp)) { playMP3(_tp); }
      else { samSayString(getNoteTitle(selectedNote)); queueNoteTitleAudio(selectedNote); }
    }
  }
}

