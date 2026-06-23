# DigiBraille Test Plan

**Purpose of this document:** a single checklist to work through after the WiFi/online features are removed and the pin conflict is resolved. Each test has a clear goal — not just "try it and see," but a specific pass/fail bar, because a few of these (audio timing, navigation) are real design decisions, not just bugs to catch.

How to use this: go top to bottom. Section 1 first, because nothing else matters if the wiring is wrong. Record results in the "Result" column as you go — even a single line like "failed, audio cut off after 4th char" is enough to act on later.

---

## 1. Pin & Wiring Verification

These come first because every other test is meaningless if a pin is mislabeled or shared.

| # | Test | How | Goal / Pass Condition | Result |
|---|---|---|---|---|
| 1.1 | Confirm `BTN_DELETE` physical pin | Use a multimeter (continuity mode) or a simple LED+resistor test from the button pad back to the board, to confirm which GPIO physically connects. | Confirms GPIO 17 (the compiled value), not GPIO 5 (the comment). Update the comment in code to match. | |
| 1.2 | Confirm no pin is double-wired | Visually trace every wire from the build against the final pin table (Braille dots, control buttons, OLED, DAC, flash if present). | Every physical wire matches exactly one entry in the pin table. No pin serves two components. | |
| 1.3 | Test remapped Dot 3, 4, 5 pins individually | Press each dot alone, confirm correct character/dot value is registered on screen. | Each dot reports the correct bit pattern, with no cross-talk between dots. | |
| 1.4 | Isolated test of GPIO 0 (if used in remap) | Power-cycle the board with GPIO 0 wired as planned, confirm normal boot occurs (not stuck in flash/download mode). | Board boots normally every time, 10/10 power cycles. If it fails even once, do not use GPIO 0 — pick a different pin. | |
| 1.5 | Confirm flash chip pins are untouched (if flash board is present separately) | Trace SCK/MISO/MOSI/CS wiring on the flash prototype board only. | No overlap with the main board's dot pins, since they are now on separate hardware (per Option A from prior discussion). | |

---

## 2. Button Response Time

Goal of this whole section: find out if any button is meaningfully slower than the others, and confirm debounce (15ms) is doing its job without overcorrecting.

| # | Test | How | Goal / Pass Condition | Result |
|---|---|---|---|---|
| 2.1 | Baseline timing, all 6 dots | Log `millis()` at press-detected and at action-complete, for each dot, 20 presses each. | All dots within a similar range of each other (no outlier 5x slower than the rest). | |
| 2.2 | Baseline timing, all 6 control buttons | Same method, for Select, Back, Down, Reread, AI/Save, Delete. | Same as above — no outliers. | |
| 2.3 | Double-press / bounce test | Press each button rapidly 10 times in under 2 seconds each. | No double-registers (one press counted as two) and no missed presses. | |
| 2.4 | Debounce tuning check | If 2.3 fails, test debounce values above and below 15ms (e.g. 10ms, 20ms, 25ms) against your actual physical switches. | Find the lowest debounce value that produces zero double-registers across 30+ presses. Lower is better for responsiveness, but only if it's reliable. | |

---

## 3. Audio Feedback vs. Typing Speed

This is the most important section for daily usability. Refer back to the two failure modes: **device ignores presses during speech** (structural bug) vs. **device hears presses but audio gets confused** (playback policy decision).

| # | Test | How | Goal / Pass Condition | Result |
|---|---|---|---|---|
| 3.1 | "Deaf while talking" check | Trigger long speech (read full note). Press Down/Back at start, middle, and end of the speech. | Every press is acknowledged in some way — visually or audibly — never silently dropped. | |
| 3.2 | Natural-speed typing test | Type a 10–15 character sentence at a real, non-paused pace. | OLED/text buffer is 100% accurate for every character, even if audio falls behind. Text accuracy is non-negotiable; audio lag is tolerable. | |
| 3.3 | Per-character audio behavior | While doing 3.2, listen closely: does each press get a full spoken letter, or a short tick? | Decide and confirm intentionally — recommendation: short tick per character, full speech reserved for word/sentence/title confirmation. | |
| 3.4 | Audio overlap/cutoff check | Type fast enough to outpace any spoken feedback. | Audio either cleanly queues or cleanly interrupts — it should never play two sounds on top of each other at the same time. | |
| 3.5 | Compare full-speech vs. tick-sound | If 3.3 currently uses full speech per letter, temporarily test a short tick sound instead, same typing speed. | Confirms whether full-speech-per-letter is the actual bottleneck behind any lag found in 3.2–3.4. | |

---

## 4. Navigation Speed (Down / list browsing)

| # | Test | How | Goal / Pass Condition | Result |
|---|---|---|---|---|
| 4.1 | Steady single-press navigation | Press Down once, wait for full announcement, repeat 10 times through a note list. | Every item announced fully and correctly, no skips. | |
| 4.2 | Fast repeated-press navigation | Press Down repeatedly at real "scrolling" speed, not waiting for announcements to finish. | Decide on policy (see 4.3) and confirm it's applied consistently — no random mix of both behaviors. | |
| 4.3 | Decide: interrupt vs. queue policy | Compare two behaviors directly: (a) new press cuts off current announcement and starts the next one immediately, (b) new press queues and waits its turn. | Recommendation: use interrupt-and-restart (option a) for navigation — it matches how a sighted user visually scans a list without waiting for each item to "finish." | |
| 4.4 | Silence/freeze check during fast scrolling | During 4.2, time any gaps between a press and any feedback at all (sound or screen change). | No gap should ever exceed roughly 0.5 seconds. Slow is tolerable; dead silence is not — the user must never wonder if the device is frozen. | |

---

## 5. Mixed Real-World Use (combined stress test)

| # | Test | How | Goal / Pass Condition | Result |
|---|---|---|---|---|
| 5.1 | Chained rapid actions | Type a few characters → Back → type again → Down to check menu, all within a few seconds, no pausing between actions. | Device never goes completely unresponsive for more than ~0.5 seconds at any point in the chain. | |
| 5.2 | Interrupt mid-save | Start saving a note (AI/Save), and immediately press another button before save completes. | Device either finishes the save safely first, or clearly indicates the second press was queued/ignored — never a half-saved or corrupted note. | |

---

## 6. Storage & Power Safety

| # | Test | How | Goal / Pass Condition | Result |
|---|---|---|---|---|
| 6.1 | Fill storage to the limit | Create and save notes until hitting `MAX_NOTES = 10`. | "Storage full" message triggers cleanly, with a clear next step (e.g. guided to delete a note), not a crash or silent failure. | |
| 6.2 | Fill a single note to character limit | Type a note until hitting `MAX_HISTORY = 512` characters. | Device clearly signals the limit is reached, doesn't silently truncate or crash. | |
| 6.3 | Power-loss mid-note | Start typing a note, cut power mid-sentence (unplug/battery pull), power back on. | Draft recovers from `/draft.txt` with no data loss, or the device clearly states the draft was lost — never silent data loss with no explanation. | |
| 6.4 | Power-loss during save | Cut power exactly while a note is being written to storage (AI/Save just pressed). | No corrupted note file left behind; either the old version remains intact or the new one is fully written — never a half-written, broken file. | |

---

## 7. Undo / Error Recovery Feedback

| # | Test | How | Goal / Pass Condition | Result |
|---|---|---|---|---|
| 7.1 | Single-character undo (Back, single tap) | Type several characters, single-tap Back once. | Device clearly states what was removed and what the buffer now reads — not just a generic "undo" beep. | |
| 7.2 | Sentence undo (Back, double tap) | Type a full sentence, double-tap Back. | Device clearly confirms a full sentence was removed, and states what remains, if anything. | |
| 7.3 | Exit-to-menu (double tap on empty buffer) | Clear the buffer fully, double-tap Back again. | Device clearly announces it's exiting to the main menu — this should never feel like an accidental jump. | |

---

## 8. Offline Fallback (post WiFi/online removal)

Since WiFi, Gemini, and VoiceRSS are being removed, these tests confirm the device is fully self-contained with no dead code paths left behind.

| # | Test | How | Goal / Pass Condition | Result |
|---|---|---|---|---|
| 8.1 | Confirm no WiFi connection attempts remain | Monitor Serial output on boot and during use. | Zero WiFi connection attempts, zero related error messages. | |
| 8.2 | Confirm OLED status bar no longer shows WiFi icon | Visual check of the top status bar. | WiFi/disconnected icon is removed entirely, not just hidden or always showing "disconnected." | |
| 8.3 | Confirm English audio plays from preloaded files only | Check `/sfx/en/` folder is fully populated before first use, with no background fetch task running. | English prompts play correctly on first use, with no delay from a missing-file fetch attempt. | |
| 8.4 | Confirm FreeRTOS audio-fetch task is fully removed, not just idle | Check Core 1 task list / memory usage in code or via Serial debug. | Task no longer exists — confirms it isn't quietly consuming RAM or CPU scheduling time for nothing. | |
| 8.5 | AI/Save button behavior with no AI backend connected | Press AI/Save in note view. | Device behavior is intentional and clearly communicated — either grammarllm has been wired in, or the button explicitly states "AI improvement unavailable" rather than doing nothing or erroring silently. | |

---

## 9. Physical / Tactile Usability

| # | Test | How | Goal / Pass Condition | Result |
|---|---|---|---|---|
| 9.1 | Find Dot 1 vs Dot 6 by touch alone, no sight | Have a tester close their eyes (or use a real blind/low-vision tester if possible) and locate specific dots by feel only. | Tester can reliably and quickly distinguish dot positions without looking — flag if a raised marker or asymmetric shape is needed. | |
| 9.2 | Locate control buttons by feel alone | Same method, for Select/Back/Down/Reread/AI-Save/Delete. | Tester can distinguish each control button from the others without sight. | |

---

## Summary: What "done" looks like

This test plan is complete when:

1. Every row in Section 1 passes — wiring is verified, not assumed.
2. Sections 3 and 4 have an explicit, intentional decision recorded (tick vs. full speech, interrupt vs. queue) — not just "it happened to work."
3. No test anywhere produces silence/freeze beyond ~0.5 seconds.
4. Sections 6–7 show the device never silently loses data or leaves the user without an explanation.
5. Section 8 confirms no leftover online code paths are quietly running or wasting resources.

If you'd like, the next step after running this can be a short written log template — one line per test, date, result, fix-applied — so failures don't get lost between sessions.