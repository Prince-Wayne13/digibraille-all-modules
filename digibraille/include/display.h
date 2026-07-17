#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Fonts/FreeSerif9pt7b.h>
#include "globals.h"

extern Adafruit_SH1106G display;

// Forward declaration only — NOT a full #include "braille.h" here.
// braille.h calls drawBraillePad()/drawToast() (defined below in this
// file), so a two-way #include creates a circular dependency: whichever
// header gets pulled in first has its functions used by the other before
// they're declared. Declaring just the one symbol this file actually
// needs (the real decodeBraille(), defined in braille.h) breaks that
// cycle. The live preview in drawBraillePad() below calls this exact
// function on local copies of capitalNext/numberMode so the preview
// can't mutate real typing state.
char decodeBraille(uint8_t code, bool& capNext, bool& numMode);

void drawStatusBar() {
}

void drawBraillePad(const char* modeLabel) {
  display.clearDisplay();
  display.setFont(); display.setTextSize(1); display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0); display.print(modeLabel);
  if (capitalNext) { display.setCursor(55, 0); display.print("CAP"); }
  if (numberMode)  { display.setCursor(80, 0); display.print("NUM"); }
  drawStatusBar();
  int dotCol[6] = { 7,  7,  7, 18, 18, 18};
  int dotRow[6] = { 4, 12, 20,  4, 12, 20};
  for (int i = 0; i < 6; i++) {
    if (dots[i]) display.fillCircle(dotCol[i], dotRow[i], 3, SH110X_WHITE);
    else         display.drawCircle(dotCol[i], dotRow[i], 3, SH110X_WHITE);
  }
  uint8_t liveCode = 0;
  for (int i = 0; i < 6; i++) if (dots[i]) liveCode |= (1<<i);
  bool dC = capitalNext, dN = numberMode; // local copies — preview must not mutate real state
  char liveChar = (liveCode == 0) ? ' ' : decodeBraille(liveCode, dC, dN);
  display.setTextSize(2); display.setCursor(32, 6);
  if (cellBuilding) {
    if      (liveCode == BRAILLE_CAPITAL_IND) display.print("^");
    else if (liveCode == BRAILLE_NUMBER_IND)  display.print("#");
    else display.print(liveChar != '?' ? String(liveChar) : "?");
  } else {
    display.setTextSize(1);
    display.setCursor(32, 10); display.print("press");
    display.setCursor(32, 19); display.print("dots..");
  }
  display.setTextSize(1); display.setCursor(100, 9);
  display.print(histLen); display.print("c");
  display.drawLine(0, 25, 127, 25, SH110X_WHITE);
  String full = "";
  for (int i = 0; i < histLen; i++) full += history[i].value;
  String bufLines[6]; int numLines = 0; String tmp = full;
  while (tmp.length() > 0 && numLines < 6) {
    int len = min((int)tmp.length(), 21);
    if (len == 21 && tmp.length() > 21) { int sp = tmp.lastIndexOf(' ', 20); if (sp > 4) len = sp+1; }
    bufLines[numLines++] = tmp.substring(0, len); tmp = tmp.substring(len);
  }
  int startLine = max(0, numLines-3); int screenY[] = {28, 39, 50};
  for (int i = 0; i < 3; i++) {
    int li = startLine+i; if (li >= numLines) break;
    String row = bufLines[li];
    display.setCursor(0, screenY[i]); display.print(row);
    if (li == numLines-1) { int cx = row.length()*6; display.fillRect(cx, screenY[i]+7, 5, 2, SH110X_WHITE); }
  }
  display.display();
}

void drawStartup() {
  display.clearDisplay(); display.setTextColor(SH110X_WHITE);
  display.setFont(&FreeSerif9pt7b); display.setCursor(12, 37);
  display.print("WAYNE INC."); display.display();
}

void drawLangSelect() {
  display.clearDisplay(); display.setFont(&FreeSerif9pt7b);
  display.setCursor(10, 14); display.setTextColor(SH110X_WHITE); display.print("Language");
  display.setFont(); display.drawLine(3, 18, 128, 18, SH110X_WHITE);
  const char* options[] = {"English", "Chichewa"};
  for (int i = 0; i < 2; i++) {
    int y = 28 + (i * 14);
    if (i == langChoiceIndex) {
      display.fillRect(6, y - 2, 116, 11, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK);
    } else {
      display.setTextColor(SH110X_WHITE);
    }
    display.setCursor(12, y); display.print(options[i]);
  }
  display.setTextColor(SH110X_WHITE);
  drawStatusBar(); display.display();
}

void drawMainMenu() {
  display.clearDisplay(); display.setTextColor(SH110X_WHITE);
  display.setFont(&FreeSerif9pt7b); display.setCursor(10, 14); display.print("DigiBraille");
  display.setFont(); display.drawLine(3, 18, 128, 18, SH110X_WHITE);
  display.drawRect(3, 23, 121, 37, SH110X_WHITE);
  int textY[] = {26, 38, 49};
  for (int i = 0; i < MENU_COUNT; i++) {
    if (i == menuIndex) { display.fillRect(4, textY[i]-1, 119, 10, SH110X_WHITE); display.setTextColor(SH110X_BLACK); }
    else display.setTextColor(SH110X_WHITE);
    display.setCursor(9, textY[i]); display.print(menuLabel(i));
  }
  drawStatusBar(); display.display();
}

void drawToast(const char* msg) {
  display.clearDisplay(); display.setFont(); display.setTextColor(SH110X_WHITE);
  display.drawRect(4, 20, 120, 24, SH110X_WHITE);
  display.setCursor(10, 28); display.print(msg);
  display.display(); delay(1400);
}

void drawSdWarning() {
  display.clearDisplay(); display.setFont(); display.setTextColor(SH110X_WHITE);
  display.drawRect(2, 12, 124, 40, SH110X_WHITE);
  display.setCursor(8, 22); display.print("SD card not found");
  display.setCursor(8, 34); display.print("Saving temporarily");
  display.display();
}

void drawReadNotes() {
  display.clearDisplay(); display.setFont(&FreeSerif9pt7b);
  display.setCursor(10, 14); display.setTextColor(SH110X_WHITE);
  display.print(langEnglish ? "Read Notes" : "Werenga");
  display.setFont(); display.drawLine(3, 18, 128, 18, SH110X_WHITE);
  if (noteCount == 0) {
    display.setCursor(4, 30); display.print(langEnglish ? "No notes yet." : "Palibe malemba.");
    display.drawLine(0, 54, 127, 54, SH110X_WHITE); display.setCursor(0, 56); display.print("BACK=menu");
  } else {
    int rowY[] = {22, 33, 44}; int start = (selectedNote/3)*3;
    for (int i = start; i < min(noteCount, start+3); i++) {
      int row = i-start; String title = getNoteTitle(i);
      String lbl = String(i+1)+": "+title.substring(0,14);
      if (title.length()>14) lbl+="..";
      if (i==selectedNote) { display.fillRect(3,rowY[row],121,10,SH110X_WHITE); display.setTextColor(SH110X_BLACK); }
      else display.setTextColor(SH110X_WHITE);
      display.setCursor(6, rowY[row]+1); display.print(lbl); display.setTextColor(SH110X_WHITE);
    }
    if (noteCount>3) { display.setCursor(100,10); display.print(String(selectedNote+1)+"/"+String(noteCount)); }
    display.drawLine(0, 54, 127, 54, SH110X_WHITE); display.setCursor(0, 56); display.print("OK=open DEL=delete");
  }
  drawStatusBar(); display.display();
}

void drawViewNote(int index) {
  display.clearDisplay(); display.setFont(&FreeSerif9pt7b);
  display.setCursor(10, 14); display.setTextColor(SH110X_WHITE);
  String title = getNoteTitle(index);
  if (title.length()>12) title=title.substring(0,12)+"..";
  display.print(title); display.setFont(); display.drawLine(3,18,128,18,SH110X_WHITE);
  String fullNote=getNoteBody(index); String lines[40]; int totalLines=0;
  while (fullNote.length()>0 && totalLines<40) {
    int len=min((int)fullNote.length(),21);
    if (len==21&&fullNote.length()>21) { int sp=fullNote.lastIndexOf(' ',20); if(sp>8) len=sp+1; }
    lines[totalLines++]=fullNote.substring(0,len); fullNote=fullNote.substring(len); fullNote.trim();
  }
  int maxScroll=max(0,totalLines-3);
  if (noteScrollOffset>maxScroll) noteScrollOffset=maxScroll;
  int screenY[]={22,33,44};
  for (int i=0;i<3;i++) { int li=noteScrollOffset+i; if(li>=totalLines) break; display.setCursor(3,screenY[i]); display.print(lines[li]); }
  if (totalLines>3) { display.setCursor(98,10); display.print(String((noteScrollOffset/3)+1)+"/"+String((totalLines+2)/3)); }
  display.drawLine(0,54,127,54,SH110X_WHITE); display.setCursor(0,56); display.print("AI RD DEL BACK");
  drawStatusBar(); display.display();
}

void drawAILoading() {
  display.clearDisplay(); display.setFont(&FreeSerif9pt7b);
  display.setCursor(10,14); display.setTextColor(SH110X_WHITE); display.print("AI");
  display.setFont(); display.drawLine(3,18,128,18,SH110X_WHITE);
  display.setCursor(4,28); display.print("Loading...");
  display.setCursor(4,42); display.print("Please wait.");
  drawStatusBar(); display.display();
}

void drawAIPreview(String improved) {
  display.clearDisplay(); display.setFont(&FreeSerif9pt7b);
  display.setCursor(10,14); display.setTextColor(SH110X_WHITE); display.print("AI Result");
  display.setFont(); display.drawLine(3,18,128,18,SH110X_WHITE);
  String note=improved;
  for (int line=0;line<3;line++) {
    if (note.length()==0) break; int len=min((int)note.length(),21);
    display.setCursor(3,22+line*11); display.print(note.substring(0,len));
    note=(note.length()>21)?note.substring(21):"";
  }
  if (note.length()>0) { display.setCursor(110,43); display.print("..."); }
  display.drawLine(0,54,127,54,SH110X_WHITE); display.setCursor(0,56); display.print("OK=save BACK=keep");
  drawStatusBar(); display.display();
}

void drawDeleteConfirm() {
  display.clearDisplay(); display.setFont(&FreeSerif9pt7b);
  display.setCursor(10,14); display.setTextColor(SH110X_WHITE);
  display.print(langEnglish?"Delete?":"Chotsani?");
  display.setFont(); display.drawLine(3,18,128,18,SH110X_WHITE);
  String title=getNoteTitle(selectedNote);
  if (title.length()>18) title=title.substring(0,18)+"..";
  display.setCursor(4,26); display.print(title);
  display.setCursor(4,38); display.print(langEnglish?"Cannot be undone.":"Sichinthidwa.");
  display.drawLine(0,54,127,54,SH110X_WHITE); display.setCursor(0,56); display.print("OK=confirm BACK=no");
  drawStatusBar(); display.display();
}