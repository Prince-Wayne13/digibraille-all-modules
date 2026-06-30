# DigiBraille Project Architecture (v2 — Offline Redesign)

This document replaces the previous architecture description. It reflects the offline-only redesign decided on in the current UX session. Anything related to WiFi, VoiceRSS, Gemini, AI-preview, or background audio fetching has been removed — these are deleted from the design, not deferred.

---

## 1. Project Scope

DigiBraille is an ESP32-based, fully offline assistive Braille input and note-taking device for visually impaired users. The device:

- Accepts Braille input through six dot buttons
- Provides spoken feedback in English or Chichewa
- Shows minimal status information on an OLED screen, intended for sighted helpers rather than the blind user
- Stores notes on an SD card, with system/menu assets on internal flash (LittleFS)
- Offers an offline grammar correction feature for saved notes

Supporting modules (`flashmemory/`, `grammarllm/`, `trial/`) are described in Section 8.

---

## 2. Hardware Pin Map (current, post-revision)

| Group | Function | Pin(s) | Notes |
|---|---|---|---|
| Braille Dots | Dot 1–6 | GPIO 32 / 33 / 18 / 19 / 23 / 26 | `INPUT_PULLUP`, pressed = LOW |
| Control Button | Back | GPIO 13 | Internal pull-up |
| Control Buttons | Select / Down / Save / Delete | GPIO 34 / 35 / 36 / 39 | Input-only pins — **require external pull-up resistors**, cannot use internal pull-up |
| OLED | SDA / SCL | GPIO 21 / 22 | I2C, SH1106 128x64, address `0x3C` |
| Amplifier (MAX98357A) | DIN / BCLK / LRC | GPIO 25 / 16 / 27 | I2S audio output |
| SD Card | CS / SCK / MISO / MOSI | GPIO 5 / 14 / 4 / 17 | SPI |

**Known shared-pin note:** GPIO 4 is used as SD-MISO. An earlier finding flagged this pin as causing upload failures when the SD card was physically wired during flashing. As of this pin revision, it has been reported working reliably — this is **not** treated as confirmed-fixed, since intermittent hardware faults can pass casual testing. Monitor during further development.

**Control button count:** Five — Select, Back, Down, Save, Delete. The previous **Reread** button has been removed from the design.

---

## 3. Audio Architecture

| Audio Type | Storage | Behavior |
|---|---|---|
| Menu / system prompts (English + Chichewa) | LittleFS (`/sfx/en/`, `/sfx/ch/`) | Always available, regardless of SD card state. Played via MP3 playback through the MAX98357A. |
| Note content (what the user writes/reads) | Synthesized; cached as audio on SD card | Normally cached on SD for faster replay. If SD card is unavailable, falls back to **live SAM TTS synthesis** every time, instead of cached playback. |

Audio output has been migrated from the ESP32 DAC + PAM8403 to the **MAX98357A I2S amplifier**. (`sam_tts.cpp` and `mp3_player.cpp` still require conversion from `dacWrite()` to `AudioOutputI2S` — open implementation task, not yet done.)

**Text pipeline:** The Braille decoder resolves all markers (capital indicator, number indicator, punctuation) into plain text **before** anything reaches storage or speech. SAM TTS and the grammar engine only ever operate on ordinary text — they never see raw Braille markers.

---

## 4. Storage Architecture

### LittleFS (internal flash)
Always available, independent of SD card state. Holds:
- Menu/system MP3 prompts (English + Chichewa)
- The grammar engine's own dictionary/rule data
- Device config (language setting, etc.)

### SD Card
Holds the user's actual work. Used for:
- Saved notes (text)
- Pre-correction recovery backups (originals saved before grammar correction overwrites a note)
- Cached note-reading audio

### SD Card Failure Handling

| Scenario | Behavior |
|---|---|
| SD card trips or is removed mid-use | Notes fall back to saving temporarily on LittleFS. Spoken warning + screen warning + short beep, repeated **every time the user reaches the main menu**, until resolved. |
| SD card returns (detected on boot) | Temporary LittleFS notes are automatically moved to SD card: copy to SD, verify the copy, then delete the LittleFS original. This happens **silently** — no user-facing message on success. A warning is only shown if the copy/verification fails. |
| SD card unavailable during grammar correction | Correction still runs (engine data lives on LittleFS), but the recovery backup step is skipped. |
| SD card unavailable for note-reading audio | Falls back to live SAM TTS synthesis instead of cached audio playback. |

---

## 5. Application Flow

### 5.1 Boot & Language
- **First-ever boot:** device asks the user to select a language (English / Chichewa), then remembers it.
- **Every boot after:** device goes straight to the main menu in the last-used language.
- Language is changeable at any time via a main menu item.

### 5.2 Menu & List Navigation
- **Down-only**, wraps around: pressing Down past the last item returns to the first. No dedicated Up button, no dead ends.

### 5.3 New Note Flow
1. User selects "New note" from the main menu.
2. Device asks: **"Add a title?"** — Select = yes, Back = no.
3. **If yes:** user writes the title in Braille → Save confirms the title → moves to body writing.
4. **If no:** device goes straight to body writing.
5. User writes the body in Braille.
6. User presses Save to finish.
7. Body is saved instantly.
8. **If no title was given**, the device instantly generates one from the body: takes the first 3 non-filler words (filtering a small blacklist — "i," "the," "a," "so," "and," "um," "to," etc.) and formats it as `note1: systems are wrong`. This happens synchronously on the same core — no second-core task is used, since the operation is trivial (well under a millisecond) and introduces no perceptible delay.
9. Device plays a "note saved" confirmation.
10. Returns to main menu.

### 5.4 Text Editing Behavior (Back Button)

| Action | Behavior |
|---|---|
| Single tap | Delete last character |
| Long press | Delete back to the last space, or to the start of the buffer if no space exists yet (Ctrl+Backspace style) |
| Tap or long press on a **completely empty buffer** | Exit to main menu, nothing saved |

### 5.5 Save Button Behavior (Context-Dependent)

| Context | Behavior |
|---|---|
| Writing title/body, buffer has content | Save note / confirm title and move to body |
| Writing title/body, buffer is empty | Exit to main menu, nothing saved (same outcome as Back in this state) |
| Viewing a saved note (Read Notes) | Trigger offline grammar correction on that note |
| After grammar correction plays | Accept — corrected version overwrites the saved note |

**Design rule used throughout:** Save always means commit/move forward. Back always means undo/discard. This mapping is consistent across every screen in the device.

### 5.6 Reading a Note & Grammar Correction
1. User opens a saved note from Read Notes.
2. Note plays back (cached SD audio, or live SAM TTS if SD/cache unavailable).
3. User presses Save to trigger grammar correction.
4. Device silently backs up the original text to SD card (copy verified).
5. The `grammarllm` pipeline runs (Normalizer → Tokenizer → SpellChecker → POS Tagger → GrammarEngine → QuestionDetector → PunctuationEngine → Reconstructor).
6. Device says **"here's the corrected version"**, then plays the corrected audio.
7. **Save** = accept, corrected version overwrites the saved note.
   **Back** = reject, original is kept, correction is discarded.

### 5.7 Note Deletion — OPEN ITEM
Not yet finalized. Delete button (GPIO 39) is the natural candidate to initiate deletion, with Save/Back handling confirm/cancel per the established pattern. To be completed in a future session.

---

## 6. OLED Display Design

**Primary audience: sighted helpers, teachers, or parents glancing at the device** — not the blind user, and not developer debugging. Detail level is deliberately minimal.

### Design Rules
- Show only the current state name and the one piece of content relevant to it (e.g., text typed so far).
- No persistent status indicators or icons of any kind — no WiFi icon, no "heart" icon, no SD card icon.
- The only on-screen interruptions are **temporary warning windows**: appear briefly, paired with spoken audio, then clear.
- Warnings relevant to the **blind user's experience or the safety of their work** (e.g., SD card missing, recovery backup unavailable) trigger a short, non-annoying beep + spoken message + screen warning, **every time the user arrives at the main menu**, repeating each visit until resolved.
- Developer/debug-only issues (corrupted internal files, firmware bugs) never surface to the user in any form — on-screen or spoken.

### Screen Content by State

| State | OLED Shows |
|---|---|
| Startup | "WAYNE INC." |
| Language select (first boot only) | "Select Language" + currently highlighted option |
| Main Menu | Currently selected menu item name |
| Write Title | "Title:" + text typed so far |
| Write Body | "Note:" + text typed so far (most recent visible line if long) |
| Read Notes (list) | Currently highlighted note name |
| View Note | Note title + full text |
| Grammar Correction running | "Correcting..." |
| Grammar Correction preview | "Corrected:" + corrected text |
| Delete confirm (pending design) | Note name + "Delete this note?" |
| SD card warning | "SD card not found — saving temporarily" |

---

## 7. Known Open Items

- `sam_tts.cpp` and `mp3_player.cpp` still need conversion from `dacWrite()` to `AudioOutputI2S` for the MAX98357A.
- `grammarllm/` needs to be integrated into the main firmware to support the Save-triggered correction flow described in 5.6.
- Note deletion flow (5.7) is undesigned.
- Battery power (3.7V Li-ion + TP4056 + 5V booster) implementation is planned but not yet wired.
- GPIO pin definitions should be consolidated into a single hardware config file.
- Actual SD card read/write subsystem (mount, error handling, copy-verify-delete logic) is not yet implemented in code — this document describes the intended behavior, not existing code.
- A Braille input timing test (logging per-letter/per-cell timestamps during real typing) is planned, to replace estimated typing-speed assumptions with real numbers.

---

## 8. Supporting Modules (Unchanged Scope)

These modules are described in full in the prior architecture document and are not affected by today's redesign, except where noted.

| Block | Folder | Purpose | Status |
|---|---|---|---|
| External Flash Memory | `flashmemory/` | Experimental external SPI flash storage for larger audio/asset libraries. | Separate prototype; pin overlap with main board needs resolving if integrated. Lower priority given SD card now fills this role. |
| Offline Grammar Engine | `grammarllm/` | Rule-based, dictionary-based offline grammar correction. | Now has a defined entry point in the UX (Section 5.6) but not yet integrated into main firmware. |
| Chichewa Dataset / Audio Lab | `trial/` | PC/web tool for recording, segmenting, and synthesizing Chichewa audio units. | Unchanged — supports asset creation for `/sfx/ch/`. |

---

## 9. Removed From Design (No Longer Applicable)

The following existed in the previous architecture and have been fully removed, not deferred:

- WiFi connectivity and setup
- VoiceRSS online TTS and caching
- Gemini API note improvement (`callGemini()`)
- AI Preview firmware state
- FreeRTOS background audio-fetch task and queue (Core 1 download task)
- WiFi status icon and "heart" icon on the OLED status bar
- Reread control button
- DAC + PAM8403 audio path (replaced by MAX98357A I2S)