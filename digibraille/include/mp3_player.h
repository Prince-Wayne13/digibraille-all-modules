#pragma once
#include <Arduino.h>

// MP3 + VoiceRSS — function declarations only
// Implementation is in mp3_player.cpp which includes ESP8266Audio in isolation

void mp3Setup();
void playMP3(String path);
bool vrssFetch(String text, String path);
void speakPhrase(const char* name);
void speakText(String text, String mp3Path);
void cacheMenuAudio();
void queueNoteAudio(int index);
void queueNoteTitleAudio(int index);
void audioFetchTask(void* param);
