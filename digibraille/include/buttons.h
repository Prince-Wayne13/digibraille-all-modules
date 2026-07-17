#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include "globals.h"
#include "sam_tts.h"
#include "mp3_player.h"
#include "offline_grammar.h"
#include "storage.h"
#include "display.h"

// draw functions declared in display.h — included via globals chain
// isValidMp3 / playMP3 declared in mp3_player.h (included above) — no
// redeclaration here; a stale duplicate previously drifted out of sync
// with mp3_player.h's real signature and would no longer compile.
//
// brailleResetBuffer() / brailleResetGestureState() / brailleClearCell()
// are defined in braille.h. This file doesn't #include braille.h directly
// — it relies on the .ino's include order putting braille.h before this
// file, same as the pre-existing (unchanged) brailleClearCell() call
// further down. If buttons.h is ever compiled/included on its own or before
// braille.h, these calls won't resolve — worth adding
// #include "braille.h" here explicitly if that ordering isn't guaranteed.

// ─── SPEAK A NOTE TITLE, WITH DOWN-TO-SKIP CHAINING ───────────
// Replaces the "resolve title mp3 or fall back to SAM + queue" block that
// used to be copy-pasted at every call site (handleMainMenu, handleReadNotes
// x2, handleViewNote, handleDeleteConfirm x2).
//
// Interrupt-aware: playMP3() blocks loop() while a clip plays, so without
// this, holding DOWN through a title cut the clip but nothing advanced —
// control returned to a caller that had already finished its "if" block.
// Here, if the clip was cut specifically by DOWN, we advance the index
// ourselves and immediately speak the next title, chaining for as long as
// DOWN keeps interrupting. Any other interrupt (BACK, SELECT, etc.) just
// stops, exactly as before, and lets the normal handler loop pick it up.
static void speakNoteTitle(int index) {
  while (true) {
    String title = getNoteTitle(index);
    // A title "Grace" reads from recorded assets via its prebuilt chain
    // (whole-word "grace.mp3" if present, else letter-by-letter) — no
    // reliance on a never-generated note{N}_title.mp3. Build it on the fly
    // if a boot migration somehow hasn't produced one yet, then play it.
    if (noteTitleChainExists(index) || buildNoteTitleChain(index, title)) {
      playNoteTitleChain(index);
    } else {
      // Last-resort fallback if storage is unwritable: live asset/SAM spell.
      speakText(title, "");
      lastInterruptPin = -1; // speakText's SAM/asset path isn't interrupt-tracked like playMP3
    }

    if (lastInterruptPin != BTN_DOWN) break; // finished naturally, or cut by something else
    if (noteCount <= 0) break;

    index = (index + 1) % noteCount;
    selectedNote = index;
    drawReadNotes();
    logTestEvent(4, "navigation-down-chained", String("selected=") + String(selectedNote));
  }
}

void enterMainMenu(bool speakHighlighted) {
  currentState = STATE_MAIN_MENU;
  drawMainMenu();
  if (sdWarningPending) {
    logTs("SD", "Warning user at main menu; notes are using LittleFS fallback");
    drawSdWarning();
    speakPhrase("sd_missing");
    drawMainMenu();
  }
  if (speakHighlighted) {
    if (menuIndex == 0) speakPhrase("new_note");
    else if (menuIndex == 1) speakPhrase("read_notes");
    else speakPhrase("language");
  }
}

void handleLangSelect() {
  unsigned long now = millis();
  if (now - lastDebounce < DEBOUNCE_MS) return;

  if (buttonPressed(BTN_DOWN)) {
    lastDebounce = now;
    langChoiceIndex = (langChoiceIndex + 1) % 2;
    Serial.print(F("[LANG] Highlight "));
    Serial.println(langChoiceIndex == 0 ? "English" : "Chichewa");
    drawLangSelect();
    speakPhrase(langChoiceIndex == 0 ? "english" : "chichewa");
    return;
  }

  if (buttonPressed(BTN_SELECT)) {
    lastDebounce = now;
    langEnglish = (langChoiceIndex == 0);
    saveConfig();
    Serial.print(F("[LANG] Saved "));
    Serial.println(langEnglish ? "English" : "Chichewa");
    speakPhrase(langEnglish ? "english" : "chichewa");
    menuIndex = 0;
    enterMainMenu();
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
      brailleResetBuffer();
      pendingTitle = "";
      brailleResetGestureState();
      lastAutosave=millis();
      lastDebounce=millis();
      if (restoreDraft()) {
        brailleClearCell();
        drawBraillePad(pendingTitle.length() ? "Note:" : "Title:");
        speakText("Draft restored.", "");
        if (histLen > 0) speakText(brailleBufferString(), "");
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
      if (noteCount==0) speakPhrase("no_notes");
      else speakNoteTitle(0);
    } else {
      langChoiceIndex = langEnglish ? 0 : 1;
      currentState=STATE_LANG_SELECT; drawLangSelect();
      lastDebounce=millis();
      speakPhrase("choose_language");
      speakPhrase(langChoiceIndex == 0 ? "english" : "chichewa");
      speakPhrase("lang_instructions");
    }
  }
}

// Plays note `index`'s body via its pre-built audio chain (see
// buildNoteAudioChain/playNoteAudioChain in mp3_player.cpp). If no chain
// exists yet (an older note saved before this feature existed), falls
// back to speaking just the title so the user isn't met with silence.
//
// Interrupt-aware for BOTH BACK and DOWN now:
//  - BACK cuts the chain short and this returns immediately WITHOUT
//    speaking "end_of_note" — caller checks lastInterruptPin to navigate
//    back a screen right away.
//  - DOWN cuts the chain short, scrolls the on-screen text one page, and
//    resumes reading, rather than the chain playing to completion no
//    matter what the user presses.
//
// BUG FIX: previously mp3_player.cpp's chain player only stopped for
// BACK, so a DOWN press mid-note silenced one word's clip but the chain
// kept right on playing every remaining word — DOWN appeared to do
// nothing because handleViewNote() (which scrolls the screen) never got
// a turn until the whole note finished, often a minute or more later, by
// which point the button press was long over. Now that the chain player
// stops for any control button (see playChainAtPath in mp3_player.cpp),
// DOWN here actually scrolls the screen and keeps reading, instead of
// being silently swallowed.
static void speakNoteBody(int index) {
  speakPhrase("reading_note");
  while (true) {
    if (noteAudioChainExists(index)) {
      playNoteAudioChain(index);
    } else {
      Serial.println(F("[NOTE] No chain built yet — reading title only"));
      speakText(getNoteTitle(index), "");
      lastInterruptPin = -1; // speakText's SAM/asset path isn't interrupt-tracked like playMP3
      break;
    }
    if (lastInterruptPin == BTN_DOWN) {
      noteScrollOffset++;
      drawViewNote(index);
      logTestEvent(4, "view-scroll-down", String("offset=") + String(noteScrollOffset));
      continue; // keep reading — chain playback doesn't track position, so
                // this re-reads from the start; acceptable tradeoff vs. the
                // prior behavior of DOWN doing nothing at all
    }
    break;
  }
  if (lastInterruptPin != BTN_BACK) speakPhrase("end_of_note");
}

void handleReadNotes() {
  if (millis()-lastDebounce<DEBOUNCE_MS) return;
  if (buttonPressed(BTN_DOWN)) {
    unsigned long actionStart = millis();
    lastDebounce=millis();
    if (noteCount>0) {
      selectedNote=(selectedNote+1)%noteCount; drawReadNotes();
      logTestEvent(4, "navigation-down", String("selected=") + String(selectedNote));
      speakNoteTitle(selectedNote);
      logTestEvent(4, "navigation-feedback-complete", String("duration_ms=") + String(millis() - actionStart));
    }
  } else if (buttonPressed(BTN_SELECT)) {
    lastDebounce=millis();
    if (noteCount>0) {
      noteScrollOffset=0; currentState=STATE_VIEW_NOTE; drawViewNote(selectedNote);
      speakNoteBody(selectedNote);
    }
  } else if (buttonPressed(BTN_REREAD)) {
    lastDebounce=millis();
    if (noteCount>0) speakNoteTitle(selectedNote);
  } else if (buttonPressed(BTN_DELETE)) {
    lastDebounce=millis();
    if (noteCount>0) { currentState=STATE_DELETE_CONFIRM; drawDeleteConfirm(); speakText("delete "+getNoteTitle(selectedNote)+"?", ""); }
  } else if (buttonPressed(BTN_BACK)) {
    lastDebounce=millis(); menuIndex=0; enterMainMenu();
  }
}

void handleViewNote() {
  if (millis()-lastDebounce<DEBOUNCE_MS) return;
  if (buttonPressed(BTN_DOWN)) {
    unsigned long actionStart = millis();
    lastDebounce=millis(); noteScrollOffset++; drawViewNote(selectedNote);
    logTestEvent(4, "view-scroll-down", String("offset=") + String(noteScrollOffset));
    // This branch only fires once speakNoteBody() has returned (finished,
    // or was cut by something other than DOWN) — while a chain is
    // actively playing, DOWN is now handled inline inside speakNoteBody()
    // itself (see buttons.h), which scrolls AND resumes reading. This is
    // the "note already finished, DOWN just pages the visible text"
    // silent-scroll case.
    logTestEvent(4, "view-scroll-feedback-complete", String("duration_ms=") + String(millis() - actionStart));
  } else if (buttonPressed(BTN_REREAD)) {
    lastDebounce=millis();
    speakNoteBody(selectedNote);
    if (lastInterruptPin == BTN_BACK) {
      // BACK cut the re-read short — honor BACK's real function here too
      // (go back to the note list), not just silence the audio.
      noteScrollOffset=0; currentState=STATE_READ_NOTES; drawReadNotes();
      speakNoteTitle(selectedNote);
    }
  } else if (buttonPressed(BTN_AISAVE)) {
    lastDebounce=millis();
    if (aiBusy) { speakPhrase("loading"); return; }
    aiBusy=true; speakPhrase("ai"); speakPhrase("loading"); drawAILoading();
    String improved=improveNoteOffline(getNoteBody(selectedNote)); aiBusy=false;
    if (improved.length()>0) {
      aiImprovedNote=improved; currentState=STATE_AI_PREVIEW; drawAIPreview(improved);
      speakPhrase("ai_ready"); speakText(improved, "");
    } else { speakPhrase("ai_failed"); speakText("Keep writing or try again.", ""); drawToast("Grammar failed!"); drawViewNote(selectedNote); }
  } else if (buttonPressed(BTN_DELETE)) {
    lastDebounce=millis(); currentState=STATE_DELETE_CONFIRM; drawDeleteConfirm();
    speakText("delete "+getNoteTitle(selectedNote)+"?", "");
  } else if (buttonPressed(BTN_BACK)) {
    lastDebounce=millis(); noteScrollOffset=0; currentState=STATE_READ_NOTES; drawReadNotes();
    speakNoteTitle(selectedNote);
  }
}

void handleAIPreview() {
  if (millis()-lastDebounce<DEBOUNCE_MS) return;
  if (buttonPressed(BTN_REREAD)) { lastDebounce=millis(); speakText(aiImprovedNote, ""); }
  else if (buttonPressed(BTN_SELECT)) {
    lastDebounce=millis();
    String fullNote=getNoteTitle(selectedNote)+"\n"+aiImprovedNote;
    File f=openNoteFile(selectedNote,FILE_WRITE); if(f){f.print(fullNote);f.close();}
    noteList[selectedNote]=fullNote; loadNotes();
    removeNoteAudioFiles(selectedNote);
    buildNoteAudioChain(selectedNote, aiImprovedNote);
    speakPhrase("note_saved"); drawToast("Updated!");
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
    for (int i=selectedNote;i<noteCount-1;i++) noteList[i]=noteList[i+1];
    noteCount--;
    for (int i=0;i<MAX_NOTES;i++) if (noteExists(i)) removeNoteFile(i);
    for (int i=0;i<MAX_NOTES;i++) removeNoteAudioFiles(i);
    for (int i=0;i<noteCount;i++) { File f=openNoteFile(i,FILE_WRITE); if(f){f.print(noteList[i]);f.close();} }
    if (selectedNote>=noteCount&&selectedNote>0) selectedNote--;
    speakPhrase("deleted"); drawToast("Deleted!"); currentState=STATE_READ_NOTES; drawReadNotes();
    if (noteCount==0) speakPhrase("no_notes");
    else speakNoteTitle(selectedNote);
  } else if (buttonPressed(BTN_BACK)) {
    lastDebounce=millis(); speakPhrase("cancelled");
    currentState=STATE_READ_NOTES; drawReadNotes();
    if (noteCount>0) speakNoteTitle(selectedNote);
  }
}