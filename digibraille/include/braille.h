#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "globals.h"
#include "sam_tts.h"
#include "mp3_player.h"
#include "storage.h"

// BRAILLE_CAPITAL_IND / BRAILLE_NUMBER_IND now defined in globals.h (this
// file already includes it above) — moved there so display.h can use them
// too without a circular #include between display.h and this file.

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

// Forward declarations for display.h functions used below. Previously this
// relied on display.h being #included before braille.h in the .ino's
// include order — that assumption broke once display.h needed a symbol
// from this file too (decodeBraille) and pulled braille.h in from partway
// through its own body. Declaring these explicitly here means this file's
// correctness no longer depends on include order at all.
void drawBraillePad(const char* modeLabel);
void drawToast(const char* msg);

// handleBackButton is defined near the bottom of this file (it's the real
// implementation of BACK's tap/double-tap/long-press logic). Forward
// declared here so speakInterruptibleAndProcessBack() below — used by
// functions defined earlier in the file — can call it.
static void handleBackButton(unsigned long now, bool undoNow);

// ─── PAD LOGIC ───────────────────────────────────────────────
void brailleClearCell() { for (int i=0;i<6;i++) dots[i]=false; cellBuilding=false; }

// Speaks msg via speakText() — same as calling speakText() directly —
// EXCEPT: speakText() may play several clips back-to-back (spelling a
// word letter by letter, several words in a sentence). Each clip can be
// cut short by a BACK press mid-playback, but BACK's actual tap/
// double-tap/long-press logic (handleBackButton) only runs from
// handleBraillePad(), which never gets a turn while this blocking
// playback is happening. Without this wrapper, BACK stops the current
// clip and the chain just rolls on to the next word/letter — the
// button's real function (undo, clear, exit+save) never actually fires.
//
// This closes that gap: right after speakText() returns, it checks
// BACK's live physical state and — if BACK caused the interruption or is
// still held — feeds that straight into handleBackButton() so the real
// behavior fires immediately, not on whatever the next loop() pass
// happens to see.
static void speakInterruptibleAndProcessBack(const String& msg) {
  speakText(msg, "");
  unsigned long now = millis();
  bool backNow = buttonPressed(BTN_BACK);
  if (lastInterruptPin == BTN_BACK || backNow) {
    handleBackButton(now, backNow);
  }
}

// ─── SHARED RESET HELPERS ──────────────────────────────────────
// Two distinct, independently-needed resets, previously inlined by hand at
// every place that enters or leaves the Braille pad (new note, title
// confirm, save, clear, exit) — with small, easy-to-miss variations
// between the copies. Kept as two separate functions rather than one
// combined reset because call sites genuinely need different combinations:
// e.g. brailleClearBuffer() wants only the typed-buffer reset and must NOT
// touch pendingTitle or gesture state; brailleSaveNote() wants buffer reset
// plus pendingTitle cleared, but gesture state is handled separately there.

// Clears what the user has typed in the current cell/word/note buffer.
// Does NOT touch pendingTitle — callers that are also done with the title
// (leaving the pad entirely, or starting a fresh note) clear that themselves.
void brailleResetBuffer() {
  histLen = 0;
  capitalNext = false;
  numberMode = false;
  brailleClearCell();
}

// Clears in-progress button-gesture tracking (BACK long-press / double-tap
// timers, SELECT edge state). Needed whenever we're about to change
// screens or states, so a gesture that was mid-flight on the old screen
// doesn't fire unexpectedly on the new one.
void brailleResetGestureState() {
  selectPrev = false;
  undoPrev = false;
  undoPending = false;
  backLongFired = false;
  backWarnSpoken = false;
}

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
  speakInterruptibleAndProcessBack(msg);
}

void brailleCommit() {
  unsigned long actionStart = millis();
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
      if (c>='A'&&c<='Z') { speakPhrase("capital"); speakCharacter((char)(c+32)); }
      else speakCharacter(c);
    }
  } else speakText("unknown", "");
  brailleClearCell(); drawBraillePad(lbl);
  logTestEvent(2, "braille-commit-complete", String("duration_ms=") + String(millis() - actionStart));
  logTestEvent(3, "typing-buffer", brailleBufferString());
}

void brailleUndoChar() {
  char removed = 0;
  if (histLen>0) removed = history[histLen-1].value;
  if (histLen>0) histLen--; capitalNext=false; brailleClearCell();
  drawBraillePad(currentState==STATE_WRITE_TITLE?"Title:":"Note:");
  speakUndoResult(removed);
}

// Exits the pad WITHOUT saving. No longer reachable from BACK (long-press
// now saves a draft instead — see brailleExitAndSaveDraft below) but kept
// in case another code path still wants a hard discard.
void exitBraillePadWithoutSaving() {
  brailleResetBuffer();
  pendingTitle = "";
  brailleResetGestureState();
  clearDraft();
  menuIndex = 0;
  enterMainMenu();
}

// BACK long-press: exit the pad AND save current progress as a draft, so a
// long-press means "step away for now," not "throw this away." saveDraft()
// already exists (used by the periodic autosave in handleBraillePad) —
// this just calls it explicitly on the way out instead of discarding.
void brailleExitAndSaveDraft() {
  saveDraft();
  brailleResetBuffer();
  pendingTitle = "";
  brailleResetGestureState();
  menuIndex = 0;
  speakText("Draft saved.", "");
  enterMainMenu();
}

void brailleUndoWord() {
  while (histLen>0 && history[histLen-1].value==' ') histLen--;
  while (histLen>0 && history[histLen-1].value!=' ') histLen--;
  capitalNext=false; numberMode=false; brailleClearCell();
  drawBraillePad(currentState==STATE_WRITE_TITLE?"Title:":"Note:");
  String msg = "Removed word. Now reads ";
  String buf = brailleBufferString();
  msg += buf.length() ? buf : "empty";
  speakInterruptibleAndProcessBack(msg);
}

// brailleReadBuffer() removed — "read everything typed so far" while
// actively typing is gone. REREAD itself is NOT being retired; it's still
// used on other screens (re-read note title/body/AI result — see
// buttons.h). It's just unbound during live Braille-pad typing now.

void brailleConfirmTitle() {
  pendingTitle=""; for (int i=0;i<histLen;i++) pendingTitle+=history[i].value;
  pendingTitle.trim(); if (pendingTitle.length()==0) pendingTitle="untitled";
  speakPhrase("title_saved");
  brailleResetBuffer();
  brailleResetGestureState();
  currentState=STATE_NEW_NOTE; drawBraillePad("Note:");
}

void brailleSaveNote() {
  String body=""; for (int i=0;i<histLen;i++) body+=history[i].value; body.trim();
  if (pendingTitle.length()==0 && body.length()==0) {
    speakPhrase("nothing_typed"); drawToast("Nothing typed!");
    menuIndex=0; enterMainMenu(); return;
  }
  String fullNote=pendingTitle+"\n"+body;
  int slot=-1;
  for (int i=0;i<MAX_NOTES;i++) { if (!noteExists(i)) { slot=i; break; } }
  if (slot==-1) {
    speakPhrase("storage_full");
    speakText("Delete an old note from read notes, then save again.", "");
    return;
  }
  File f=openNoteFile(slot, FILE_WRITE); if (f) { f.print(fullNote); f.close(); }
  loadNotes(); speakPhrase("note_saved"); drawToast("Saved!"); clearDraft();
  buildNoteAudioChain(slot, body);
  buildNoteTitleChain(slot, pendingTitle);
  brailleResetBuffer();
  pendingTitle = "";
  backWarnSpoken = false;
  menuIndex=0; enterMainMenu();
}

void brailleClearBuffer() {
  brailleResetBuffer();
  drawBraillePad(currentState==STATE_WRITE_TITLE?"Title:":"Note:");
  speakPhrase("cleared");
}

// ─── BACK BUTTON: single tap / spaced double-tap / long-press ────────────
// Three distinct outcomes:
//   single tap                         -> delete last character
//   two taps, 150ms-600ms apart        -> clear current word buffer
//   held for BACK_LONG_PRESS_MS        -> exit pad, save as draft
//
// A tap under UNDO_DBLTAP_MIN_MS after the first is treated as bounce, not
// a deliberate second press, and resolves as a plain single-tap delete.
// A tap over UNDO_DBLTAP_MAX_MS after the first means the first tap has
// already resolved on its own; the late tap starts its own fresh timer.
//
// undoPending / undoPendingTime (declared in globals.h) hold the "first tap
// seen, waiting to see what happens next" state between calls.
static void handleBackButton(unsigned long now, bool undoNow) {
  // ── long-press detection (unchanged shape, new action) ──
  if (undoNow && !undoPrev) { backHeldSince = now; backLongFired = false; backWarnSpoken = false; }
  if (undoNow && !backLongFired && (now - backHeldSince) >= BACK_LONG_PRESS_MS) {
    backLongFired = true;
    lastDebounce = now;
    undoPending = false; // long-press wins outright; discard any pending tap-timer
    brailleExitAndSaveDraft();
    return;
  }

  // ── release handling: this is where tap counting happens ──
  if (!undoNow && undoPrev && !backLongFired) {
    if (undoPending) {
      unsigned long gap = now - undoPendingTime;
      if (gap >= UNDO_DBLTAP_MIN_MS && gap <= UNDO_DBLTAP_MAX_MS) {
        // Deliberate, properly-spaced second tap -> clear word buffer.
        undoPending = false;
        lastDebounce = now;
        if (histLen == 0) { speakPhrase("empty"); return; }
        brailleUndoWord();
        return;
      }
      if (gap < UNDO_DBLTAP_MIN_MS) {
        // Too fast to be deliberate — likely bounce off the same physical
        // press. Fold it back into "still waiting for a real second tap"
        // rather than resolving anything yet.
        return;
      }
      // gap > UNDO_DBLTAP_MAX_MS: the pending tap is stale. Let it resolve
      // as its own single-tap delete (handled by the timeout check below,
      // which will have already fired by now), and start fresh with this
      // release as a brand new potential first tap.
    }
    undoPending = true;
    undoPendingTime = now;
  }
  undoPrev = undoNow;

  // ── timeout: a pending single tap that never got a valid second tap ──
  if (undoPending && (now - undoPendingTime) > UNDO_DBLTAP_MAX_MS) {
    undoPending = false;
    lastDebounce = now;
    brailleUndoChar();
  }
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
      if (r) {
        dots[i]=true; cellBuilding=true; anyNewDot=true;
        logTestEvent(2, "dot-press-detected", String("DOT") + String(i + 1));
      }
    }
  }
  if (anyNewDot) {
    drawBraillePad(lbl);
    logTestEvent(2, "dot-action-complete", String("buffer_len=") + String(histLen));
  }
  bool selNow=buttonPressed(BTN_SELECT);
  if (selNow&&!selectPrev&&(now-lastDebounce)>DEBOUNCE_MS) { lastDebounce=now; brailleCommit(); }
  selectPrev=selNow;

  bool undoNow=buttonPressed(BTN_BACK);
  handleBackButton(now, undoNow);

  // Typing-buffer readback (REREAD -> "read everything I've typed so far")
  // removed. REREAD still exists for other screens (re-read note title,
  // note body, AI result — see buttons.h); it's just no longer bound to
  // anything while actively typing on the Braille pad.
  if (buttonPressed(BTN_AISAVE)&&(now-lastDebounce)>DEBOUNCE_MS) {
    lastDebounce=now;
    if (currentState==STATE_WRITE_TITLE) brailleConfirmTitle(); else brailleSaveNote();
  }
  if (buttonPressed(BTN_DELETE)&&(now-lastDebounce)>DEBOUNCE_MS) { lastDebounce=now; brailleClearBuffer(); }
}