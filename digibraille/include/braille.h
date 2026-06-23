#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "globals.h"
#include "sam_tts.h"
#include "mp3_player.h"
#include "storage.h"

#define BRAILLE_CAPITAL_IND  0b100000
#define BRAILLE_NUMBER_IND   0b111100

// ─── DECODER ─────────────────────────────────────────────────
inline char decodeBrailleLetter(uint8_t code) {
  switch (code) {
    case 0b000001: return 'a'; case 0b000011: return 'b'; case 0b001001: return 'c';
    case 0b011001: return 'd'; case 0b010001: return 'e'; case 0b001011: return 'f';
    case 0b011011: return 'g'; case 0b010011: return 'h'; case 0b001010: return 'i';
    case 0b011010: return 'j'; case 0b000101: return 'k'; case 0b000111: return 'l';
    case 0b001101: return 'm'; case 0b011101: return 'n'; case 0b010101: return 'o';
    case 0b001111: return 'p'; case 0b011111: return 'q'; case 0b010111: return 'r';
    case 0b001110: return 's'; case 0b011110: return 't'; case 0b100101: return 'u';
    case 0b100111: return 'v'; case 0b111010: return 'w'; case 0b101101: return 'x';
    case 0b111101: return 'y'; case 0b110101: return 'z'; case 0b110010: return '.';
    case 0b010000: return '!'; case 0b001100: return '?';
    default: return 0;
  }
}
inline char decodeBrailleNumber(uint8_t code) {
  switch (code) {
    case 0b000001: return '1'; case 0b000011: return '2'; case 0b001001: return '3';
    case 0b011001: return '4'; case 0b010001: return '5'; case 0b001011: return '6';
    case 0b011011: return '7'; case 0b010011: return '8'; case 0b001010: return '9';
    case 0b011010: return '0'; default: return 0;
  }
}
inline char decodeBraille(uint8_t code, bool& capNext, bool& numMode) {
  if (code == 0) return ' ';
  if (code == BRAILLE_CAPITAL_IND) { capNext = true; return 0; }
  if (code == BRAILLE_NUMBER_IND)  { numMode = true; return 0; }
  if (numMode) { char n = decodeBrailleNumber(code); if (n) return n; numMode = false; }
  char c = decodeBrailleLetter(code); if (c == 0) return '?';
  if (capNext) { capNext = false; return (char)(c-32); }
  return c;
}

// display.h and storage.h are included before braille.h in the .ino
// so drawBraillePad, drawMainMenu, drawToast, loadNotes, saveDraft, clearDraft
// are all already declared — no forward declarations needed here

// ─── PAD LOGIC ───────────────────────────────────────────────
void brailleClearCell() { for (int i=0;i<6;i++) dots[i]=false; cellBuilding=false; }

String brailleBufferString() {
  String buf = "";
  for (int i=0;i<histLen;i++) buf += history[i].value;
  return buf;
}

void speakUndoResult(char removed) {
  if (removed == 0) { speakPhrase("empty"); return; }
  String msg = "Removed ";
  if (removed == ' ') msg += "space";
  else msg += removed;
  msg += ". Now reads ";
  String buf = brailleBufferString();
  msg += buf.length() ? buf : "empty";
  samSayChunked(msg);
}

void brailleCommit() {
  uint8_t code = 0;
  for (int i=0;i<6;i++) if (dots[i]) code|=(1<<i);
  const char* lbl = (currentState==STATE_WRITE_TITLE)?"Title:":"Note:";
  if (code==BRAILLE_CAPITAL_IND) { capitalNext=true; speakPhrase("capital"); brailleClearCell(); drawBraillePad(lbl); return; }
  if (code==BRAILLE_NUMBER_IND)  { numberMode=true;  speakPhrase("number");  brailleClearCell(); drawBraillePad(lbl); return; }
  char c = decodeBraille(code, capitalNext, numberMode);
  if (c==' ') {
    numberMode=false;
    if (histLen<MAX_HISTORY) { history[histLen++]={' '}; speakPhrase("space"); }
  } else if (c!=0 && c!='?') {
    if (histLen<MAX_HISTORY) {
      history[histLen++]={c};
      if (c>='A'&&c<='Z') { String s="capital "; s+=(char)(c+32); samSayString(s); }
      else if (c>='0'&&c<='9') { char nb[2]={c,0}; samSay(nb); }
      else { char buf[2]={c,0}; samSay(buf); }
    }
  } else samSay("unknown.");
  brailleClearCell(); drawBraillePad(lbl);
}

void brailleUndoChar() {
  char removed = 0;
  if (histLen>0) removed = history[histLen-1].value;
  if (histLen>0) histLen--; capitalNext=false; brailleClearCell();
  drawBraillePad(currentState==STATE_WRITE_TITLE?"Title:":"Note:");
  speakUndoResult(removed);
}

void brailleUndoSentence() {
  String removed = "";
  if (histLen>0) histLen--;
  while (histLen>0 && history[histLen-1].value!='.' && history[histLen-1].value!='!' && history[histLen-1].value!='?') {
    removed = String(history[histLen-1].value) + removed;
    histLen--;
  }
  capitalNext=false; numberMode=false; brailleClearCell();
  drawBraillePad(currentState==STATE_WRITE_TITLE?"Title:":"Note:");
  String msg = "Removed sentence. Now reads ";
  String buf = brailleBufferString();
  msg += buf.length() ? buf : "empty";
  samSayChunked(msg);
}

void brailleReadBuffer() {
  if (histLen==0) { speakPhrase("empty"); return; }
  String buf=""; for (int i=0;i<histLen;i++) buf+=history[i].value;
  samSayChunkedInterruptible(buf);
}

void brailleConfirmTitle() {
  pendingTitle=""; for (int i=0;i<histLen;i++) pendingTitle+=history[i].value;
  pendingTitle.trim(); if (pendingTitle.length()==0) pendingTitle="untitled";
  speakPhrase("title_saved");
  histLen=0; capitalNext=false; numberMode=false;
  brailleClearCell(); selectPrev=false; undoPrev=false; undoPending=false; backWarnSpoken=false;
  currentState=STATE_NEW_NOTE; drawBraillePad("Note:");
}

void brailleSaveNote() {
  String body=""; for (int i=0;i<histLen;i++) body+=history[i].value; body.trim();
  if (pendingTitle.length()==0 && body.length()==0) {
    speakPhrase("nothing_typed"); drawToast("Nothing typed!");
    currentState=STATE_MAIN_MENU; menuIndex=0; drawMainMenu(); return;
  }
  String fullNote=pendingTitle+"\n"+body;
  int slot=-1;
  for (int i=0;i<MAX_NOTES;i++) { if (!LittleFS.exists(noteTxtPath(i))) { slot=i; break; } }
  if (slot==-1) {
    speakPhrase("storage_full");
    samSayString("Delete an old note from read notes, then save again.");
    return;
  }
  File f=LittleFS.open(noteTxtPath(slot), FILE_WRITE); if (f) { f.print(fullNote); f.close(); }
  loadNotes(); speakPhrase("note_saved"); drawToast("Saved!"); clearDraft(); queueNoteAudio(slot);
  histLen=0; pendingTitle=""; capitalNext=false; numberMode=false;
  brailleClearCell(); backWarnSpoken=false;
  currentState=STATE_MAIN_MENU; menuIndex=0; drawMainMenu();
}

void brailleClearBuffer() {
  histLen=0; capitalNext=false; numberMode=false; brailleClearCell();
  drawBraillePad(currentState==STATE_WRITE_TITLE?"Title:":"Note:");
  speakPhrase("cleared");
}

void handleBraillePad() {
  unsigned long now=millis();
  const char* lbl=(currentState==STATE_WRITE_TITLE)?"Title:":"Note:";
  if (now-lastAutosave>AUTOSAVE_MS) { lastAutosave=now; saveDraft(); }
  bool anyNewDot=false;
  for (int i=0;i<6;i++) {
    bool r=!digitalRead(DOT_PINS[i]);
    if (r!=dotPrev[i] && (now-dotLastMs[i])>DOT_DEBOUNCE_MS) {
      dotLastMs[i]=now; dotPrev[i]=r;
      if (r) { dots[i]=true; cellBuilding=true; anyNewDot=true; }
    }
  }
  if (anyNewDot) drawBraillePad(lbl);
  bool selNow=buttonPressed(BTN_SELECT);
  if (selNow&&!selectPrev&&(now-lastDebounce)>DEBOUNCE_MS) { lastDebounce=now; brailleCommit(); }
  selectPrev=selNow;
  bool undoNow=buttonPressed(BTN_BACK);
  if (undoNow&&!undoPrev) { backHeldSince=now; backLongFired=false; backWarnSpoken=false; }
  if (!undoNow&&undoPrev&&!backLongFired) {
    if (undoPending&&(now-undoPendingTime)<UNDO_DBLCLICK_MS) {
      undoPending=false;
      // Double tap on empty buffer = escape to main menu
      if (histLen==0) {
        Serial.println(F("[BRAILLE] Empty buffer double-tap — escaping to menu"));
        histLen=0; pendingTitle=""; capitalNext=false; numberMode=false;
        brailleClearCell(); undoPrev=false; undoPending=false;
        currentState=STATE_MAIN_MENU; menuIndex=0; drawMainMenu();
        speakPhrase("new_note");
        return;
      }
      brailleUndoSentence();
    }
    else { undoPending=true; undoPendingTime=now; }
  }
  undoPrev=undoNow;
  if (undoPending&&(now-undoPendingTime)>=UNDO_DBLCLICK_MS) { undoPending=false; brailleUndoChar(); }
  if (buttonPressed(BTN_REREAD)&&(now-lastDebounce)>DEBOUNCE_MS) { lastDebounce=now; brailleReadBuffer(); }
  if (buttonPressed(BTN_AISAVE)&&(now-lastDebounce)>DEBOUNCE_MS) {
    lastDebounce=now;
    if (currentState==STATE_WRITE_TITLE) brailleConfirmTitle(); else brailleSaveNote();
  }
  if (buttonPressed(BTN_DELETE)&&(now-lastDebounce)>DEBOUNCE_MS) { lastDebounce=now; brailleClearBuffer(); }
}

