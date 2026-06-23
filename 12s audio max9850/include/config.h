#pragma once

// ---------------------------------------------------------------
// TRIAL SKETCH CONFIG — VoiceRSS TTS -> I2S -> MAX98357A
// Standalone trial. Not part of the main DigiBraille firmware.
// ---------------------------------------------------------------

// ---------- WiFi + API credentials (not pins, just text values) ----------
#define WIFI_SSID     "wayne"
#define WIFI_PASSWORD "iamthegreatest"

#define VRSS_API_KEY  "72f72678e3ac4477a3638b69ef893e2d"
#define VRSS_HOST     "api.voicerss.org"
#define VRSS_VOICE    "Mike"
#define TEXT_TO_SPEAK "Hello, this is a test of the I2S audio path."

// ---------- I2S pins — the only real wires in this project ----------
#define I2S_BCLK 16   // Bit Clock
#define I2S_LRC  27   // Word Select (Left/Right Clock)
#define I2S_DIN  25   // Data In

// ---------- Storage ----------
#define TEMP_MP3_PATH "/voicerss_temp.mp3"
