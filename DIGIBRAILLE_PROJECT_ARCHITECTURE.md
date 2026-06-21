# DigiBraille All Modules: Complete Project Architecture

## 1. Overall Project Block Architecture

The DigiBraille All Modules project is organized as four major blocks. Each block corresponds to a folder in the root directory and represents a separate subsystem of the complete DigiBraille system.

| Block | Folder | Purpose |
|---|---|---|
| Main DigiBraille Device | `digibraille/` | Main ESP32 firmware for Braille input, note writing, note reading, OLED display, speech output, AI note improvement, and LittleFS storage. |
| External Flash Memory | `flashmemory/` | Experimental ESP32 firmware for storing larger audio files or other assets on an external SPI flash chip. |
| Offline Grammar LLM / Grammar Engine | `grammarllm/` | Embedded C++ grammar correction engine designed to run locally/offline on ESP32 without needing cloud AI. |
| Chichewa Dataset / Audio Lab | `trial/` | Chichewa audio dataset builder with backend, frontend, waveform editor, morpheme segmentation, unit library, and synthesis tools. |

The main block is `digibraille/`. The other three blocks support future expansion: more storage, offline intelligence, and Chichewa language/audio data creation.

---

## 2. Main Block: `digibraille/`

### 2.1 Purpose

The `digibraille` block is the core DigiBraille device firmware. It turns an ESP32 into a portable Braille note-taking and reading device. The device allows a visually impaired user to write notes using six Braille dot buttons, save notes to internal flash storage, read notes through speech, choose English or Chichewa prompts, and optionally improve notes using an AI service.

The implemented firmware includes:

- Six-dot Braille input.
- Navigation buttons for select, back/undo, down, reread, AI/save, and delete.
- OLED visual interface.
- English and Chichewa audio prompts.
- SAM offline speech synthesis fallback.
- MP3 playback through the ESP32 DAC.
- VoiceRSS online TTS caching for higher-quality MP3 speech.
- LittleFS storage for notes, settings, drafts, and audio cache.
- Optional Gemini API note improvement.
- Background FreeRTOS task for downloading/caching audio.

### 2.2 Target Hardware

The firmware is configured for:

- Board: ESP32 DevKit style board.
- PlatformIO board: `esp32dev`.
- Framework: Arduino.
- Filesystem: LittleFS.
- Partition table: `huge_app.csv`.
- Serial monitor speed: `115200`.

### 2.3 Hardware Pin Map

The implemented pin map is taken from the compiled constants in `digibraille/include/globals.h` and `digibraille/src/main.cpp`.

| Hardware Part | Signal | ESP32 GPIO | Notes |
|---|---:|---:|---|
| OLED Display | SDA | GPIO 21 | I2C data line. |
| OLED Display | SCL | GPIO 22 | I2C clock line. |
| OLED Display | I2C address | `0x3C` | SH1106 128x64 OLED. |
| Select Button | `BTN_SELECT` | GPIO 14 | Uses `INPUT_PULLUP`; pressed = LOW. |
| Back / Undo Button | `BTN_BACK` | GPIO 27 | Uses `INPUT_PULLUP`; pressed = LOW. |
| Down / Next Button | `BTN_DOWN` | GPIO 13 | Uses `INPUT_PULLUP`; pressed = LOW. |
| Reread Button | `BTN_REREAD` | GPIO 12 | Uses `INPUT_PULLUP`; pressed = LOW. |
| AI / Save Button | `BTN_AISAVE` | GPIO 4 | Uses `INPUT_PULLUP`; pressed = LOW. |
| Delete / Clear Button | `BTN_DELETE` | GPIO 17 | Uses `INPUT_PULLUP`; pressed = LOW. |
| Speaker Output | DAC | GPIO 25 | Analog audio output to PAM8403 amplifier. |
| Braille Dot 1 | Dot 1 | GPIO 32 | Uses `INPUT_PULLUP`; dot pressed to GND. |
| Braille Dot 2 | Dot 2 | GPIO 33 | Uses `INPUT_PULLUP`; dot pressed to GND. |
| Braille Dot 3 | Dot 3 | GPIO 18 | Uses `INPUT_PULLUP`; dot pressed to GND. |
| Braille Dot 4 | Dot 4 | GPIO 19 | Uses `INPUT_PULLUP`; dot pressed to GND. |
| Braille Dot 5 | Dot 5 | GPIO 23 | Uses `INPUT_PULLUP`; dot pressed to GND. |
| Braille Dot 6 | Dot 6 | GPIO 26 | Uses `INPUT_PULLUP`; dot pressed to GND. |

Important implementation note: the comment block in `digibraille/src/main.cpp` says `BTN_DELETE` is GPIO 5, but the actual compiled firmware constant is GPIO 17. The documentation should use GPIO 17 unless the code is changed.

### 2.4 Button Behavior

| Button | Main Function |
|---|---|
| Select | Confirms menu item, commits current Braille cell, opens notes, saves AI preview. |
| Back | Goes back in menus; in Braille mode acts as undo. Single tap undoes a character, double tap undoes a sentence, double tap on empty buffer exits to main menu. |
| Down | Moves through menu items and note list; scrolls note view. |
| Reread | Repeats current buffer, note title, or full note. |
| AI / Save | In title mode confirms title; in note mode saves note; in note view starts AI improvement. |
| Delete | Clears Braille buffer or starts note deletion confirmation. |

All buttons use internal pull-ups, so each switch should connect the GPIO pin to GND when pressed.

### 2.5 Braille Input Architecture

The Braille input block uses six physical dot inputs mapped to:

```text
DOT_PINS = {32, 33, 18, 19, 23, 26}
```

Each dot is debounced with `DOT_DEBOUNCE_MS = 15`. A Braille cell is built by pressing one or more dot buttons, and the user commits the cell using the Select button.

The firmware supports:

- Letters `a` to `z`.
- Space using empty cell.
- Period, exclamation mark, and question mark.
- Capital indicator using binary pattern `0b100000`.
- Number indicator using binary pattern `0b111100`.
- Numbers 1 to 0 after number mode is activated.

The Braille decoder is implemented in `digibraille/include/braille.h`. Characters are stored in a history buffer:

```text
MAX_HISTORY = 512 characters
```

The note writing flow is:

1. User selects `New note`.
2. Device enters title writing state.
3. User writes the title using Braille.
4. User presses AI/Save to confirm title.
5. Device enters body writing state.
6. User writes the note body using Braille.
7. User presses AI/Save to save the note.

### 2.6 OLED Display Architecture

The display block uses an Adafruit SH1106 OLED:

- Resolution: 128 x 64.
- I2C address: `0x3C`.
- Driver class: `Adafruit_SH1106G`.
- Libraries: Adafruit GFX and Adafruit SH110X.

The display module is implemented in `digibraille/include/display.h`.

Main screens:

- Startup screen: displays `WAYNE INC.`
- Language selection screen.
- Main menu.
- Braille writing pad.
- Read notes list.
- View note screen.
- AI loading screen.
- AI preview screen.
- Delete confirmation screen.

The top status bar includes:

- Heart icon.
- WiFi connected icon or disconnected mark.

### 2.7 Audio Architecture

The audio system has two speech paths that share GPIO 25:

| Audio Path | File | Purpose |
|---|---|---|
| SAM TTS | `src/sam_tts.cpp` | Offline fallback speech using SAM speech synthesis. |
| MP3 Player | `src/mp3_player.cpp` | Plays cached MP3 prompts and note audio from LittleFS. |

Both paths output audio using `dacWrite(25, value)`. The speaker output is expected to go into an amplifier such as the PAM8403.

The firmware intentionally separates SAM and MP3 into different `.cpp` files because both libraries define audio-related classes with overlapping names. Keeping them in separate translation units avoids class name conflicts.

SAM settings:

- Voice: `SAM::Elf`.
- Pitch: `64`.
- Throat: `128`.
- Speed: `48`.
- Mouth: `150`.
- Sample rate: `22050 Hz`.
- Channels: mono.
- Bits per sample: 16-bit.

MP3 settings:

- MP3 decoder: `AudioGeneratorMP3`.
- File source: `AudioFileSourceLittleFS`.
- Output: custom `DacOutput`.
- Target DAC pin: GPIO 25.
- Sample rate: 22050 Hz.
- Mono output.

### 2.8 Speech Prompt System

The firmware has a phrase table in `digibraille/src/main.cpp`. Each phrase has:

- A phrase name.
- English text.
- Chichewa text.

Examples:

- `new_note`: “New note.” / “Lemba latsopano.”
- `read_notes`: “Read notes.” / “Werenga malemba.”
- `language`: “Language.” / “Chilankhulo.”
- `note_saved`: “Note saved.” / “Lembalo lasungidwa.”
- `storage_full`: “Storage full.” / “Malo odzaza.”

English prompts are loaded from:

```text
/sfx/en/<phrase>.mp3
```

Chichewa prompts are loaded from:

```text
/sfx/ch/<phrase>.mp3
```

If an MP3 file is missing or invalid, the firmware falls back to SAM speech. For English, if WiFi is connected, it also queues a background VoiceRSS download to cache the MP3 for next time.

### 2.9 Storage Architecture

The main firmware uses LittleFS internal ESP32 flash storage.

Storage folders and files:

| Path | Purpose |
|---|---|
| `/notes` | Stores saved notes and note MP3s. |
| `/sfx/en` | Stores English MP3 prompt cache. |
| `/sfx/ch` | Stores Chichewa MP3 prompt files. |
| `/draft.txt` | Autosaved draft while writing. |
| `/config.txt` | Stores language selection, `en` or `ch`. |
| `/notes/note0.txt` to `/notes/note9.txt` | Saved note text files. |
| `/notes/note0.mp3` etc. | Cached body audio for notes. |
| `/notes/note0_title.mp3` etc. | Cached title audio for notes. |

Storage limits:

```text
MAX_NOTES = 10
MAX_HISTORY = 512
```

Each note text file stores:

```text
Title
Body text
```

The firmware creates required folders on boot using `ensureFolders()`.

### 2.10 AI Note Improvement

The current online AI feature is implemented in `digibraille/include/buttons.h` through `callGemini()`.

The function:

1. Checks WiFi.
2. Builds a prompt asking the model to rewrite the note.
3. Sends HTTPS request to Gemini API.
4. Parses JSON response using ArduinoJson.
5. Shows the improved note in the AI preview screen.
6. Allows the user to save or reject the improved note.

Configured model:

```text
gemini-2.5-flash
```

The prompt asks the model to:

- Fix grammar.
- Add key phrases.
- Use a maximum of 5 sentences.
- Use a maximum of 7 words per sentence.
- Return only the rewritten note.

Future direction: this online AI block can later be replaced or supplemented by the offline `grammarllm/` block.

### 2.11 FreeRTOS Background Audio Fetching

The firmware creates an audio fetch queue:

```text
audioFetchQueue = xQueueCreate(10, sizeof(AudioFetchJob))
```

It starts a FreeRTOS task pinned to Core 1:

```text
xTaskCreatePinnedToCore(audioFetchTask, "AudioFetch", 8192, nullptr, 1, nullptr, 1)
```

Core 0 handles the main UI loop and user interaction. Core 1 handles slow audio downloads in the background.

Background jobs include:

- First-run menu audio caching.
- Missing phrase MP3 fetching.
- Missing note title MP3 fetching.
- Missing note body MP3 fetching.

This prevents the user interface from freezing while audio files are being downloaded.

### 2.12 Main Firmware States

The firmware state machine is defined in `globals.h`.

| State | Purpose |
|---|---|
| `STATE_STARTUP` | Startup splash and boot delay. |
| `STATE_LANG_SELECT` | User chooses English or Chichewa. |
| `STATE_MAIN_MENU` | Main menu with New note, Read Notes, Language. |
| `STATE_WRITE_TITLE` | Braille input mode for note title. |
| `STATE_NEW_NOTE` | Braille input mode for note body. |
| `STATE_READ_NOTES` | Browse saved notes. |
| `STATE_VIEW_NOTE` | View/read a selected note. |
| `STATE_AI_PREVIEW` | Preview AI-improved note. |
| `STATE_DELETE_CONFIRM` | Confirm note deletion. |

### 2.13 Main Firmware Software Files

| File | Purpose |
|---|---|
| `src/main.cpp` | Main setup, loop, global instances, phrase table, boot sequence, state routing. |
| `src/mp3_player.cpp` | MP3 playback, VoiceRSS fetching, MP3 validation, audio fetch task. |
| `src/sam_tts.cpp` | Offline SAM text-to-speech through ESP32 DAC. |
| `include/globals.h` | Pin definitions, constants, global variables, state enum, helper functions. |
| `include/braille.h` | Braille decoding, dot input handling, undo, save note, title/body writing. |
| `include/buttons.h` | Menu handling, read-note handling, AI flow, delete flow, Gemini API call. |
| `include/display.h` | OLED drawing functions and screen layouts. |
| `include/storage.h` | LittleFS folders, note loading/saving, config, drafts. |
| `include/mp3_player.h` | MP3/audio function declarations. |
| `include/sam_tts.h` | SAM speech function declarations. |

### 2.14 DigiBraille Data Assets

The `digibraille/data/sfx/ch/` folder contains Chichewa MP3 prompt files, including:

- `new_note.mp3`
- `read_notes.mp3`
- `language.mp3`
- `write_title.mp3`
- `title_saved.mp3`
- `note_saved.mp3`
- `storage_full.mp3`
- `ai_ready.mp3`
- `ai_failed.mp3`
- `no_wifi.mp3`

These files are intended to be uploaded to LittleFS and used when the user chooses Chichewa.

---

## 3. External Storage Block: `flashmemory/`

### 3.1 Purpose

The `flashmemory` block is an experimental external SPI flash storage subsystem. Its purpose is to provide extra space beyond the ESP32 internal LittleFS partition. This is useful because DigiBraille uses many audio files, and internal flash can become limited when storing MP3 prompts, note recordings, or future Chichewa speech assets.

### 3.2 Target Hardware

The block is also configured for:

- Board: ESP32 DevKit.
- Framework: Arduino.
- PlatformIO board: `esp32dev`.
- Serial monitor speed: `115200`.
- Upload speed: `921600`.
- Library: `Marzogh/SPIMemory`.

### 3.3 SPI Flash Pin Map

The implemented firmware uses:

| SPI Signal | ESP32 GPIO |
|---|---:|
| CS | GPIO 5 |
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |

This overlaps with some Braille dot pins in the main DigiBraille firmware. Therefore, the external flash block is currently a separate experiment and should be integrated carefully if used in the final device.

### 3.4 Flash Storage Layout

The firmware uses a simple persistent file table:

| Address Range | Purpose |
|---|---|
| `0x0000` to `0x0FFF` | First 4 KB sector reserved for file table. |
| `0x1000` onward | Stored file contents. |

Constants:

```text
FLASH_PAGE_SIZE   = 256 bytes
FLASH_SECTOR_SIZE = 4096 bytes
MAX_FILES         = 32
FILENAME_MAX_LEN  = 48
TABLE_ADDR        = 0x0000
FILE_START_ADDR   = 0x1000
```

Each file table entry stores:

- File name.
- Start address.
- File size.
- Used/free flag.

### 3.5 Serial Protocol

The ESP32 receives commands over Serial.

| Command | Purpose |
|---|---|
| `PING` | Test connection; replies `PONG`. |
| `ERASE` | Erase flash chip and reset file table. |
| `STATUS` | Print next write address and file count. |
| `WRITE:filename:size` | Prepare to receive binary file data. |
| `LIST` | List stored files with size and address. |
| `DUMP` | Dump the first stored files as hex/ASCII for inspection. |

The write flow is:

1. PC sends `WRITE:filename:size`.
2. ESP32 replies `OK:READY:<address>`.
3. PC sends raw binary bytes.
4. ESP32 writes data in 256-byte flash pages.
5. ESP32 updates the persistent table.
6. ESP32 replies `OK:DONE`.

### 3.6 Role in the Full DigiBraille System

This block can become the long-term storage extension for:

- Larger Chichewa audio prompt libraries.
- Cached TTS MP3 files.
- Saved note audio.
- Offline grammar dictionaries.
- Future local language models or compressed data tables.

At the current stage, it is separate from the main `digibraille/` firmware and acts as a storage proof of concept.

---

## 4. Offline Grammar Block: `grammarllm/`

### 4.1 Purpose

The `grammarllm` block is an offline grammar correction engine for ESP32. Although the folder name says “LLM”, the current implementation is a compact rule-based and dictionary-based grammar engine rather than a neural language model. This makes it more realistic for microcontroller use because it can run without internet access and without cloud APIs.

Its purpose is to become an offline DigiBraille feature that improves typed notes locally.

### 4.2 Target Platform

Configured for:

- Board: ESP32 DevKit.
- Framework: Arduino.
- C++ standard: GNU++17.
- Serial monitor speed: `115200`.

### 4.3 Processing Pipeline

The grammar engine pipeline in `grammarllm/src/main.cpp` is:

```text
Input sentence
    -> Normalizer
    -> Tokenizer
    -> SpellChecker
    -> POS Tagger
    -> GrammarEngine
    -> QuestionDetector
    -> PunctuationEngine
    -> Reconstructor
    -> Corrected output
```

Each input sentence is typed through Serial and processed locally on the ESP32.

### 4.4 Software Modules

| Module | Purpose |
|---|---|
| `Normalizer` | Cleans input and splits fused words such as `iam` to `i am`. |
| `Tokenizer` | Splits input into word and punctuation tokens. |
| `SpellChecker` | Corrects spelling using dictionary and Levenshtein-style matching. |
| `POSTagger` | Assigns part-of-speech tags using tables and bigram transitions. |
| `GrammarEngine` | Applies grammar correction rules. |
| `QuestionDetector` | Detects question-like sentences. |
| `PunctuationEngine` | Adds or corrects punctuation. |
| `Reconstructor` | Rebuilds corrected tokens into final text. |
| `RegressionRunner` | Runs test cases on boot and on command. |

### 4.5 Grammar Rules

The grammar engine includes rules for:

- Capitalizing the pronoun `I`.
- Correcting `was` and `were`.
- Subject-verb agreement.
- Noun-be agreement.
- Plural subject-verb correction.
- Their/there/they’re correction.
- Past tense correction when past-time words are detected.
- Missing article insertion.
- `a` versus `an`.
- Double negative correction.
- Adverb placement.
- Question protection so normal declarative grammar rules do not corrupt questions.

Example intended corrections:

```text
i go school       -> I go to school / I go school. depending available rules
he go             -> he goes
the dogs helps    -> the dogs help
the dog are       -> the dog is
iam happy         -> i am happy
iknow             -> i know
donot             -> do not
cannot            -> can not
```

### 4.6 Debug and Testing Features

Serial commands:

| Command | Purpose |
|---|---|
| `test` | Re-run regression test suite. |
| `debug` | Toggle debug timing and processing logs. |
| `stats` | Show session input and correction counts. |

Debug mode prints timing per stage:

- Normalization time.
- Tokenization time.
- Spell check time.
- POS tagging time.
- Grammar correction time.
- Punctuation correction time.
- Reconstruction time.

### 4.7 Role in the Full DigiBraille System

The current main DigiBraille firmware uses online Gemini for AI note improvement. The `grammarllm` block is the offline replacement or backup. In the final architecture, DigiBraille can use:

- Online AI when WiFi is available.
- Offline grammar engine when WiFi is unavailable.
- A hybrid approach where simple grammar corrections happen locally and larger rewriting uses online AI.

This would make DigiBraille more reliable in schools and rural settings where internet access may not be stable.

---

## 5. Chichewa Dataset / Audio Lab Block: `trial/`

### 5.1 Purpose

The `trial` block is the Chichewa dataset creation and audio synthesis workstation. It is not ESP32 firmware. It is a PC/server-based tool used to record, clean, segment, label, save, and synthesize Chichewa audio units.

Its purpose is to build language resources for DigiBraille, especially Chichewa speech prompts and reusable morpheme-level audio units.

### 5.2 High-Level Architecture

The block has two main parts:

| Part | Folder | Purpose |
|---|---|---|
| Backend | `trial/backend/` | FastAPI server for audio processing, unit storage, synthesis, and library access. |
| Frontend | `trial/frontend/` | React + Vite web app with waveform editor, recording/upload UI, unit library, and synthesis lab. |

Data flow:

```text
Microphone or uploaded audio
    -> Frontend FormData upload
    -> FastAPI /api/record
    -> Audio cleaning and normalization
    -> F0 extraction
    -> Proposed morpheme segmentation
    -> Cleaned WAV returned to frontend
    -> User edits/accepts waveform regions
    -> /api/save-unit
    -> WAV unit saved on disk
    -> Metadata saved in SQLite
    -> Unit available for synthesis
```

### 5.3 Backend Architecture

Backend technology:

- Python.
- FastAPI.
- SQLite.
- Pydub.
- SciPy.
- NumPy.
- noisereduce.
- Praat-Parselmouth.
- Uvicorn.

Main backend files:

| File | Purpose |
|---|---|
| `backend/main.py` | FastAPI application and API routes. |
| `backend/audio_utils.py` | Audio loading, cleaning, F0 extraction, and morpheme cut proposal. |
| `backend/db.py` | SQLite database initialization, saving units, searching units, deleting units. |
| `backend/synthesis.py` | Audio joining and transition methods. |
| `backend/requirements.txt` | Python dependencies. |
| `backend/units.db` | SQLite database containing saved unit metadata. |
| `backend/uploads/` | Stored WAV units and temporary uploads. |

### 5.4 Backend API Routes

| Route | Method | Purpose |
|---|---|---|
| `/api/record` | POST | Receive audio and transcript, clean audio, extract F0, propose morpheme cuts, return cleaned WAV as hex. |
| `/api/save-unit` | POST | Save an accepted audio segment and metadata into the unit library. |
| `/api/library` | GET | Search/list saved audio units. |
| `/api/unit/{filename}` | GET | Return a saved WAV unit. |
| `/api/unit/{filename}` | DELETE | Delete a saved unit file and remove database record. |
| `/api/synthesize` | POST | Join selected units into a synthesized utterance. |
| `/api/compare-transitions` | POST | Compare different transition methods between two units. |

### 5.5 Audio Processing Pipeline

The audio utilities perform:

1. Decode uploaded audio using Pydub.
2. Convert to mono.
3. Resample to 16 kHz.
4. Convert to 16-bit samples.
5. Apply high-pass filter at 80 Hz.
6. Apply noise reduction.
7. Apply pre-emphasis.
8. Normalize amplitude to 95 percent peak.
9. Extract F0 using autocorrelation.
10. Propose morpheme cuts using energy thresholding.

Output sample rate:

```text
16000 Hz
```

The segmentation output includes:

- Start time in milliseconds.
- End time in milliseconds.
- Duration.
- Transcript segment.
- Morphological type.
- F0 mean.
- F0 onset.
- F0 offset.
- Confidence score.

### 5.6 Unit Library Database

The SQLite table is called `units`.

Stored fields include:

- `id`
- `filename`
- `transcript`
- `morph_type`
- `grammatical_role`
- `tone_pattern`
- `duration_ms`
- `f0_mean`
- `f0_onset`
- `f0_offset`
- `preceding_context`
- `following_context`
- `quality_score`
- `created_at`
- `tags`

The database has indexes on:

- `transcript`
- `morph_type`

Saved filenames follow this pattern:

```text
{transcript}_{morph_type}_{tone_pattern}_{hash}.wav
```

### 5.7 Synthesis Lab

The synthesis module supports multiple transition methods:

| Method | Purpose |
|---|---|
| `crossfade_25ms` | Standard 25 ms overlap crossfade. |
| `crossfade_40ms` | Longer 40 ms overlap crossfade. |
| `pitch_matched` | Attempts to shift onset pitch of the second unit before joining. |
| `overlap_add` | Uses windowed overlap-add joining for smoother transitions. |

This allows Chichewa morphemes or word units to be combined into larger spoken phrases.

### 5.8 Frontend Architecture

Frontend technology:

- React 18.
- Vite.
- Axios.
- WaveSurfer.js.
- Lucide React.

Main frontend files:

| File | Purpose |
|---|---|
| `frontend/src/App.jsx` | Main state hub for recording, uploads, editor state, unit library, and view switching. |
| `frontend/src/WaveformEditor.jsx` | Waveform editor using WaveSurfer and regions. |
| `frontend/src/SynthesisLab.jsx` | UI for selecting units and synthesizing audio. |
| `frontend/src/MorphologyPanel.jsx` | UI for morphology analysis. |
| `frontend/src/utils/mic.js` | Browser microphone recording helper. |
| `frontend/src/utils/audio.js` | Audio helper utilities. |
| `frontend/src/index.css` | Styling. |

Frontend views:

- Waveform Editor.
- Synthesis Lab.
- Unit Library.

The app can:

- Record audio through the microphone.
- Upload audio files.
- Ask for a transcript.
- Send audio to backend.
- Display cleaned waveform.
- Show proposed segments.
- Save selected segments as reusable units.
- Browse saved units.
- Delete units.
- Synthesize new audio from saved units.

### 5.9 Role in the Full DigiBraille System

The `trial` block supports DigiBraille by creating and refining Chichewa language assets. These assets can later be exported into:

- Chichewa prompt MP3 files for `digibraille/data/sfx/ch/`.
- Chichewa speech units for future embedded synthesis.
- Datasets for improving pronunciation and morphology handling.
- Offline language resources for the grammar or speech modules.

---

## 6. Complete System Integration View

The intended full DigiBraille system can be described as:

```text
User
  -> Braille dot buttons
  -> ESP32 DigiBraille firmware
  -> Braille decoder
  -> Note buffer
  -> LittleFS storage
  -> OLED feedback
  -> Speech feedback through DAC + amplifier
  -> Optional AI/grammar correction
  -> Saved readable notes
```

Supporting systems:

```text
trial/
  -> Creates Chichewa speech data and morpheme audio units
  -> Exports audio assets to DigiBraille

grammarllm/
  -> Provides offline grammar correction logic
  -> Can be integrated into DigiBraille AI/save workflow

flashmemory/
  -> Provides extra external flash space
  -> Can hold large audio libraries or dictionaries
```

---

## 7. Hardware Architecture Summary

The physical DigiBraille hardware consists of:

- ESP32 development board as the main controller.
- Six Braille dot buttons connected to GPIO 32, 33, 18, 19, 23, and 26.
- Six control buttons connected to GPIO 14, 27, 13, 12, 4, and 17.
- SH1106 128x64 I2C OLED connected to GPIO 21 and GPIO 22.
- PAM8403 amplifier connected to DAC output GPIO 25.
- Speaker connected to the amplifier output.
- Optional external SPI flash connected to GPIO 5, 18, 19, and 23 in the separate flash memory prototype.

Because GPIO 18, 19, and 23 are used by Braille dots in the main firmware and SPI flash in the flash prototype, the final combined hardware would need either a changed pin map, an I/O expander, or a separate SPI wiring plan.

---

## 8. Software Architecture Summary

The software architecture is layered:

1. Hardware abstraction:
   - GPIO buttons.
   - OLED display.
   - DAC audio.
   - LittleFS storage.
   - WiFi.

2. Input layer:
   - Braille dot scanning.
   - Button debouncing.
   - Braille cell decoding.

3. Application layer:
   - State machine.
   - Menus.
   - Note writing.
   - Note browsing.
   - Note deletion.
   - Language selection.

4. Audio layer:
   - SAM fallback TTS.
   - MP3 playback.
   - VoiceRSS audio fetching.
   - Chichewa prompt playback.

5. Storage layer:
   - Notes.
   - Drafts.
   - Config.
   - Cached audio.

6. Intelligence layer:
   - Online Gemini rewriting in current main firmware.
   - Offline grammar engine in `grammarllm/`.

7. Dataset/tooling layer:
   - Chichewa audio recording.
   - Morpheme segmentation.
   - Unit library.
   - Audio synthesis.

---

## 9. Current Limitations and Integration Notes

- The main DigiBraille firmware currently stores only 10 notes by default.
- English online TTS uses VoiceRSS, but the API key placeholder must be filled before use.
- Gemini API key is currently hardcoded in source and should be moved to a private config before sharing.
- The main firmware has WiFi setup commented out, so online features depend on re-enabling WiFi initialization.
- `BTN_DELETE` has a documentation mismatch: comment says GPIO 5, compiled constant says GPIO 17.
- The external flash prototype uses pins that overlap with main Braille dot pins.
- `grammarllm/` is offline and embedded, but not yet integrated directly into the main DigiBraille note workflow.
- `trial/` is a PC/web tool for dataset creation, not firmware for the ESP32 device.

---

## 10. Final Project Description

DigiBraille is an ESP32-based assistive note-taking system for blind and visually impaired users. The main device accepts Braille input through six dot buttons, provides spoken feedback through a speaker, shows status and menus on a small OLED screen, stores notes locally in flash memory, and supports both English and Chichewa prompts. It also includes an AI-assisted note improvement workflow.

The wider project includes supporting modules for external flash memory expansion, offline grammar correction, and Chichewa dataset/audio generation. Together, these modules form a complete path toward a low-cost, multilingual, offline-capable Braille learning and note-taking device.
