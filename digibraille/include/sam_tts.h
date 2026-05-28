#pragma once
#include <Arduino.h>

// SAM TTS — function declarations only
// Implementation is in sam_tts.cpp which includes AudioTools in isolation

void samSetup();
void samSay(const char* text);
void samSayString(String t);
void samSayChunked(String text);
void samSayChunkedInterruptible(String text);
