#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Fonts/FreeSerif9pt7b.h>
#include "globals.h"

extern Adafruit_SH1106G display;

static const unsigned char PROGMEM image_heart_8x8[] = {
  0x66, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00
};
static const unsigned char PROGMEM wifi_bmp[] = {
  0x1C, 0x22, 0x5D, 0x41, 0x1C, 0x22, 0x08, 0x00
};

// ─── braille decode needed for live preview ──────────────────
#define BRAILLE_CAPITAL_IND  0b100000
#define BRAILLE_NUMBER_IND   0b111100

inline char _dispDecodeBraille(uint8_t code, bool& capNext, bool& numMode);

void drawStatusBar() {
  display.drawBitmap(115, 0, image_heart_8x8, 8, 8, SH110X_WHITE);
  if (WiFi.status() == WL_CONNECTED)
    display.drawBitmap(104, 0, wifi_bmp, 8, 8, SH110X_WHITE);
  else {
    display.drawLine(104, 1, 110, 7, SH110X_WHITE);
    display.drawLine(110, 1, 104, 7, SH110X_WHITE);
  }
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
  bool dC = capitalNext, dN = numberMode;
  char liveChar = (liveCode == 0) ? ' ' : _dispDecodeBraille(liveCode, dC, dN);
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

// Minimal decode just for live preview in drawBraillePad
inline char _dispDecodeLetter(uint8_t c) {
  switch (c) {
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
    default: return '?';
  }
}
inline char _dispDecodeBraille(uint8_t code, bool& capNext, bool& numMode) {
  if (code == 0) return ' ';
  if (code == BRAILLE_CAPITAL_IND) return '^';
  if (code == BRAILLE_NUMBER_IND)  return '#';
  char c = _dispDecodeLetter(code);
  if (capNext && c >= 'a' && c <= 'z') { capNext = false; return (char)(c-32); }
  return c;
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
  display.setCursor(10, 28); display.print("SELECT = English");
  display.setCursor(10, 42); display.print("DOWN   = Chichewa");
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
