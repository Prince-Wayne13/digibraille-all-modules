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
    bool played = false;
    if (noteTitleChainExists(index) || buildNoteTitleChain(index, title)) {
      played = playNoteTitleChain(index);
      if (!played) {
        // Chain existed but produced no audio at all — most likely a
        // stale/empty/corrupt chain left over from before the asset
        // library or chain format changed. Force one rebuild and retry
        // instead of silently doing nothing forever.
        Serial.println(F("[TITLE] Chain produced no audio - rebuilding once"));
        buildNoteTitleChain(index, title);
        played = playNoteTitleChain(index);
      }
    }
    if (!played) {
      // Last-resort fallback if storage is unwritable or rebuild still
      // fails: live asset/SAM spell.
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
    // Index and bank are both built per-language (/words/en vs /words/ch).
    // Bank is tried FIRST and, if it loads, the slow full-folder index scan
    // is skipped entirely — see log session 14: with a large word library
    // (thousands of clips), that scan measured a clearly quadratic growth
    // in cost (each successive 1000 files took 3x+ longer than the last),
    // projecting to roughly four hours for ~9,629 files. Since
    // resolveAudioAsset() checks the bank before ever touching the index,
    // running the slow scan when the bank already covers this language was
    // pure wasted time — and doing it HERE, mid-use while the UI is
    // blocked waiting on this call, would be far worse than at boot.
    bool bankLoaded = loadWordBank();
    if (!bankLoaded) {
      Serial.println(F("[INDEX] No word bank for this language - rebuilding per-file index"));
      buildWordAssetIndex();
    } else {
      Serial.println(F("[INDEX] Word bank loaded - skipping slow per-file directory scan"));
    }
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

// Callback passed to playNoteAudioChain() (see mp3_player.h/.cpp session
// 18 change) — fires once per physical DOWN press WHILE the note body is
// being read aloud, WITHOUT stopping playback. Reuses the exact same
// scroll logic handleViewNote()'s own DOWN handler already had (advance
// noteScrollOffset, redraw) — this just makes it reachable during
// playback too, which it never was before (loop() was blocked inside the
// audio call the whole time DOWN normally would have been handled).
// Must be a plain function (not a lambda with captures) since it's
// invoked through a C-style function pointer, matching how the rest of
// this codebase already passes callbacks across the mp3_player.cpp
// boundary — relies on selectedNote/noteScrollOffset being globals
// (globals.h), same as handleViewNote() itself does.
static void onDownDuringNoteBody() {
  noteScrollOffset++;
  drawViewNote(selectedNote);
  logTestEvent(4, "view-scroll-down-during-playback", String("offset=") + String(noteScrollOffset));
}

// Plays note `index`'s body via its pre-built audio chain (see
// buildNoteAudioChain/playNoteAudioChain in mp3_player.cpp). If no chain
// exists yet (an older note saved before this feature existed), falls
// back to speaking just the title so the user isn't met with silence.
//
// BACK-aware: if BACK cuts the chain short, this returns immediately
// afterward WITHOUT speaking "end_of_note" — the caller checks
// lastInterruptPin itself to decide whether to also navigate back a
// screen right away, same as any other BACK-interrupted action.
//
// DOWN-aware (session 18): passes onDownDuringNoteBody() to
// playNoteAudioChain() so DOWN scrolls the screen in sync while the body
// keeps playing, instead of doing nothing (previously DOWN was one of the
// buttons that could interrupt a clip, but interrupting mid-word and then
// letting the chain continue anyway meant DOWN visibly did nothing useful
// during body playback — this replaces that dead behavior with real
// scrolling). Only body playback gets this — title playback (via
// speakNoteTitle() above) is unchanged and still uses DOWN to skip to the
// next note's title, since that's a different, already-useful behavior.
static void speakNoteBody(int index) {
  speakPhrase("reading_note");
  if (noteAudioChainExists(index)) {
    playNoteAudioChain(index, onDownDuringNoteBody);
  } else {
    Serial.println(F("[NOTE] No chain built yet — reading title only"));
    speakText(getNoteTitle(index), "");
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
    // The chain already reads the whole note aloud once on entry, so DOWN
    // here only scrolls the on-screen text — no per-line audio needed or
    // spoken twice.
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