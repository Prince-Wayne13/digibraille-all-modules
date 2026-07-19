# DigiBraille v2 — Changelog

**Author:** Sam Claude **Date:** 2026-07-15 **Scope:** `mp3_player.h`, `mp3_player.cpp`, `buttons.h`, `braille.h`, `display.h`, `globals.h`, `main.cpp`

---

## mp3_player.h

- Changed `playMP3`, `speakText`, `vrssFetch` from pass-by-value `String` to `const String&` to avoid unnecessary heap copies.
- Added missing `bool isValidMp3(String path);` declaration (previously only existed as a stray, now-removed redeclaration in `buttons.h`).
- Added `extern int lastInterruptPin;` — reports which button (if any) cut short the most recent `playMP3()` call.

## mp3_player.cpp

- Switched internal `String` parameters to `const String&` where safe, reducing heap copies in the per-character asset-spelling path.
- Added `lastInterruptPin` global (`-1` = no interrupt). Reset at the start of every `playMP3()` call; set when a debounced interrupt fires.
- Added debounce to mid-playback button interrupts (`kInterruptDebounceMs = 20`) — a control button must read "pressed" continuously for 20ms before it's treated as a deliberate interrupt, instead of firing on a single raw read (bounce protection).
- Added `openAndValidateMp3()` so `isValidMp3()` and `playMP3()` share one file-open/validate step instead of each independently reopening and re-statting the same file.
- Added `configureI2SOnce()` — I2S pinout/gain is now configured once instead of being reset on every `playMP3()` call.
- Declared the control-button interrupt pin list once (`kInterruptPins`) instead of redeclaring it in two functions.
- Collapsed the duplicated fallback branching in `speakPhrase()`/`speakText()` into a single shared `speakWithFallback()` helper.
- Left the dead network-fetch stubs (`vrssFetch`, `audioFetchTask`, `_safeQueue`) in place, since other files still call them, but clarified in comments that they are offline-mode no-ops rather than working code.

## buttons.h

- Removed stray `isValidMp3`/`playMP3` redeclarations that conflicted with the corrected `mp3_player.h` signatures.
- Added `speakNoteTitle(int index)`, replacing six duplicated "resolve title mp3, or fall back to SAM + queue" blocks across `handleMainMenu`, `handleReadNotes` (x2), `handleViewNote`, `handleDeleteConfirm` (x2).
- **Bug fix:** `speakNoteTitle()` is now interrupt-aware. If a title clip is cut short specifically by `BTN_DOWN`, it advances to the next note and immediately plays its title, chaining for as long as DOWN keeps interrupting — previously, `playMP3()`'s internal loop blocked `loop()` entirely, so a DOWN press mid-title stopped the clip but nothing advanced, producing silence until the next `loop()` pass picked up the button.

## braille.h

- Fixed dead double-tap logic caused by `UNDO_DBLCLICK_MS` being set to `0` (the double-tap branch could never fire). BACK button now has three distinct outcomes:
    - Single tap → delete last character
    - Two taps spaced 150–600ms apart → clear current word buffer
    - Long press (650ms, unchanged threshold) → exit the Braille pad **and save as a draft** (previously discarded unsaved progress)
- Added `brailleResetBuffer()` and `brailleResetGestureState()` helpers, replacing repeated inline reset field-lists at six call sites (`exitBraillePadWithoutSaving`, new `brailleExitAndSaveDraft`, `brailleConfirmTitle`, `brailleSaveNote`, `brailleClearBuffer`, and the new-note entry point in `buttons.h`).
- Removed the second, hand-maintained copy of the Braille letter decoder that previously lived in `display.h`; `display.h` now calls this file's real `decodeBraille()` via forward declaration (no `#include` in either direction between the two files — see `display.h` notes below).
- Added explicit forward declarations for `drawBraillePad()`/`drawToast()` (both defined in `display.h`) instead of silently relying on `.ino` include order, which is what broke when `display.h` needed a symbol from this file too.
- Fixed a duplicate-function-definition regression introduced mid-session (two copies each of `brailleResetBuffer`/`brailleResetGestureState`, and a use-before-definition of `brailleClearCell()`) before it was delivered.

## display.h

- Removed `_dispDecodeBraille`/`_dispDecodeLetter`, a duplicate of the real Braille decoder in `braille.h` that had to be manually kept in sync.
- The live typing preview in `drawBraillePad()` now calls the real `decodeBraille()`, on local copies of `capitalNext`/`numberMode` so the preview cannot mutate actual typing state.
- **Post-delivery fix:** an initial version of this change added a full `#include "braille.h"` to pull in the decoder. That created a circular dependency — `braille.h` calls `drawBraillePad()`/`drawToast()`, which are defined in `display.h`, so whichever header got included first left the other's symbols undeclared at the point they were used, causing build failures (`'drawBraillePad' was not declared in this scope`, then `'BRAILLE_CAPITAL_IND' was not declared`). Fixed by using forward declarations in both directions instead of a real `#include`, and by moving the shared `BRAILLE_CAPITAL_IND`/`BRAILLE_NUMBER_IND` macros into `globals.h`, which both files already include independently. Neither header now includes the other.

## globals.h

- Replaced the dead `UNDO_DBLCLICK_MS 0` constant with `UNDO_DBLTAP_MIN_MS` (150) and `UNDO_DBLTAP_MAX_MS` (600), defining a real spaced-double-tap window.
- Changed `getNoteTitle()`/`getNoteBody()` to reference `noteList[i]` instead of copying the full note string before slicing out the title/body, removing one redundant allocation per call.
- **Post-delivery fix:** added `BRAILLE_CAPITAL_IND`/`BRAILLE_NUMBER_IND` here (moved from `braille.h`) so both `braille.h` and `display.h` can use them without needing to include each other.

## main.cpp

- Deduplicated the filesystem info dump: the recursive directory-listing lambda and the usage-stats block were previously copy-pasted in both `setup()` and the `"fs"` serial console command. Consolidated into `dumpFilesystemInfo()` / `printFsTree()`, called from both places.
- Removed the commented-out, non-functional WiFi bring-up block, replacing it with a short explanatory comment rather than leaving dead code in place.

---

## mp3_player.cpp (session 2)

- **Bug fix:** `speakTextFromAssets()` (spells/speaks text word-by-word or letter-by-letter across multiple `playMP3()` calls) now checks for a BACK interrupt after every clip via a new `wasBackInterrupt()` helper, and stops the entire sequence immediately instead of continuing to the next word/letter. Previously, a BACK press mid-sequence stopped only the current clip; the sequence rolled on to the next one regardless, which read as "audio pauses for a moment then continues."
- `spellWithAssets()` changed from `void` to `bool` return, reporting whether it was cut short by BACK so its caller can stop the outer sequence too.

## braille.h (session 2)

- **Bug fix:** added `speakInterruptibleAndProcessBack()`, used by `speakUndoResult()` and `brailleUndoWord()`'s readback. Stopping a multi-clip speak sequence on BACK (see `mp3_player.cpp` above) only solved half the problem — BACK's actual tap/double-tap/long-press logic (`handleBackButton`) lives in `handleBraillePad()`, which never runs while blocked inside `playMP3()`. This wrapper checks BACK's live state immediately after the interrupted speech returns and calls `handleBackButton()` directly, so a BACK press during a readback now actually deletes/clears/exits, instead of only silencing the audio.
- Removed `brailleReadBuffer()` and its `BTN_REREAD` binding in `handleBraillePad()` — "read everything typed so far" while actively typing is gone, per request. `BTN_REREAD` itself is unchanged elsewhere (`buttons.h` still uses it for re-reading a note's title/body and the AI-improved result on other screens) — only the typing-screen behavior was removed.

## New feature: note audio chains (session 3)

Previously, note bodies had no working audio path at all — the intended "note.mp3" cache was never produced by any code, so opening a note either fell back to reading only the title, or (during typing/scrolling) to slow live SAM/asset resolution. Replaced with a pre-built playback plan, resolved once at save time.

- **`globals.h`**: removed `noteMp3Path()` (referenced a file format nothing ever produced). Added `noteChainPath(int i)`.
- **`mp3_player.h`/`.cpp`**: added `buildNoteAudioChain(index, body)`, `playNoteAudioChain(index)`, `noteAudioChainExists(index)`.
    - Chain format: one line per unit — `W:<stem>` (whole-word asset), `L:<letter>` (single-character spelling fallback), or `SP` (space/pause). Built by walking the note body once, resolving each word against the SD card's asset library via the same `wordAssetName()` lowercasing/punctuation-stripping already used elsewhere (so `Poverty` and `poverty` resolve identically).
    - Playback walks the chain file line-by-line, stops immediately if BACK interrupts (via the existing `wasBackInterrupt()` check), rather than continuing to the next line.
    - Removed the old `queueNoteAudio()` stub entirely (it referenced the now-removed `noteMp3Path`, and never worked). `queueNoteTitleAudio()` — a separate, unrelated concern (title caching) — is unchanged.
- **`braille.h`**: `brailleSaveNote()` now calls `buildNoteAudioChain()` right after saving, so every note gets a working chain the moment it's created.
- **`buttons.h`**:
    - New `speakNoteBody(index)` helper plays a note's chain on entry to `STATE_VIEW_NOTE` (from the list, or via REREAD), falling back to title-only if no chain exists yet (older notes).
    - `handleViewNote`'s DOWN/scroll no longer re-reads text line-by-line — the chain already reads the whole note once on entry, so DOWN is purely visual scrolling now.
    - REREAD and VIEW-entry now behave identically (previously they had two different, inconsistent no-MP3 fallback behaviors).
    - `handleAIPreview`'s save-edit path rebuilds the chain with the new (AI-improved) body text, since the words actually changed.
    - BACK now correctly returns to the note list when it interrupts REREAD's chain playback, instead of only silencing audio — same class of fix as the typing-screen BACK issue from session 2, applied here.

### Known limitation, flagged not fixed

`removeNoteAudioFiles()` (called on note delete and on AI-edit resave) is defined in `storage.cpp`/`storage.h`, which were not provided. It's not known whether that function has been updated to clean up `note{N}_chain.txt` files, or whether it still only targets the old `.mp3` naming — worth checking directly, since an out-of-date version could leave orphaned chain files on the SD card after a note is deleted or edited.

### storage.h — fix (2026-07-15)

- **Logged by:** GitHub Copilot

- **Logged at:** 2026-07-15

- **Files changed:** `include/storage.h`

- **What was there before:** `storage.h` referenced the removed `noteMp3Path()` identifier and contained a small accidental insertion of stray lines which caused IntelliSense errors; it also used forward-declarations instead of including the MP3 API header.

- **Changes made:**

    - Included `mp3_player.h` for audio/chain declarations.
    - Replaced `noteMp3Path(...)` usages with `noteChainPath(...)` where files are removed.
    - Replaced body-MP3 validity checks with `noteAudioChainExists(...)` since body audio is now a chain file.
    - Updated `removeNoteAudioFiles()` to remove chain files and to check `sdReady` before SD removals.
    - Removed stray accidental lines from `verifySdNotesFolder()` so probe write/read works correctly.
- **Why:** the project moved from a never-produced single `note.mp3` file to pre-built note audio chains; `storage.h` needed to be updated to match `globals.h` and `mp3_player.*` APIs to resolve undefined identifier errors and to properly manage chain files.


## Note audio chain migration (session 4 — 2026-07-15)

Notes created **before** the chain feature existed have a `.txt` file on disk but no `_chain.txt`. `speakNoteBody()` (buttons.h) then silently falls back to reading only the title, so those old notes could never speak their body. There was no migration path — a chain was only ever built at save time (braille.h) or on AI-improve (buttons.h).

- **Files changed:** `include/storage.h`
- **Sections changed:**
    - Added `ensureNoteAudioChains()` — iterates **real file slots** `0..MAX_NOTES` (not the compacted `noteList`, so it stays correct even if note slots have gaps). For each slot that exists but has no chain (`noteAudioChainExists(i)` is false), it reads the note body and calls `buildNoteAudioChain(i, body)`. Title-only notes (empty body) get an empty chain written so the slot is marked done and won't be retried on every boot. Already-chained notes are skipped — no rebuild, no file churn.
    - `loadNotes()` now calls `ensureNoteAudioChains()` right after loading notes and before the existing title/body status log, so the migration runs automatically on every boot. Seed notes (written by `writeSeedNotes()`) are covered too, since they're chained on the first `loadNotes()` after they're created.
- **Why:** ensures **every** note present in the note list gets a body audio chain, including pre-existing ones, instead of only notes saved/AI-improved after the feature shipped. Idempotent and cheap (bounded by `MAX_NOTES`), so running it every boot is safe.
- **No change needed in:** `mp3_player.cpp`/`.h` (existing `buildNoteAudioChain`/`noteAudioChainExists` already do the work), `braille.h`, `buttons.h`.

## Note title audio chains (session 5 — 2026-07-15)

Note titles were read by probing a `note{N}_title.mp3` that the offline build never generates, then falling back to live asset/SAM spelling of the title. That's the same problem the body chain solved, so titles now use the identical prebuilt-chain mechanism.

- **Files changed:** `include/globals.h`, `include/mp3_player.h`, `src/mp3_player.cpp`, `include/storage.h`, `include/braille.h`, `include/buttons.h`, `log files/log.v.2.md`
- **Sections changed:**
    - **`globals.h`**: added `noteTitleChainPath(int i)` (`/notes/note{N}_title_chain.txt`). Removed the now-unused `noteTitleMp3Path()` (the file it pointed at is never produced).
    - **`mp3_player.cpp`**: refactored the chain build/play logic into shared static helpers `buildChainToFile(path, text)` and `playChainAtPath(path)`; `buildNoteAudioChain`/`playNoteAudioChain` and the new `buildNoteTitleChain`/`playNoteTitleChain` are now thin wrappers that pass the right path. Added `noteTitleChainExists()`. Removed the dead `queueNoteTitleAudio()` stub (it only ever queued the nonexistent title MP3).
    - **`mp3_player.h`**: declared `buildNoteTitleChain`/`playNoteTitleChain`/`noteTitleChainExists`; removed `queueNoteTitleAudio` declaration.
    - **`storage.h`**: `ensureNoteAudioChains()` now also builds a missing title chain per note (same boot migration that covers bodies), reading both title and body from the one note file. `loadNotes()` status log reports the title *chain* (`noteTitleChainExists`) instead of probing `title.mp3`, which also kills the noisy `[E] vfs_api.cpp ... note{N}_title.mp3 does not exist` lines at boot. `removeNoteAudioFiles()` now deletes the title chain too.
    - **`braille.h`**: `brailleSaveNote()` builds the title chain (`buildNoteTitleChain(slot, pendingTitle)`) right after saving, so new notes get both chains immediately.
    - **`buttons.h`**: `speakNoteTitle()` now plays the title chain (`noteTitleChainExists` → `playNoteTitleChain`), building it on the fly only if a migration hasn't produced one yet, and falling back to live spelling only if storage is unwritable. DOWN-chaining behavior preserved (chain playback is interrupt-aware via `playMP3`).
- **Why:** titles "Grace" etc. now resolve against the recorded asset library (whole-word `grace.mp3` if present, else letter-by-letter) using the same saved plan as bodies — no dependency on a never-generated MP3, and the body/title audio paths are now uniform.
- **No behavior change for:** `speakNoteBody()` (still body-chain based), the chain file format (`W:`/`L:`/`SP`), or the offline/SAM gating.

## Audio bugs fixed + asset diagnostics (session 6 — 2026-07-15)

Reported: a note titled/body "Grace" was spelled out letter-by-letter even though `grace.mp3` was present, and word boundaries had no audible pause.

- **Files changed:** `src/mp3_player.cpp`, `include/storage.h`, `src/main.cpp`, `log files/log.v.2.md`
- **Bug 1 — stale cached chains (why "Grace" stayed spelled).** `ensureNoteAudioChains()` in `storage.h` previously *skipped* any note that already had a chain file. So a chain built before `grace.mp3` existed was frozen forever — adding the asset later did nothing. Fixed: chains are now **rebuilt every boot** for every note (body + title), so they always reflect the current asset library. Cost is negligible (≤ `MAX_NOTES` tiny files).
- **Bug 2 — `SP` produced no pause.** In `playChainUnit()` (`mp3_player.cpp`), `SP` only played `space.mp3`; with no such asset it played *nothing*, so words ran together. Fixed: `SP` now plays `space.mp3` if present, otherwise waits `kSpacePauseMs` (300 ms) — a real word gap either way.
    - **Bug 3 — resolver searched the wrong folders (root cause of the reported symptom).** `resolveAudioAsset()` only searched `/data/en`+`/sfx/en` (SD then LittleFS). Confirmed in `changes.txt` §6 that the canonical word-audio location is `/words/<lang>/<word>.mp3` on the SD card (flat, one file per word), and the user confirmed `/words/en` is what their card uses. So `grace.mp3` etc. were never found and every word was spelled out. Fixed: `audioLangFolder()` now returns `/words/<lang>`; `resolveAudioAsset()` searches, in priority order, `/words/<lang>` (SD, canonical) → `/voice/<lang>` (alternate SD naming) → `/sfx/<lang>` (legacy LittleFS letter/system assets) → `/data/<lang>` (legacy fallback), each on SD then LittleFS. With the every-boot rebuild (Bug 1), chains now pick up `W:grace` automatically once the clip is present.
    - **Diagnostics.** Added `diagAudioAssets()` in `main.cpp`, called once at boot and via the serial `assets` command. It lists `/words/en`, `/voice/en`, `/sfx/en`, `/data/en` (SD + LittleFS) and probes `grace`/`poverty`/`space`/`a`, printing `FOUND`/`MISSING`. Now also lists `/words/en`+`/voice/en` so the real word-audio folder shows up directly. Added `assets` to the `help` listing.
    - **Refinement — note audio is SD-only (not LittleFS).** Per the user's standalone SD voice-player sketch (which defines `VOICE_DIR "/words/en"` and plays entirely from SD), note speech must come from the SD card, not LittleFS. `fileExistsReadOnly()` (the only consumer of which is `resolveAudioAsset()`) now opens SD only and drops its LittleFS branch, so note bodies/titles and letter-spelling resolve from SD `/words/<lang>` etc. The `littleFsOnly` flag is kept for API compatibility but ignored. System/menu prompts (`new_note.mp3`…) remain a separate path resolved by `sfxPath()` against LittleFS `/sfx/<lang>` and are intentionally unaffected. With no SD mounted, note audio resolves to nothing and the existing SAM fallback engages.

## Known open items (not yet done)

- No shared debounce helper yet for the repeated `if (millis()-lastDebounce<DEBOUNCE_MS) return;` pattern across all six `handleX()` state handlers in `buttons.h` (originally flagged as item #12 in the review).
- `noteList[noteCount]` / `history[histLen]` bounds-checking at write sites was flagged during review but not directly addressed in these files, as the relevant write sites are believed to live in `storage.cpp`, which was not provided.
- `globals.h` continues to mix hardware pin config, global state declarations, and note-parsing business logic in one file — a larger structural split (`pins.h` / `state.h` / `notes_util.h`) was discussed but not carried out, by design, to keep this pass scoped to the files provided.

## Boot-time stall + robotic playback fixes (session 7)

**Session start:** 2026-07-15 21:13 UTC **Session end:** 2026-07-15 21:36 UTC **Logged by:** Sam Claude

Reported via `test2_v2.txt`: from main-menu SELECT to `[STATE] Entering READ_NOTES` actually appearing took **~3+ minutes** on a 3-note / ~60-word note set, plus dozens of `[E][vfs_api.cpp]` "does not exist" lines cluttering the log, plus word-by-word note playback that "didn't feel nice" — flat, evenly-spaced, robotic.

- **Root cause 1 (main stall): `ensureNoteAudioChains()` rebuilt every note's chain on every boot, unconditionally.** Each word in each note pays up to 8 blocking filesystem probes in `resolveAudioAsset()` (4 candidate folders × 2 filesystems) during chain building. For the seeded "Grace" note (~60 words) that's hundreds of blocking SD/LittleFS calls before the note list is even usable — every single boot, even when nothing had changed since the last one.
    - **Fix (`storage.h`):** `ensureNoteAudioChains()` now only builds a chain for a note that doesn't have one yet (new note / first-run migration). Existing chains are left alone. Added an explicit `force` parameter for the one case that legitimately needs a full rebuild: you added new word clips to the SD card after a note was already chained. Trigger that manually via the new `rechain` serial command instead of paying the cost on every boot.
- **Root cause 2 (log spam, minor blocking cost): `diagAudioAssets()` ran unconditionally in `setup()`.** It's a pure diagnostic dump (6 folder listings + several word probes), each probe a real blocking file-open call, each miss printing a Serial line — dozens of them, on every boot, for no runtime benefit.
    - **Fix (`main.cpp`):** removed the boot-time call. Still available on demand via the existing `assets` serial command.
    - **Added (`main.cpp`):** new `rechain` serial command — force-rebuilds all note chains on demand (pairs with the "I added new SD assets" case above). Added to `help` listing.
- **Root cause 3 (robotic pacing): every word boundary got the identical fixed 300ms pause, regardless of whether it was mid-sentence or between sentences** — reads as a metronome, not speech.
    - **Fix (`mp3_player.cpp`):** shortened the ordinary inter-word pause (`kSpacePauseMs`, 300ms → 120ms) and added a distinct, longer pause after sentence-ending punctuation (`kSentencePauseMs`, 260ms) via a new `SPS` chain unit. `buildChainToFile()` now detects `.`/`!`/`?` at the end of a token and writes `SPS` instead of plain `SP` there; `playChainUnit()` handles both. Net effect: words within a sentence run closer together, sentences get a real breath between them — same idea as punctuation-aware TTS pacing, done entirely at chain-build time so it costs nothing extra at playback.
    - **Also trimmed (`mp3_player.cpp`):** removed an unconditional 30ms delay in `playMP3()` that fired before every single clip regardless of state, and removed the per-word `[MP3] Validate ...` Serial print in `openAndValidateMp3()` (kept the failure-path logging). Across a 60-word note that's dozens of avoided Serial writes and small delays — not the dominant cost, but it adds up and was free to remove.

### Not changed / still open

- The per-word clip playback duration itself (e.g. a 3.6KB `a.mp3` taking over a second to play in the original log) looks disproportionate to file size and may point to I2S DMA buffer sizing or SD-read throughput under `AudioFileSourceSD`, not to anything fixed in this session. Worth profiling directly (e.g. time just `_mp3.loop()` iterations) if playback still feels slow per-clip after this pass — this session only removed the *setup/rebuild* overhead around each clip, not the clip-decode path itself.
- `SPS` is a new chain-file format addition. Any chain files already on the SD card from before this change only contain `SP`/`W:`/`L:` lines and will keep working (no crash — `playChainUnit()` still handles plain `SP`), they just won't get the new sentence-pause behavior until re-chained (`rechain` command, or re-save the note).

## Silent notes/titles fix + single-directory resolver + pure-silence pacing (session 8)

**Session start:** 2026-07-16 06:44 UTC **Session end:** 2026-07-16 07:01 UTC **Logged by:** Sam Claude

Reported: after session 7, notes and titles stopped being read at all — total silence, but no errors, and the log showed chain playback finishing in a few hundred ms with zero `AUDIO MP3 START` lines. Also reported: a title chain visibly containing `W:grace` still wasn't playing; requested collapsing the word-audio search to the single real directory instead of checking several; requested that SP be genuine silence (no clip lookup) rather than a fixed delay stacked on top of a failed search.

- **Root cause (silent playback): a success-counting bug in `playChainAtPath()`/`playChainUnit()` — not something introduced by Claude in session 7, but present in the code by the time this session started.** Pause lines (`SP`/`SPS`) were being counted as "this chain played successfully," including when *every* real word/letter lookup in the chain failed. A chain that was 100% failed word-lookups plus pauses would still report success, so callers (`speakNoteTitle`/`speakNoteBody` in `buttons.h`) never triggered their fallback — the result is a chain that "finishes" in log terms while producing literally no audio, matching the reported symptom exactly (fast completion, no MP3 start lines, no errors).
    - **Fix (`mp3_player.cpp`):** `playChainUnit()` now reports success separately from "produced audible output" via an out-parameter (`producedAudio`). Only `W:`/`L:` lines that actually found and played a real clip count toward the chain's overall success. `playChainAtPath()` now returns true only if at least one real audio unit played, not merely if any line (including a silent pause) was processed.
- **Root cause (title with `W:grace` still silent): chain "exists" only meant "the file is present," not "the file has real content."** A chain file could be empty, truncated, or otherwise broken and still pass the exists-check forever, since `ensureNoteAudioChains()` only rebuilds chains that are *missing*, not ones that are present-but-useless.
    - **Fix (`mp3_player.cpp`):** `noteAudioChainExists()`/`noteTitleChainExists()` now open the file and confirm it contains at least one real `W:`/`L:` line before reporting "exists." An empty or word-less chain now correctly reads as "not ready" and gets rebuilt automatically.
    - **Guard added (`storage.h`):** a genuinely empty note (blank title or body — nothing typed) legitimately produces a chain with zero word lines. That's correct, not broken, so `ensureNoteAudioChains()` now only expects word content for titles/bodies that actually have text, and skips the empty ones — otherwise every boot would try (and "fail") to rebuild an intentionally-empty chain forever.
- **Single-directory resolver, as requested.** `resolveAudioAsset()` previously searched up to 4 folders (`/words`, `/voice`, `/sfx`, `/data`) × 2 filesystems per word. Confirmed all real word audio lives at `/words/<lang>/<word>.mp3` and nowhere else, so the other three search locations were pure overhead with no upside — removed. `audioLangFolder()` is now the only lookup path; `legacySfxFolder()` and the `littleFsOnly` parameter threading (no longer meaningful with one directory) were removed from `resolveAudioAsset()`, `fileExistsReadOnly()`, and `canSpellFromAssets()`, with call sites (`samAllowedFor()`) updated to match.
- **Pure-silence pacing, as requested.** `SP`/`SPS` no longer search for a `space.mp3` clip before falling back to a delay — confirmed no such clip exists on the card, so that lookup was a guaranteed-miss search paid on every single word boundary in every note, for no benefit. They're now plain timed silence: `kSpacePauseMs` (120ms) between words in the same sentence, `kSentencePauseMs` (260ms) after `.`/`!`/`?`.

### Not done in this session (explicitly deferred, not silently skipped)

- Using actual measured lookup/read latency as the pause duration (rather than a fixed timed silence) was requested as an idea ("leverage the time it takes to search for the audio... to come up with natural flowing audio") but not implemented — with the resolver now down to one directory and a single fast file check, there's no longer a meaningful chunk of search time to repurpose as pacing. If the intent was specifically to make pause length adapt to real SD/read latency rather than use a fixed value, that's a distinct feature and wasn't built here; flagging instead of guessing at it.
- The remaining `resolveAudioAsset("space")` lookups in the LIVE (non-chain) speech path — `canSpellFromAssets()` and `speakCharacter()` in `mp3_player.cpp`, used for menu prompts and un-chained fallback speech — were left unchanged. They're a separate code path from note chains and weren't part of what was asked this session.

## Word-clip lookup latency: index, prefetch, numeral fix (session 9)

**Session start:** not independently captured at the top of this turn (best lower bound: after session 8 ended, 2026-07-16 07:01 UTC) — will capture precisely going forward per standing instruction. **Session end:** 2026-07-16 18:19 UTC **Logged by:** Sam Claude

Reported (with a fresh boot log): individual word clips during note playback still show multi-second gaps — same file, same latency every time it's opened (not cold-cache-related), unrelated to file size. Requested, in priority order: (1) an in-memory index built at boot so lookups don't re-scan the SD card, (2) a prefetch/lookahead mechanism to hide search time behind playback time, (3) an evaluation of whether restructuring the SD folder layout (per-letter subfolders, or a single indexed blob) would be more robust long-term, (4) a fix for why lookups are attempted at all for files that don't exist (numeral pronunciations), fixed upstream in the word-to-filename mapping.

**Evidence pulled directly from the provided boot log**, to confirm the diagnosis before changing anything:

| File | Open latency | Total duration | Playback-only |
|---|---|---|---|
| grace.mp3 (1st play) | 3785ms | 5260ms | ~1475ms |
| grace.mp3 (2nd play) | 3778ms | 5253ms | ~1475ms |
| a.mp3 (x3 plays) | 35-36ms every time | ~1222ms | ~1187ms |
| m.mp3 (1st play) | 4358ms | 5657ms | ~1299ms |
| m.mp3 (2nd play) | 4360ms | 5658ms | ~1298ms |
| z.mp3 | 6448ms | 7707ms | ~1259ms |

This confirms the hypothesis cleanly: latency is per-file and repeatable (not cold/warm cache — repeat plays of the identical file cost the identical time), and actual playback duration is consistent (~1.2-1.5s) regardless of word. All the variance is in locating the file, not playing it — consistent with FAT directory-entry/chain position, not file size.

### 1. In-memory word asset index (`mp3_player.cpp`, `mp3_player.h`, `main.cpp`, `buttons.h`)

- Added `buildWordAssetIndex()` — scans `/words/<lang>` once via `openNextFile()`, stores every filename (lowercased, no extension) in a sorted `std::vector<String>`, looked up afterward via `std::binary_search` — zero SD access per lookup.
- `resolveAudioAsset()` now consults the index first: a known-missing word costs nothing (no SD call at all), and a known-present word skips the old redundant existence-probe SD call entirely (the real read still happens once, later, when the clip actually plays). Falls back to the original real-SD-probe behavior if the index failed to build (e.g. no SD at boot).
- Called once at boot in `main.cpp` (after `loadConfig()`, since the index is per-language and depends on `langEnglish`), and rebuilt on language change in `buttons.h`'s `handleLangSelect()`. New `reindex` serial command rebuilds it on demand (e.g. after copying new word clips onto the card) — paired with the existing `rechain` command in the `help` listing.
- **What this does NOT fix:** the actual per-file open cost for files that DO exist, since the plain Arduino `SD.h` API doesn't expose a way to cache a direct "go straight there" handle the way raw SdFat's indexed-open does. That's what items 2 and 3 below address.

### 2. Lookahead prefetch for chain playback (`mp3_player.cpp`)

- Word clips are consistently small (3.6-8.9KB observed) — small enough to hold entirely in RAM. Chain playback (`playChainAtPath`) now reads the whole chain file into memory as a line list up front, and before playing each unit, scans ahead (past any `SP`/`SPS`) to find the next real word/letter and kicks off a background load of that file into a RAM buffer — on a FreeRTOS task pinned to the other core — while the current unit plays. By the time playback reaches that next word, it's usually already sitting in RAM.
- Added `AudioFileSourceRAM`, a minimal `AudioFileSource` subclass that feeds the MP3 decoder from a RAM buffer instead of streaming from SD. This also has a secondary benefit: since playback is now RAM-backed rather than streamed from SD, the SD/SPI bus is genuinely free during the ~1.2-1.5s playback window — which is what makes concurrent background prefetching safe rather than contending with the current clip's own reads.
- A real FreeRTOS mutex guards the handoff between the background task and the main task (not just a bare `volatile bool` flag, which isn't a real memory barrier across cores) — closes a narrow race window between the background task finishing a write and the foreground task reading/swapping buffers.
- **Feature-flagged** behind `#define ENABLE_WORD_CLIP_PREFETCH` (default on). The `AudioFileSourceRAM` class mirrors the `AudioFileSource` virtual interface already used elsewhere in this exact file (matching the `read()`/`seek()`/`getSize()`/`isOpen()`/`close()` call shapes visible in `openAndValidateMp3()`), but **has not been verified against the actual installed ESP8266Audio library source** — I don't have that header available to check exact signatures. If it doesn't compile or misbehaves on real hardware, set the flag to 0 to instantly revert to the old, proven streaming-from-SD path with no other changes needed.
- **Honest limit on what this fixes:** prefetch hides latency up to roughly one word's playback window (~1.2-1.5s). Most observed latencies (2000-4700ms) benefit a lot — the wait after playback ends shrinks to zero or near-zero. The worst outliers (e.g. z.mp3 at 6448ms) still produce *some* residual wait even with one-word lookahead, since the fetch needs longer than one playback window completes. A deeper lookahead queue (2-3 words) or fixing the underlying open-cost (item 3) would be needed to close that remaining gap fully.

### 3. Folder restructuring — evaluated, not implemented (recommendation only)

Two options were considered for a more robust long-term architecture:

- **Per-letter subfolders** (`/words/en/a/`, `/words/en/b/`, ...): reduces how many directory entries a lookup has to walk through in any single folder, which helps as vocabulary grows, but doesn't eliminate the underlying position-dependent cost — a large per-letter folder (e.g. many `s`-words) would still show the same symptom at smaller scale.
- **Single indexed blob** (`/words/en/bank.bin` + `/words/en/bank.idx`, all word clips concatenated with an offset/length index): this is the stronger long-term answer. One file open at boot, then every word lookup becomes a seek-by-offset instead of a filename search — eliminates the per-word FAT-position cost entirely, regardless of vocabulary size, and trivially supports prefetching (reading a known byte range instead of resolving a name). Recommended if vocabulary is expected to grow significantly or if the residual prefetch-miss gaps from item 2 remain noticeable.
- **Not implemented this session** because it's a genuinely separate deliverable: it needs a companion PC-side packing tool (to build `bank.bin`/`bank.idx` from the current per-word `.mp3` files), a versioned chain-file format change (word name → byte offset instead of filename), and a migration path for chains already on deployed cards. Flagged here as the recommended next step if the prefetch approach in item 2 doesn't fully resolve the remaining gaps in practice — happy to build it as a follow-up.

### 4. Numeral (and any missing-character) lookups fixed upstream (`mp3_player.cpp`)

- Root cause: `buildChainToFile()` spelled out any word with no whole-word clip letter-by-letter, writing an `L:<char>` chain line for **every** character unconditionally — including digits. This SD card has letter clips (a-z) but no digit clips (0-9), so any note containing a number (e.g. "18") produced `L:1`/`L:8` chain lines that were **guaranteed to fail on every future playback, forever** — a doomed SD lookup repeated every single time that note was read, producing silence for that character with no indication why.
- Fixed at the source: `buildChainToFile()` now checks (via the index) whether each character's clip actually exists **before** writing an `L:` line for it. A character with no clip is skipped (silent for just that character, same convention used elsewhere for "asset missing") and reported **once**, at build time, via a single aggregate log line (e.g. `[CHAIN] .../note0_chain.txt — 2 character(s) have no audio clip and will be silent: 1 8`) — not rediscovered as a failed SD open on every future read.
- This is a real content gap, not just a performance one: numbers in notes will be silent (not spelled, not spoken any other way) until digit clips (`0.mp3`-`9.mp3`) are added to the card and the affected notes are re-chained (`rechain` command). Flagging this plainly rather than treating "faster silence" as equivalent to "fixed."

## Design discussion: address-resolved soundbank proposal (between sessions 9 and 10)

**Session start:** 2026-07-16 (continuation of prior analysis; exact clock start not tool-logged) **Session end:** 2026-07-16 **Logged by:** kdprince claude

This session was analysis and design discussion only — no source files were changed. Logged per standing instruction to record every session, code-related or not.

### Starting evidence

Boot logs (`test4.txt`, a pre-session-6/7/8 capture) showed per-word SD lookup gaps (time between `AUDIO MP3 START` and `AUDIO MP3 source SD`) ranging from ~35ms (`a.mp3`) to ~6,448ms (`z.mp3`) for files of comparable size, plus two ~2,500ms wasted lookups for non-existent files (`1.mp3`, `8.mp3`). Averaged across a 10-word sample (excluding the anomalous `a.mp3`): ~3.84 seconds per word. This was cross-checked against the actual session-8 source (`mp3_player.cpp` as pasted into this conversation, not independently re-fetched) rather than assumed.

### Root cause, confirmed against actual code (not guessed)

`resolveAudioAsset()` does a single `SD.open()`-based existence check per word (via `fileExistsReadOnly()`) — it is not a multi-candidate folder loop; the single-directory fix from session 8 is real and correct. The remaining cost sits **underneath** this function, in how the SD library / FAT filesystem locates a file by name: FAT32 stores directory entries in write order, not alphabetically or indexed, so `SD.open("grace.mp3")` must linearly scan every entry written before it. Files added early (`a.mp3`) resolve fast; files added later resolve slower, and the gap grows with directory size, not file size — matching the measured pattern.

Additionally identified, directly from the pasted `mp3_player.cpp`: **playing one word currently pays three separate filename-based SD lookups**, not one — `fileExistsReadOnly()` inside `resolveAudioAsset()`, `SD.exists()` inside `openAndValidateMp3()`, and the open inside `AudioFileSourceSD`'s constructor. This is a compounding inefficiency independent of the directory-scan issue above.

### Proposed action (not yet implemented — pending review)

A compound, multi-layer fix, deliberately built so each layer degrades safely to current behavior rather than introducing new failure modes:

1. **PC-built soundbank.** One combined file `/words/en/bank.bin` (all word clips concatenated) plus `/words/en/bank.idx` (word → byte offset + length), built by a PC-side script and manually copied to the SD card whenever new word audio is added — explicitly *not* built on-device, to avoid power-loss-mid-write corruption risk on the ESP32 itself. User has confirmed manual PC-side rebuild + SD copy is an acceptable workflow.
2. **RAM index at boot.** `bank.idx` loaded once into memory at boot (small, one sequential read). All later word lookups become in-RAM table lookups instead of SD filename searches — collapses the three-lookups-per-word cost above to zero SD searches, one seek+read for the actual audio.
3. **Offset-resolved chains.** `buildChainToFile()` (used by both note bodies and titles) writes resolved `offset,length` pairs into chain lines instead of symbolic `W:<word>` names, so the lookup cost is paid once at note-save time, not on every future playback/re-read of that note.
4. **`rechain` updated to use the same RAM index**, rather than re-resolving by filename, so adding new words becomes: copy new `bank.bin`/`bank.idx` → reboot (reloads index) → run `rechain` → all chains refreshed from RAM lookups only.
5. **Old per-file resolver kept as fallback**, untouched, for if `bank.bin`/`bank.idx` are missing, outdated, or fail validation — device degrades to current (slower but safe) behavior rather than failing silently or playing wrong audio.

### Known open risks, flagged not resolved

- No code path currently exists (in the files reviewed) that keeps `bank.bin` contents in sync with `/words/en/*.mp3` if individual word clips are later replaced or removed — this needs explicit design, not an assumption that existing storage logic covers it.
- Changing the chain file format (name → offset) is a breaking change for any chain files already on deployed SD cards; needs either a version marker in the chain format or a forced full `rechain` after rollout.
- MP3 decoding from an arbitrary byte range inside a larger concatenated file requires the audio source layer to treat that byte range as a self-contained stream (a "ranged" file source) — flagged as a real implementation detail to verify against the ESP8266Audio library's actual seek/sub-range behavior, not assumed to work automatically from concatenation alone.
- The per-clip *decode* duration itself (already flagged as "not changed / still open" in session 7) is expected to become the next visible bottleneck once lookup cost is removed, and was not addressed by this proposal.

### Status

Design discussion only. No code written or changed in this session. Awaiting decision to proceed to implementation.

— kdprince claude

## Word bank (packed blob storage) implementation (session 10)

**Session start:** 2026-07-17 06:51 UTC **Session end:** 2026-07-18 06:35 UTC **Logged by:** Sam Claude

Follow-up to session 9's recommendation (evaluated but not built at the time): implement the "single indexed blob" approach as a real accelerator on top of the per-file index and RAM prefetch already in place. Also verified the hand-written `AudioFileSourceRAM` class from session 9 against the actual `AudioFileSource.h`/`AudioFileSourceSD.h`/`AudioFileSourceFS.h`/`AudioFileSourceBuffer.h` headers the user provided — confirmed every overridden method (`read`, `seek`, `close`, `isOpen`, `getSize`, `getPos`) matches the real base class signature exactly (types, argument order, return types all correct). No changes needed there; `AudioFileSourceBuffer` in the real library follows the same "constructed directly, no `open()` override" pattern used for `AudioFileSourceRAM`, confirming that was a reasonable design choice.

### PC-side packer tool (`tools/pack_word_bank.py`)

- New standalone Python script (stdlib only, no dependencies). Run as `python3 pack_word_bank.py <folder>` against a folder of individual word `.mp3` files (e.g. `/words/en`). Produces `bank.bin` (every clip's bytes concatenated back-to-back) and `bank.idx` (plain text, one `word,offset,length` line per clip) in that same folder.
- Skips (with a warning, not an abort) any clip over `MAX_CLIP_BYTES` (16384, matching `WORD_CLIP_MAX_BYTES` in `mp3_player.cpp`) or exactly 0 bytes — those stay as individual files only; the on-device per-file fallback still finds and plays them, just without the bank's speed benefit for that one word.
- Verified end-to-end against synthetic test files during this session: packed 3 valid clips + 1 oversized (correctly skipped), then independently confirmed every `bank.idx` offset/length byte-slices `bank.bin` into something byte-identical to its source file. Not just "should work" — actually checked.
- Deliberately never runs on-device, matching the same reasoning already applied to chain files elsewhere in this project (power-loss-mid-write corruption risk on the ESP32 itself).

### On-device bank support (`mp3_player.cpp`, `mp3_player.h`, `main.cpp`, `buttons.h`)

- Added `loadWordBank()`: reads `/words/<lang>/bank.idx` into a sorted in-RAM `std::vector<WordBankEntry>` (word → offset, length) and opens `/words/<lang>/bank.bin` ONCE, keeping that single `File` handle (`_bankFile`) open for the device's lifetime. A word lookup afterward is `std::lower_bound` (in-RAM, zero SD access) + a `seek()+read()` on the already-open handle — no per-word `open()` at all, which is the actual root cause identified in session 9 (directory-position-dependent open cost, 35ms-6500ms measured on this card, paid on every fresh file open regardless of caching).
- `resolveAudioAsset()` now checks the bank first (fastest path), falling back to the per-file index (session 9), falling back to a raw SD probe if neither is loaded — three layers, each falling back cleanly to the next, never a hard requirement.
- Bank hits return a synthetic `"bank:<word>"` path rather than a real filesystem path. Both `playMP3()` (live speech: menu prompts, un-chained fallback) and `loadMp3ToRam()` (chain prefetch path) recognize and handle this prefix — the bank now accelerates BOTH playback paths, not just note chains. `isValidMp3()` also has a defensive (currently unreachable, since no caller passes it a `resolveAudioAsset()` result today) guard for the same prefix, kept for correctness if that ever changes.
- Extracted `AudioFileSourceRAM` (verified this session, see above) and the guarding mutex out from behind the `ENABLE_WORD_CLIP_PREFETCH` flag, since the bank needs both unconditionally for live speech too, not just chain prefetching. The mutex was upgraded from a plain `SemaphoreHandle_t` to a **recursive** mutex (`xSemaphoreCreateRecursiveMutex`/`Take`/`GiveRecursive`), since bank-file access can now be reached both directly (a single blocking load) and from within an already-locked outer scope (the prefetch task wraps its whole load in the same lock) — a plain mutex would have deadlocked a task against itself in that case.
- `buildWordAssetIndex()` and `loadWordBank()` are both called at boot (after `loadConfig()`, since both are per-language), on language change (`handleLangSelect()` in `buttons.h`), and on demand via the `reindex` serial command (now reloads both, updated `help` text accordingly).
- Updated the `assets` diagnostic (`diagAudioAssets()` in `main.cpp`) to match current reality: it was still listing the four-folder search (`/words`, `/voice`, `/sfx`, `/data`) removed back in session 8 — a stale diagnostic that no longer matched how `resolveAudioAsset()` actually looks things up. Now reports the single real word directory plus bank load status/entry count (new `wordBankIsLoaded()`/`wordBankEntryCount()` accessors) alongside the existing per-file probes.

### What this does and doesn't fix

- **Fixes:** the per-word open cost for any word present in the bank, which becomes a single one-time cost (opening `bank.bin`) at boot instead of a repeated, position-dependent cost on every future playback of that word — no matter how many times a note is re-read.
- **Does not require re-chaining existing notes.** The chain file format (`W:word`, `L:letter`, `SP`, `SPS`) is completely unchanged — `resolveAudioAsset()` is the only place that changed how a word name turns into playable bytes. Existing chains work immediately once a bank is present, no `rechain` needed (though `rechain` still works fine if run).
- **Does not fix words NOT in the bank** — those still go through the per-file index/SD fallback exactly as before. If the user adds a new word clip to the card without repacking the bank, that word plays correctly (via fallback) but doesn't get the bank speed benefit until the next repack + `reindex`.
- **Not hardware-verified.** All of this was written and reasoned through against the real ESP8266Audio headers the user provided and the project's own existing patterns, but has not been compiled/run on the actual device this session. Recommend testing incrementally: first confirm `reindex` reports the expected entry count after copying a packed bank onto the card, then confirm playback, before relying on it.

### Not done this session (explicitly deferred)

- No versioning/migration concern for the bank format itself, unlike the chain format — the bank is regenerated fresh by the packer each time, never hand-edited or carried forward, so there's no old-format compatibility question the way there was for `SP` → `SP`/`SPS`.
- No automatic detection of a bank going stale relative to the individual `.mp3` files it was packed from (e.g. a word clip re-recorded after packing). The user is expected to re-run the packer and `reindex` after any asset change — this matches the existing manual workflow already established for `rechain`.

## Bank file relocation + clip size cap adjustment (session 11)

**Session start:** 2026-07-18 ~06:35 UTC (continuation of session 10, same working session, precise boundary not separately captured — will split cleanly next time per standing instruction) **Session end:** 2026-07-18 13:21 UTC **Logged by:** Sam Claude

Two small, targeted follow-ups requested after session 10's word bank implementation:

- **Clip size cap changed from 16384 to 15360 bytes (15KB), on both sides.** `WORD_CLIP_MAX_BYTES` in `mp3_player.cpp` and `MAX_CLIP_BYTES` in `tools/pack_word_bank.py` now both read 15360 — these two constants MUST stay in sync (the packer decides what goes in the bank; the device decides what fits in its RAM buffer; a mismatch would let the packer include a clip the device then can't play from the bank). Verified with a boundary test: a clip at exactly 15360 bytes is packed, one byte over (15361) is correctly skipped with a warning.
- **Bank files moved out of `/words/<lang>/` into `/words/`, the parent.** Previously `bank.bin`/`bank.idx` lived inside the same folder as the individual word clips they were packed from — reasonable at first, but it meant every future directory listing of that folder (the per-file index scan in `buildWordAssetIndex()`, the `assets` diagnostic, or the user just browsing the card by hand) would show two files that aren't actually words, mixed in with ones that are. Moved to keep generated/derived data separate from the source-of-truth clips it came from — same principle already used elsewhere in this project (note chains live in `/notes`, not mixed into `/sfx`).
    - New location/naming: `/words/bank_en.bin` + `/words/bank_en.idx` (and `bank_ch.*` for Chichewa) — language is now baked into the filename since both languages' banks live side-by-side at the same folder level rather than being distinguished by which subfolder they're in.
    - **`mp3_player.cpp`:** `loadWordBank()` updated to construct these new paths directly from `langEnglish` rather than deriving from `audioLangFolder()` (which still correctly points at `/words/en`/`/words/ch` for the word clips themselves — that function is unchanged, only the bank's own path construction moved).
    - **`tools/pack_word_bank.py`:** rewritten to derive the language tag from the basename of the folder you point it at (e.g. pointing it at `.../words/en` gives `lang="en"`), and write `bank_<lang>.bin`/`bank_<lang>.idx` into that folder's PARENT, not the folder itself. Verified end-to-end against synthetic test files: output lands in the correct parent location with correct naming, and a repeat byte-exact check (every `bank.idx` offset/length still slices `bank.bin` into something identical to its source file) passed after the rewrite.
    - Updated in-script usage instructions and the printed "next steps" at the end of a packing run to describe the new destination correctly (previously said "same folder as the individual .mp3 files" — now says "the SD card's /words/ folder — the SAME level as the 'en/' subfolder, NOT inside it").

### Not done this session

- Did not offer/build the root-of-SD-card option (bank files directly at `/`) that was raised alongside the `/words/` option — went with `/words/` since it keeps all audio-related files under one parent rather than scattering top-level files at the card root. Easy to change if root is actually preferred; flagging rather than assuming.
- Did not implement the fixed-size-slot idea discussed earlier in this session (padding every clip to a uniform size so offsets become pure arithmetic and `bank.idx` becomes unnecessary) — that remains a suggested future option, not something requested to build yet.

## Build fix: `audioLangFolder` used before declared (session 12)

**Session start:** 2026-07-18 (start of this turn, precise clock not independently logged by tooling — using conversation timestamp) **Session end:** 2026-07-18 **Logged by:** kdprince claude

Reported build error:

```
src/mp3_player.cpp:181:19: error: 'audioLangFolder' was not declared in this scope
   String folder = audioLangFolder();
```

- **Root cause:** `buildWordAssetIndex()` (defined at line 176) calls `audioLangFolder()` at line 181, but `audioLangFolder()`'s real definition doesn't appear until line 237 — later in the same translation unit. C++ requires a function to be declared (not necessarily defined) before its first use in a given file; nothing earlier in the file told the compiler `audioLangFolder` existed, so it failed to resolve the call.
- **Fix (`mp3_player.cpp`):** added a single forward declaration, `static String audioLangFolder();`, immediately after the file's `#include` block (before the word-bank section). The real definition at line 237 is unchanged — this only tells the compiler the function exists ahead of where it's actually written out.
- **Verification pass:** walked every top-level function definition in `mp3_player.cpp` in file order and cross-checked each one's body against what's already been defined above it at that point in the file, to rule out this same class of bug (call-before-declaration) existing anywhere else. Also checked `braille.h`, `display.h`, `buttons.h`, `storage.h`, `globals.h`, and `main.cpp` for the same pattern. Result: this was the only instance in the reviewed files — everywhere else either has its correct ordering already, or already carries its own forward declaration (e.g. `handleBackButton` in `braille.h`, `decodeBraille` in `display.h`).
- **Not checked:** `offline_grammar.cpp`'s included headers (`Normalizer.h`, `Tokenizer.h`, `Reconstructor.h`, `SpellChecker.h`, `POSTagger.h`, `GrammarEngine.h`, `QuestionDetector.h`, `PunctuationEngine.h`) — none of these were provided in this session, so their internal ordering/correctness could not be verified.

— kdprince claude

## Apparent boot freeze at "[CFG] Language: en" — not a bug, silent long scan (session 13)

**Session start:** 2026-07-18 (start of this turn) **Session end:** 2026-07-18 **Logged by:** kdprince claude

Reported: fresh boot log stops printing right after `[CFG] Language: en` and never reaches `[BOOT] Complete`, with no error. User asked whether they were meant to type `reindex` in the serial monitor at that point.

- **Not a crash or hang.** The word library on the SD card is now **9,629 clips** (confirmed from `test5.txt`, the packer tool's own output: `Packing 9629 clip(s)... Wrote E:wordsbank_en.idx (9629 entries)`). Immediately after `[CFG] Language: en` prints, `setup()` runs `buildWordAssetIndex()` (scans every file in `/words/en` by name, one at a time) and then `loadWordBank()` (reads all 9,629 lines of `bank_en.idx`) — **neither function printed anything until it finished completely**, so a scan of that size produced several minutes of total silence that looked identical to a freeze.
- **Serial commands (`reindex` etc.) do nothing at that point regardless of what's typed** — the code that reads the serial monitor only runs inside `loop()`, and `loop()` cannot start until `setup()` returns. While stuck inside `buildWordAssetIndex()`/`loadWordBank()`, the board is not yet listening to the serial monitor at all; this was true before this session's fix too, and remains true — it's normal, not a bug.
- **Fix (`mp3_player.cpp`, cosmetic/diagnostic only — no logic changed):**
    - `buildWordAssetIndex()` now prints a "starting scan" line before it begins, and a `[INDEX] ...still scanning, N files so far (T ms elapsed)` heartbeat every 1000 files during the directory walk.
    - `loadWordBank()` now prints a matching `[BANK] ...still parsing index, N lines so far (T ms elapsed)` heartbeat every 1000 lines while reading `bank_en.idx`.
    - Both additions only add `Serial.print` calls at fixed intervals — no change to what gets indexed, how long it actually takes, or any other behavior. Purpose is solely so a long scan is visibly alive instead of silent.
- **Not fixed (flagged, not addressed):** the actual *duration* of the scan itself. With ~9,629 files, `buildWordAssetIndex()`'s `openNextFile()` walk could still legitimately take real time (the per-file positional-open cost discussed in session 9 applies to named opens, not necessarily to sequential `openNextFile()` calls, but this has not been measured on a library this large) — the heartbeat only makes that wait visible, it does not shorten it. If boot time at this scale becomes a real usability problem, the next step would be to time the actual scan on hardware and consider whether the word count needs to be reduced, or whether `buildWordAssetIndex()` is even necessary once a full word bank (`loadWordBank()`) is already in place — since `resolveAudioAsset()`'s first check is the bank, the per-file index may be largely redundant now that a 9,629-entry bank exists. Recommend confirming with the user before removing `buildWordAssetIndex()` from the boot path, since it's still the fallback if the bank ever fails to load.

— kdprince claude

## Confirmed: `buildWordAssetIndex()` scan cost is quadratic, not linear — reordered to prefer the bank (session 14)

**Session start:** 2026-07-18 (start of this turn) **Session end:** 2026-07-18 **Logged by:** kdprince claude

The heartbeat logging added in session 13 immediately paid for itself: it produced real measured numbers instead of a guess.

**Measured on actual hardware, ~9,629-file word library:**

<table class="e-rte-table"> <thead> <tr> <th>Files scanned</th> <th>Cumulative time</th> </tr> </thead> <tbody> <tr> <td>1,000</td> <td>155,123 ms (~2.6 min)</td> </tr> <tr> <td>2,000</td> <td>658,867 ms (~11.0 min)</td> </tr> </tbody> </table>

The second batch of 1,000 files took **503,744 ms — over 3x longer** than the first identical-size batch. Fitting this to a quadratic model (`total time ≈ k × n²`, consistent with each `openNextFile()` call re-walking the directory from the start rather than resuming from a saved position) predicts roughly **4 hours** to finish scanning all 9,629 files. This confirms and sharpens the open question flagged at the end of session 13 — the per-file/per-position cost identified in session 9 for *named* opens (`SD.open("word.mp3")`) also applies to the *sequential* `openNextFile()` directory walk used by `buildWordAssetIndex()`, and it's worse than linear.

- **Immediate advice given to user:** power-cycle the board rather than wait — the scan will not complete in a reasonable time at this file count.
- **Root fix — reorder so the bank is tried first, and the slow scan is skipped entirely when it succeeds.** `loadWordBank()` reads `bank_en.idx` as one plain-text file top to bottom — ordinary sequential file I/O, none of the per-position directory-walk penalty — and already contains the complete word list for a fully-packed language. Since `resolveAudioAsset()` (in `mp3_player.cpp`, unchanged this session) already checks the bank before ever consulting the per-file index, running the full slow scan whenever a working bank is present was pure wasted time, not a safety net.
    - **`main.cpp` (`setup()`):** swapped the call order — `loadWordBank()` now runs first; `buildWordAssetIndex()` only runs if the bank failed to load (missing/malformed bank files, or this language never got packed). Both outcomes are logged explicitly (`"Word bank loaded - skipping slow per-file directory scan"` vs `"No word bank loaded - falling back to per-file index scan"`) so it's never ambiguous which path a given boot took.
    - **`main.cpp` (`reindex` serial command):** same reorder applied. Previously this command *always* did the full slow rebuild before touching the bank; now it tries the bank first and only pays for the scan if the bank isn't usable. Added an on-screen reminder that if the user is running `reindex` specifically because they added NEW clips not yet in the bank, they need to repack `bank_en.idx`/`.bin` on the PC first — otherwise the new files won't be found via the (now-skipped) scan either.
    - **`buttons.h` (`handleLangSelect()`):** same reorder applied. This path matters even more than boot: it fires **live, mid-use**, while the UI is blocked waiting on it, when the user switches language on-device. Leaving the old order here would have meant switching to English (the large, ~9,629-word language) could freeze the device for hours in the middle of normal use, not just at boot.
- **Correctness check:** confirmed this doesn't remove any real capability — for a word NOT in the bank (e.g. added to the card after the last repack, or the bank simply wasn't built for this language), `resolveAudioAsset()` still falls through to a raw on-demand `SD.open()` existence probe for that one word when the per-file index isn't built. That's slower per-word than an indexed lookup, but it's a single per-word cost paid only for the rare out-of-bank word — not a full 9,629-file scan paid unconditionally on every boot/language-switch/reindex regardless of whether the bank already had the answer.
- **Not done this session:** did not attempt to fix `buildWordAssetIndex()`'s own scan algorithm (e.g. investigating whether the underlying `SD.h`/FAT library exposes a way to resume `openNextFile()` from a saved position instead of re-walking from the start) — the bank-first reorder sidesteps the problem for the common case (bank present) rather than solving the scan's own cost, which only still matters on the fallback path (no bank). If a fully bank-less setup at this file count is ever a real requirement, that scan algorithm itself would need real optimization, not just deprioritization.

— kdprince claude

## Boot crash-loop during `loadWordBank()` — out-of-memory `abort()`, fixed (session 15)

**Session start:** 2026-07-18 (start of this turn) **Session end:** 2026-07-18 **Logged by:** kdprince claude

Reported: after session 14's reorder (bank loads first, which worked — parsing reached 2000 of 9,629 lines in well under a second, confirming the reorder itself was correct), the board then crashed with `abort() was called at PC 0x4010da67 on core 1` partway through parsing the bank index, then rebooted and hit the identical crash at the identical point every single time — an infinite crash-reboot loop.

- **Root cause: out-of-memory.** `WordBankEntry.word` was a `String`. On this chip's C++ runtime, each `String` object owns its own separate, individually-allocated small memory booking. Loading 9,629 entries meant 9,629 individual tiny allocations. That many small separate allocations exhausts and fragments the chip's available memory (independently confirmed as the likely mechanism: the crash point was bit-for-bit identical across every reboot in the log, which is exactly what you'd expect from a fixed-size dataset hitting a fixed-size memory ceiling at the same point every time — not confirmed via a debug symbol table, since none was available this session, but consistent with every piece of evidence in the log). With exceptions disabled (typical default for this platform), a failed memory request calls `abort()` directly instead of throwing a catchable error — matching the exact crash signature reported.
- **Fix (`mp3_player.cpp`):**
    1. **`WordBankEntry.word` changed from `String` to a fixed-size `char[24]` buffer** (`BANK_WORD_MAX_LEN` = 23 usable characters + 1 terminator). This is the primary fix — it replaces 9,629 separate small allocations with ONE single allocation for the whole list, eliminating the fragmentation/exhaustion pattern that caused the crash. A word longer than 23 characters is skipped (counted and logged once in aggregate, not silently dropped) rather than overflowing the buffer.
    2. **Added a first pass that counts usable lines in `bank_en.idx` before parsing, then calls `_wordBank.reserve(totalLines)` once.** Without this, `std::vector` grows by repeated doubling, and every growth event copies everything added so far — meaning at various points during loading, roughly 1.5x the final memory needed to be alive simultaneously. `reserve()` removes that doubling/copying pattern entirely; this compounds with fix #1 rather than replacing it.
    3. **Added a live heap safety check inside the parse loop** (`BANK_LOAD_HEAP_SAFETY_MARGIN_BYTES` = 40KB reserved headroom). If free heap drops to that margin, the loop stops adding further entries immediately and logs exactly how many of the total lines were actually loaded, instead of continuing until an allocation fails into another `abort()`. Whatever loaded successfully is kept, sorted, and used. This turns "hard crash on overrun" into "graceful partial coverage" — words not in the (possibly partial) bank still resolve correctly via the existing per-file-index/raw-SD fallback path in `resolveAudioAsset()`, unchanged this session; they just don't get the bank's speed benefit.
    4. **Added `ESP.getFreeHeap()` logging** before loading starts, every 1000 lines during parsing, and after loading finishes (or stops early), so the actual memory headroom on real hardware is visible in the log going forward instead of having to infer it from a crash.
    5. Updated `WordBankEntryLess` and the sort comparator to compare via `strcmp()` on the fixed buffers instead of `String`'s `<` operator (functionally equivalent, required by the type change in #1).
- **Honest arithmetic — flagging, not guessing, whether this actually solves it fully.** The new `WordBankEntry` is `char[24] + uint32_t + uint32_t` = 32 bytes per entry. For all 9,629 entries: `9,629 × 32 ≈ 308,128 bytes (~301 KB)`. Typical usable heap on this class of chip, after the audio decode buffers, the prefetch task stack, display buffer, and filesystem library overhead already reserved elsewhere in this codebase, is commonly in the 200–300 KB range (not independently measured on this exact board this session — the new heap logging will show the real number on next boot). **That means a ~301 KB single allocation for the full word list may still fail to fit entirely, even with this fix.** The difference is what happens if it doesn't: instead of a crash-reboot loop, the device will now log `[BANK] STOPPING early at line N of 9629 - free heap too low`, keep whatever partial list did fit, and continue booting normally with graceful (slower, not silent) fallback for the rest. This is a genuinely safe outcome, but it is not a guarantee of full-speed lookup for every one of the 9,629 words — that will only be known once the "Free heap before load" number from the new logging is actually seen.
- **Recommended next step if a partial load turns out to be the actual outcome (not yet confirmed):** the durable fix is to stop holding the full word index in RAM at all, and instead store it on the SD card itself as a sorted, fixed-length binary record file, so a lookup becomes a small number of direct file seeks (binary search on disk) instead of loading everything into memory up front. This is a real architecture change (new PC-side packer output format, and a different `bankLookup()` implementation), not something attempted this session — flagging it as the correct long-term answer rather than adding another band-aid on top of this one if the heap numbers come back too tight.

— kdprince claude

## Clarification: why ESP32 RAM is a constraint despite bank.bin being 42MB on SD (session 16)

**Session start:** 2026-07-18 19:39 UTC **Session end:** 2026-07-18 19:39 UTC **Logged by:** Sam Claude

User question, prompted by re-reading `test 6.txt` (a pre-session-15 boot log — it still shows the old `abort()` crash loop from the `String`-based `WordBankEntry`, so it documents the bug being diagnosed, not a result of the session 15 fix): if `bank.bin` is 42MB and `bank.idx` is only 210KB on the SD card, why does RAM become a limiting factor at all?

- **No source files changed this session — clarification only, logged per standing instruction to record every session.**
- **`bank.bin` (42MB) is not loaded into RAM and never was.** It stays on the SD card. `loadWordBank()` opens ONE `File` handle to it (`_bankFile`) and keeps that handle open; every actual word playback does a `seek(offset)` + `read(length)` of a few KB directly against that open handle. RAM cost for `bank.bin` itself is effectively the small `AudioFileSourceRAM`/clip buffer already in use elsewhere (`WORD_CLIP_MAX_BYTES` = 15KB), not the file's full size.
- **`bank.idx` (210KB on disk) IS fully parsed into RAM at boot — that's the actual constraint**, and it does not cost 210KB in memory, it costs more. `bank.idx` is compact CSV text (`word,offset,lengthn`, variable-length names). Once parsed, each line becomes an in-RAM `WordBankEntry` struct — `char word[24]` (fixed, padded) + `uint32_t offset` + `uint32_t length` = 32 fixed bytes per entry regardless of how short the word was on disk. For 9,629 entries: `9,629 × 32 ≈ 301KB` — larger than the 210KB source file, because a fixed-size struct doesn't compress the way variable-length text does.
- **Why that specific number matters:** a typical ESP32's usable heap, after Wi-Fi/BT stack, audio decode buffers, the prefetch task, and display/filesystem overhead already reserved elsewhere in this project, commonly lands in the 200–300KB range — putting a ~301KB single allocation right at or past the edge, which is exactly the scenario session 15's heap-safety cutoff (`BANK_LOAD_HEAP_SAFETY_MARGIN_BYTES`) was added to fail gracefully against instead of crashing.
- **Summary distinction:** `bank.bin`'s size is irrelevant to RAM because it's accessed by seek/read, never loaded whole. `bank.idx`'s size is the relevant number, and even that understates the real RAM cost — what actually matters is entry count × fixed struct size (32 bytes), not the index file's byte size on disk.

— Sam Claude

## RAM ceiling removed: word bank index now binary-searched on the SD card, never loaded into RAM (session 17)

**Session start:** 2026-07-18 19:39 UTC **Session end:** 2026-07-18 19:57 UTC **Logged by:** Sam Claude

Prompted directly by a real hardware reading the user provided this session (PlatformIO memory report: 327,680 bytes total RAM, 80,100 bytes used at boot, ~247,580 bytes free before `loadWordBank()` even runs). Applying that number to the session-15 fix confirmed the full 9,629-entry bank (~301KB) does **not** fit in free heap on this board — the partial-load path from session 15 was the realistic outcome here, not a rare edge case. User asked whether the index could instead be searched directly on the SD card the same way `bank.bin`'s audio already is, rather than parsed into RAM at all. It can — implemented this session.

### Root design change

The RAM-resident `std::vector<WordBankEntry> _wordBank` from session 15 is **removed entirely**. In its place, the index (`bank_<lang>.bidx`) is now a **sorted, fixed-length binary record file** — 32 bytes per record (`char word[24]` + `uint32_t offset` + `uint32_t length`, little-endian) — kept open on the SD card and searched directly via `seek()`+`read()` binary search, exactly the same access pattern already used for `bank.bin`'s audio bytes. RAM cost for the whole index is now a small constant (one 32-byte record buffer during a lookup), regardless of vocabulary size — the ~301KB ceiling that made session 15's partial-load fallback a near-certainty on this hardware is gone.

- **`mp3_player.cpp`:**
    - Removed `struct WordBankEntry`, `_wordBank` (`std::vector`), `WordBankEntryLess`, and `BANK_LOAD_HEAP_SAFETY_MARGIN_BYTES` — no RAM array, so no heap-safety cutoff is needed for the index anymore (the audio side, `WORD_CLIP_MAX_BYTES`, is unrelated and unchanged).
    - Added `readBankRecord(recordIndex, ...)` — one `seek(recordIndex * 32)` + one 32-byte `read()` against the open index handle. "Record N" is always at a known byte offset — no scanning, no parsing.
    - `loadWordBank()` no longer reads or parses the index at all. It opens `bank_<lang>.bidx`, validates the file size is a whole multiple of 32 bytes (a cheap format sanity check — rejects a leftover old-format text `.idx` instead of misreading it as binary), and derives entry count from `size / 32` — one `size()` call, not a scan. Opens `bank_<lang>.bin` as before.
    - `bankLookup(stem, ...)` is now a real binary search: `ceil(log2(N))` seek+reads against the open index handle (14 for ~9,629 entries) instead of `std::lower_bound` over an in-RAM array. Same function signature, same return contract — every existing caller (`resolveAudioAsset`, `playBankPathDirect`, `loadMp3ToRam`, `isValidMp3`) needed zero changes.
    - **Concurrency fix required by this change:** the old in-RAM `bankLookup()` was a pure read with no shared mutable state, so it never needed a lock. The new version does real SD I/O on `_bankIdxFile`, a handle shared between the foreground task and the background prefetch task (same class of hazard already guarded for `_bankFile`). Moved `_prefetchMutex`/`ensurePrefetchMutex()` earlier in the file (previously defined only in the RAM-backed-playback section, after `bankLookup`) and wrapped the binary search in it. Caught and fixed during this session's own review, not reported separately — flagging since it's a real correctness fix, not just a refactor.
- **`mp3_player.h`:** updated `loadWordBank()`'s doc comment to describe the on-SD binary-search behavior and the RAM-ceiling rationale, referencing the real hardware numbers from this session.
- **`tools/pack_word_bank.py` — rewritten from CSV-text output to the new binary format.** (This file wasn't among the project files provided in earlier sessions, only referenced in changelog prose — written fresh this session to match the documented behavior, then updated for the binary format.)
    - Output filenames changed: `bank_<lang>.idx` (text) → `bank_<lang>.bidx` (binary) — deliberately a different extension, not a same-name overwrite, so a stale text-format file left on a card from before this session is never silently misread as binary.
    - Writes each record via Python's `struct.Struct("<24sII")` — little-endian, matching the device's native byte order, so no byte-swapping is needed on either side.
    - Same clip-size cap (`MAX_CLIP_BYTES = 15360`, unchanged, still must match `WORD_CLIP_MAX_BYTES` in `mp3_player.cpp`) and same skip-with-warning behavior for oversized/empty clips and over-length filenames.
    - **Verified via a real, runnable self-test** (`python3 pack_word_bank.py --self-test`), not just reasoned about: packs a synthetic folder (short words, one deliberately-too-long word, one deliberately-oversized clip), then checks — (1) every `.bidx` record's offset/length slices `.bin` into bytes byte-identical to its source clip, (2) records are sorted by word, (3) a manual binary-search re-implementation over the written records finds every real word and correctly misses a nonexistent one, (4) skip counts match expectations. **Ran this session, passed.**

### What this does and doesn't change

- **Fixes:** removes the RAM ceiling on bank coverage entirely. A vocabulary of any size can be fully covered by the bank with no RAM impact beyond a small constant — no more partial-load truncation, no more heap-safety cutoff needed for the index.
- **Cost:** each word lookup now does up to ~14 small `seek()+read()` calls against the SD card instead of one `std::lower_bound` in RAM. These are fixed-offset, non-directory-walk accesses — nothing like the slow filename-based `open()` cost measured in session 9 — but they are real SD I/O, not free. Not benchmarked on real hardware this session; flagging as the thing to confirm on next boot alongside the new `[BANK] Ready - N entries ... (searched on-card via seek, nothing parsed into RAM)` log line.
- **Breaking, requires action:** the index file format and filename changed (`bank_<lang>.idx` text → `bank_<lang>.bidx` binary). Old bank files already on an SD card will simply not be found (`loadWordBank()` looks for `.bidx` specifically) — this fails safe (falls back to the per-file index/raw SD path), not silently misreads the old format. **Existing `bank_en.idx`/`bank_ch.idx` files should be deleted from the card** (optional cleanup, not required for correctness) and **regenerated with the new packer** — see instructions below. Note audio chains themselves are unaffected (chain format `W:`/`L:`/`SP`/`SPS` is unchanged, unrelated to this session).

### Clear next steps

1. **On your PC**, run the new packer against each language's word-clip folder:

    ```
    python3 pack_word_bank.py /path/to/sdcard/words/enpython3 pack_word_bank.py /path/to/sdcard/words/ch
    ```

    Each run writes `bank_en.bin`/`bank_en.bidx` (or `_ch`) into `/path/to/sdcard/words/` — the parent of `en/`/`ch/`, same location as before, just new binary `.bidx` files instead of text `.idx`.
2. **Copy the SD card's `/words/` folder contents to the device's SD card** (or copy just the four new/updated `bank_*.bin`/`bank_*.bidx` files if you're updating in place).
3. **Delete any old `bank_en.idx`/`bank_ch.idx` (text) files** from the card if present — not required (they'll just be ignored), but avoids confusion when browsing the card by hand.
4. **Reboot the device**, or run the `reindex` serial command if it's already running. Watch for:

    ```
    [BANK] Ready - <N> entries in /words/bank_en.bidx (searched on-card via seek, nothing parsed into RAM)[BANK] Free heap after load: <N> bytes
    ```

    `<N> entries` should now equal (or be very close to) the full word count the packer reported — no more `STOPPING early ... free heap too low` line, since there's no RAM array left to run out of room for.
5. **No `rechain` needed** for existing notes — the chain file format itself didn't change, only how a `W:`/`L:` word name resolves to bytes. Existing chains work immediately once the new bank is in place.
6. If you add brand-new word clips to the card later: repack (step 1) → copy the two updated files (step 2) → `reindex` on-device (step 4). Same workflow as before, just producing/copying `.bidx` instead of `.idx`.

### Not done this session

- No benchmarking of actual seek+read latency for the binary search on real hardware — the design assumption (fixed-offset seeks are fast and roughly constant, unlike filename-based directory walks) is consistent with everything measured in session 9, but hasn't been independently timed for this specific access pattern. Worth confirming with the new `[BANK] Ready` log line's context (does boot feel instant, as expected) on next real boot.
- Did not add a migration/compatibility shim to auto-detect and convert an old text `.idx` file into the new binary format on-device — per this project's established pattern (chain format changes, bank format itself), format changes are handled via PC-side regeneration, not on-device conversion.

— Sam Claude

## Debug pass on session 17's on-SD bank rework: 2 real bugs fixed, 1 pacing tradeoff flagged (session 18)

**Session start:** 2026-07-19 00:?? UTC (exact turn-start not independently captured; using tool-logged first timestamp of this session, 01:07 UTC, as the closest reliable anchor — will capture turn-start more precisely going forward) **Session end:** 2026-07-19 01:07 UTC **Logged by:** Sam Claude

Requested: a full debug pass over session 17's rework specifically for miscalculated logic, probable fail points, unnatural sound pacing, interrupt correctness, priority ordering, memory management, and performance. Went through `mp3_player.cpp` systematically rather than asserting confidence — findings below, in the order checked.

### Checked and confirmed correct (no changes needed)

- **Deadlock risk in the new mutex-guarded `bankLookup()`:** traced every call site (`resolveAudioAsset`, `playBankPathDirect`, `isValidMp3`, `loadMp3ToRam`). The one case that looked risky — `prefetchTaskFn` holds `_prefetchMutex` and calls `loadMp3ToRam()`, which internally calls `bankLookup()`, which takes the *same* mutex again — is exactly the scenario the mutex being `Recursive` (`xSemaphoreCreateRecursiveMutex`, `TakeRecursive`/`GiveRecursive` used consistently everywhere, confirmed via grep — no plain non-recursive `xSemaphoreTake`/`Give` anywhere in the file) is designed to handle safely. Correct as shipped.
- **`playMP3()`'s `lastInterruptPin` reset ordering:** confirmed the reset (`lastInterruptPin = -1`) happens before the branch into `playBankPathDirect()` for `"bank:"` paths, so the bank path always starts from a clean interrupt state. No stale-interrupt bug here.
- **`readBankRecord()`'s byte-offset math:** manually verified against `BANK_RECORD_SIZE=32`/`BANK_WORD_MAX_LEN=23`: word field `[0,24)`, offset `[24,28)`, length `[28,32)` — matches the packer's `struct.Struct("<24sII")` layout exactly, both little-endian (native to the ESP32's Xtensa core, so no byte-swap needed). No off-by-one.
- **Binary search bounds logic:** re-implemented the exact `lo/hi/mid` loop in Python and ran it against sorted word lists including adjacent-prefix cases (`"mid"` vs `"middle"`) and edge cases (empty list, single element, not-found). All passed. No off-by-one, no infinite-loop risk.
- **`_audioSrc` pointer lifecycle** (alloc/delete pairing across `playBankPathDirect`, `playMP3`, `playChainWordFromRam`): pre-existing pattern, not touched this session, confirmed still consistent — every allocation is preceded by a delete-if-present guard and followed by a delete-after-use. No leak, no double-free introduced or pre-existing.
- **`loadWordBank()`'s malformed-file guard:** a 0-byte or non-32-byte-multiple `.bidx` is explicitly rejected with a clear log line rather than silently proceeding with `_wordBankEntryCount=0` — cleaner failure mode than relying solely on `bankLookup()`'s downstream zero-count guard.

### Bug found and fixed: redundant double SD lookup per chain word

`playChainAtPath()`'s lookahead scan calls `resolveAudioAsset()` on the *next* word to kick off its background prefetch — then, one loop iteration later, `playChainUnit()` calls `resolveAudioAsset()` on that *same* word again to actually play it. Before session 17 this cost nothing (pure in-RAM `std::lower_bound`, microseconds). After session 17 moved the bank index to an on-SD binary search (~14 mutex-guarded seek+read calls per lookup), this pre-existing double-call pattern now means **every chain word's bank lookup runs twice** — a real, avoidable doubling of new SD I/O that didn't matter before this session's change but does now.

**Fix:** added a per-line resolve cache (`resolvedFlag`/`resolvedPath` vectors, sized once to `lines.size()`) inside `playChainAtPath()`, via a `resolveCached(idx)` lambda. The lookahead scan populates the cache; `playChainUnit()` now takes the already-resolved path as a parameter instead of re-resolving it. Each word is now looked up exactly once per chain playback, regardless of how many times its resolved path is referenced.

- Changed `playChainUnit()`'s signature from `(line, producedAudio)` to `(line, resolvedPath, producedAudio)` — it no longer calls `resolveAudioAsset()` itself.
- Verified only one call site existed for the old signature; updated cleanly, no orphaned old-signature callers left.

### Bug found and fixed during the cache implementation itself (caught before shipping, not a separate report)

While writing the fix above, an initial version used a ternary (`cond ? resolveCached(i) : line`) to pick between the cached-lookup reference and a fallback for non-word lines — binding a `const String&` ternary's two branches to two different-lifetime sources (`resolveCached()`'s vector-owned reference vs. the loop's `line` reference) is a real dangling-reference hazard pattern in C++, not just a style nit. Caught in review before finalizing; replaced with a plain `String cachedPath = isWordLine ? resolveCached(i) : String("");` — a small value copy, sidestepping the lifetime question entirely rather than trying to make the reference version provably safe. Also fixed a broken comma-operator expression in `playChainUnit()`'s non-prefetch (`#else`) branch left over from an editing mistake (`playMP3()` returns `void`; the original attempted `bool ok = playMP3(...), true;` doesn't do what it looks like it does) — corrected to match the pre-existing `playResolvedAsset()` semantics (path existed and was handed to `playMP3()` → treat as success, since `playMP3` has no return value to check).

### Flagged, not fixed: lookahead resolve is no longer free, so less latency gets hidden behind playback

This is a real pacing tradeoff, not a bug, and worth stating plainly rather than leaving implicit. The lookahead scan's `resolveAudioAsset()` call for the *next* word runs **synchronously on the foreground task**, before `requestPrefetch()` can even be called — you need the resolved offset/length before you can ask the background task to fetch those bytes. Before session 17 this resolve step was near-instant, so effectively the *entire* ~1.2–1.5s playback window of the current word was available to hide the *next* word's file-open latency behind. After session 17, the resolve step itself now costs real SD time (up to ~14 small seek+reads), which happens **before** the hide-behind-playback window starts, shrinking it by however long the resolve takes.

Not benchmarked this session — no hard numbers for a single 32-byte `seek()+read()`'s real latency on this specific SD card. Even at a conservative 5–20ms each, 14 of them (worst-case binary search depth) could add on the order of 70–280ms of new, previously-nonexistent latency ahead of each word's prefetch request. This is very unlikely to cause an audible gap on its own (the background task still has the remainder of the ~1.2–1.5s window, and the existing 8-second timeout / direct-load fallback in `playChainWordFromRam` already handles a prefetch that doesn't finish in time gracefully — same safety net as before, unchanged) — but it is a real, quantifiable reduction in how much latency gets successfully hidden compared to the pure-RAM version, and should be confirmed against a real boot log's per-word timing rather than assumed away.

### Not checked this session (scope boundary, not an oversight)

- No hardware timing was gathered — everything above is static code review, not a live boot/profile. The "flagged, not fixed" item specifically needs a real measurement to move from "theoretical tradeoff" to "confirmed non-issue" or "needs further optimization."
- Did not re-audit files outside `mp3_player.cpp`/`mp3_player.h` (e.g. `braille.h`, `buttons.h`, `display.h`) — session 17's change was scoped to the bank/index mechanism only, and this debug pass followed that same scope rather than re-reviewing the whole project.
- Did not re-run the packer's self-test this session (already run and passing in session 17) — no packer-side code changed this session, only the device-side chain-playback caching.

— Sam Claude

## Pacing fix (I2S restart removed between chain words) + DOWN scrolls without stopping note-body audio (session 19)

**Session start:** 2026-07-19 07:?? UTC (turn-start not independently captured; using first tool-logged timestamp, 08:10 UTC, as anchor) **Session end:** 2026-07-19 08:10 UTC **Logged by:** Sam Claude

Two requests from a real post-fix boot log: (1) word-to-word pacing was still a "few milliseconds" off natural speech and should be tightened further; (2) DOWN did nothing useful while a note body was being read aloud — user wants it to scroll the screen in sync, without stopping the audio.

### Pacing: measured, diagnosed, and fixed a real ~148ms/word overhead

Parsed the provided boot log's `AUDIO MP3 START/DONE (RAM)` timestamps directly rather than estimating. Clean mid-sentence word-to-word gaps (excluding UI-transition outliers): `blind→girl` 268ms, `girl→from` 270ms, `from→z` 265ms — remarkably consistent (within 5ms of each other), against an intended `kSpacePauseMs` of only 120ms. That consistency ruled out variable SD/lookup jitter as the cause and pointed at fixed per-word setup/teardown cost instead.

**Root cause identified:** `playChainWordFromRam()` called `_i2sOut.stop()` unconditionally at the end of **every single word**, and the next word's `_mp3.begin()` implicitly re-initialized the I2S output from scratch. Stopping/restarting an I2S DMA output has real hardware settling time; this was happening on every word boundary in a chain, not just at genuine sentence/chain boundaries.

**Fix (`mp3_player.cpp`):**

- Added `keepI2SRunning` parameter to `playChainWordFromRam()`. The I2S output (`_i2sOut`) is now only stopped after the chain's actual LAST audio-producing word (or on interrupt) — not after every intermediate word. The MP3 *decoder* (`_mp3`) still gets a clean `stop()`+`delay(15)` between clips as before (unchanged, still required — it's a stateful generator being handed a new source each time); only the underlying I2S peripheral stays running across consecutive words now.
- `playChainAtPath()` precomputes a `moreAudioAfter[]` table (one O(n) backward pass, done once — not re-scanned per line) so each word knows in advance whether it's the chain's last audio-producing unit.
- Added a safety-net unconditional `_i2sOut.stop()` at the true end of `playChainAtPath()`, covering the case where playback is interrupted mid-chain before naturally reaching the last word.
- **Flagged explicitly, not silently assumed safe:** this trades a proven timing cost for an unverified assumption about the ESP8266Audio library — specifically, that `AudioGeneratorMP3::begin()` against a still-running `AudioOutputI2S` correctly resets decoder state the same way it does against a freshly-started one. This library's internals aren't available to inspect in this environment (same caveat already on record from earlier sessions re: `AudioFileSourceRAM`). **This needs a real listen-test on hardware** — worst case if the assumption is wrong is an audible click/pop at word boundaries, not a crash or silence, but that should be confirmed by ear, not assumed from log timing alone.

### DOWN now scrolls the note-view screen during body playback, without stopping audio

Traced why DOWN previously did nothing during body playback: `speakNoteBody()` → `playNoteAudioChain()` → `playChainAtPath()` blocks `loop()` synchronously for the whole chain; `handleViewNote()`'s existing DOWN-scroll handler (already written, doing exactly what was wanted) never got a turn to run until the chain finished or was interrupted. DOWN was also one of the five buttons in the shared `kInterruptPins` list, so pressing it mid-word interrupted that one clip's playback but — since only BACK actually stops the whole chain (`wasBackInterrupt()`) — the chain just continued to the next word anyway, meaning DOWN accomplished nothing visible.

Confirmed the desired behavior directly with the user before implementing (screen scrolls manually on DOWN, audio keeps playing uninterrupted — not auto-tracking word position, not stop-then-resume).

**Fix, scoped carefully so DOWN's behavior elsewhere is unaffected** (e.g. `speakNoteTitle()`'s existing DOWN-to-skip-to-next-title chaining, which still needs DOWN to interrupt):

- **`mp3_player.cpp`:** added `kChainInterruptPins` (BACK/SELECT/AI-SAVE/DELETE — everything except DOWN), used only during chain playback that opts in via a new `onDownPressed` callback parameter. Generalized `debouncedInterruptPin()`/`resetInterruptDebounce()` to accept an explicit pin list/count instead of being hardcoded to the original 5-button list, so both interrupt sets share the same debounce logic without duplicating it. Added an independent, separately-debounced DOWN poll (`downPressedThisPoll()`/`resetDownPoll()`) that fires the callback once per physical press (not repeatedly while held) — deliberately separate state from the interrupt-list debounce timers so the two can't interfere with each other.
- `playChainWordFromRam()`, `playChainUnit()`, and `playChainAtPath()` all gained an `onDownPressed` parameter (function-pointer, matching this codebase's existing callback style), threaded through with a `nullptr` default at every layer — so every other caller/behavior is unchanged unless a real callback is supplied.
- `playNoteAudioChain()`'s public signature (`mp3_player.h`) gained the same optional parameter, defaulted to `nullptr`. `playNoteTitleChain()` is untouched — title playback still uses DOWN as an ordinary interrupt.
- **`buttons.h`:** `speakNoteBody()` now passes a new `onDownDuringNoteBody()` callback to `playNoteAudioChain()`. That callback reuses the exact scroll logic `handleViewNote()`'s own DOWN handler already had (`noteScrollOffset++`, `drawViewNote()`) — this session didn't invent new scroll behavior, it just made the existing, already-correct logic reachable during playback, which it never was before.

### Not done this session

- No hardware verification of either change — the pacing fix's core hypothesis (I2S stop/restart is the dominant per-word cost) is well-supported by the log's timing consistency but not independently confirmed by isolating that one call; the DOWN-scroll fix hasn't been pressed on real hardware. Both need a real boot + listen/interact test before being considered fully confirmed, especially the I2S-continuity assumption flagged above.
- Did not implement word-level "auto-track the spoken word" scrolling — user explicitly chose the simpler manual-scroll-during-playback behavior over karaoke-style auto-advance, so that larger `display.h`-touching feature was intentionally not built.
- Did not re-run the full session-18 static debug checklist (deadlock/lifetime/etc.) against this session's new code — recommend a follow-up pass given how much of `mp3_player.cpp`'s chain-playback machinery changed again this session.

— Sam Claude#pragma once
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
    String fullNote=getNoteTitle(selectedNote)+"n"+aiImprovedNote;
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