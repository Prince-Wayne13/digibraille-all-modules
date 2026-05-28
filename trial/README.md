trial/
├── backend/
│   ├── main.py          # FastAPI. Routes: POST /api/record, /api/save-unit, /api/synthesize, GET /api/library, /api/unit/{filename}
│   ├── audio_utils.py   # load_and_clean() → extract_f0() → propose_morpheme_cuts(). pydub input, scipy processing, noisereduce cleaning
│   ├── db.py            # init_db(), save_unit(), search_units() — SQLite (units.db)
│   ├── synthesis.py     # TRANSITION_METHODS dict (crossfade, pitch_matched, etc.)
│   ├── uploads/         # raw uploads + saved unit WAVs
│   └── units.db
│
└── frontend/src/
    ├── App.jsx           # state hub. recordingState, editorStatus, audioData{audioBlob, sampleRate, proposedSegments}. calls /api/record, converts hex→Blob, passes to WaveformEditor
    ├── WaveformEditor.jsx# props: audioBlob(Blob), sampleRate, proposedSegments, onSegmentAccepted, status, statusMessage. WaveSurfer v7 + RegionsPlugin. encodeWav() slices regions client-side
    ├── SynthesisLab.jsx  # consumes libraryUnits, calls /api/synthesize
    ├── MorphologyPanel.jsx # calls /api/morphology → Claude API → returns morpheme breakdown JSON
    ├── utils/mic.js      # startRecording() → returns {start(), stop()→Blob}. outputs webm
    └── index.css         # all styles

    mic/file → FormData(file, transcript) → POST /api/record
  → pydub decode any format → clean → f0 → segments
  → JSON { cleaned_audio: hex, proposed_segments: [...] }
  → App.jsx hex→Blob → WaveformEditor
  → user edits regions → onSegmentAccepted → POST /api/save-unit
  { start_ms, end_ms, duration_ms, transcript,
  morph_type, f0_mean, f0_onset, f0_offset, confidence }