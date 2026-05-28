# backend/main.py
import logging, io, uuid, shutil, json
from pathlib import Path
from typing import List, Optional  # ← Added List import

import numpy as np
from fastapi import FastAPI, UploadFile, File, Form, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse, JSONResponse
from pydub import AudioSegment

from audio_utils import load_and_clean, extract_f0, propose_morpheme_cuts
from db import init_db, save_unit, search_units
from synthesis import TRANSITION_METHODS

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(levelname)-8s  %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("chichewa")

app = FastAPI(title="Chichewa Audio Builder")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://localhost:3000", "tauri://localhost"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

UPLOAD_DIR = Path(__file__).parent / "uploads"
UPLOAD_DIR.mkdir(exist_ok=True)
init_db()


@app.post("/api/record")
async def receive_recording(
    file: UploadFile = File(...),
    transcript: str = Form(...),
):
    log.info("▶  Received file: '%s'  content-type: %s", file.filename, file.content_type)

    raw_path = UPLOAD_DIR / f"raw_{uuid.uuid4().hex}{Path(file.filename).suffix}"
    with open(raw_path, "wb") as buf:
        shutil.copyfileobj(file.file, buf)
    log.info("   Saved raw upload → %s  (%.1f KB)", raw_path.name, raw_path.stat().st_size / 1024)

    log.info("   Loading & cleaning audio…")
    audio, sr = load_and_clean(str(raw_path))
    log.info("   Cleaned: %.2f s @ %d Hz", len(audio) / sr, sr)

    log.info("   Extracting F0…")
    f0 = extract_f0(audio, sr)
    valid_f0 = f0[~np.isnan(f0)]
    log.info("   F0 range: %.0f – %.0f Hz  (median %.0f Hz)",
             valid_f0.min() if len(valid_f0) else 0,
             valid_f0.max() if len(valid_f0) else 0,
             float(np.median(valid_f0)) if len(valid_f0) else 0)

    log.info("   Proposing morpheme cuts for: '%s'", transcript)
    segments = propose_morpheme_cuts(transcript, audio, sr, f0)
    log.info("   → %d segment(s) proposed", len(segments))

    log.info("   Encoding cleaned WAV…")
    cleaned_io = io.BytesIO()
    AudioSegment(
        (audio * 32768).astype(np.int16).tobytes(),
        frame_rate=sr,
        sample_width=2,
        channels=1,
    ).export(cleaned_io, format="wav")

    raw_path.unlink(missing_ok=True)
    log.info("✔  Done — returning %.1f KB WAV + %d segments\n",
             len(cleaned_io.getvalue()) / 1024, len(segments))

    return JSONResponse({
        "cleaned_audio"    : cleaned_io.getvalue().hex(),
        "sample_rate"      : sr,
        "duration_ms"      : int(len(audio) / sr * 1000),
        "f0_contour"       : valid_f0.tolist(),
        "proposed_segments": segments,
    })


@app.post("/api/save-unit")
async def save_unit_endpoint(
    audio: UploadFile = File(...),  # ← Changed from bytes to UploadFile
    transcript: str = Form(...),
    morph_type: str = Form(...),
    grammatical_role: Optional[str] = Form(None),
    tone_pattern: str = Form(...),
    duration_ms: int = Form(...),
    f0_mean: Optional[float] = Form(None),
    f0_onset: Optional[float] = Form(None),
    f0_offset: Optional[float] = Form(None),
    preceding_context: Optional[str] = Form(None),
    following_context: Optional[str] = Form(None),
    quality_score: float = Form(0.5),
    tags: str = Form("[]"),
):
    log.info("   Saving unit: '%s'  type=%s  dur=%d ms", transcript, morph_type, duration_ms)
    
    # Read audio bytes from UploadFile
    audio_bytes = await audio.read()
    
    unit_data = {
        "transcript": transcript, "morph_type": morph_type,
        "grammatical_role": grammatical_role, "tone_pattern": tone_pattern,
        "duration_ms": duration_ms, "f0_mean": f0_mean,
        "f0_onset": f0_onset, "f0_offset": f0_offset,
        "preceding_context": preceding_context, "following_context": following_context,
        "quality_score": quality_score, "tags": json.loads(tags),
    }
    result = save_unit(unit_data, audio_bytes, UPLOAD_DIR)
    log.info("✔  Unit saved → %s", result.get("filename", "?"))
    return result


@app.get("/api/library")
async def library_search(q: str = "", morph_type: Optional[str] = None, tone: Optional[str] = None):
    return search_units(q, morph_type, tone)


@app.get("/api/unit/{filename}")
async def get_unit_file(filename: str):
    filepath = UPLOAD_DIR / filename
    if not filepath.exists():
        raise HTTPException(404, "Unit not found")
    return FileResponse(filepath, media_type="audio/wav")


# ✅ NEW: DELETE endpoint for library units
@app.delete("/api/unit/{filename}")
async def delete_unit(filename: str):
    """Delete a saved unit from disk and database"""
    log.info("🗑️  Deleting unit: %s", filename)
    
    filepath = UPLOAD_DIR / filename
    if not filepath.exists():
        raise HTTPException(404, detail=f"File not found: {filename}")
    
    try:
        # Delete file from disk
        filepath.unlink()
        log.info("   Deleted file: %s", filepath)
        
        # Remove from SQLite database (assuming db.py has delete_unit function)
        from db import delete_unit_from_db  # You'll need to add this to db.py
        delete_unit_from_db(filename)
        log.info("   Removed from database: %s", filename)
        
        return {"status": "deleted", "filename": filename}
    except Exception as e:
        log.error("   Delete failed: %s", e)
        raise HTTPException(500, detail=f"Delete failed: {str(e)}")


# ✅ FIXED: Synthesis endpoint - accepts FormData with 'units[]' fields
@app.post("/api/synthesize")
async def synthesize_utterance(
    units: List[str] = Form(...),  # ← Changed from unit_ids to units
    transcripts: List[str] = Form(default=[]),  # ← Optional transcripts array
    transition_method: str = Form("crossfade_25ms"),
):
    log.info("🔗 Synthesizing %d units with method: %s", len(units), transition_method)
    
    if not units or len(units) < 2:
        raise HTTPException(400, detail="At least 2 units required for synthesis")
    
    if transition_method not in TRANSITION_METHODS:
        raise HTTPException(400, detail=f"Unknown transition method: {transition_method}")
    
    # Validate all files exist
    for uid in units:
        filepath = UPLOAD_DIR / uid
        if not filepath.exists():
            raise HTTPException(404, detail=f"Unit not found: {uid}")
    
    try:
        join_func = TRANSITION_METHODS[transition_method]
        
        # Start with first unit
        result = AudioSegment.from_wav(UPLOAD_DIR / units[0])
        
        # Concatenate remaining units
        for i, uid in enumerate(units[1:], 1):
            next_seg = AudioSegment.from_wav(UPLOAD_DIR / uid)
            log.info("   Joining unit %d/%d: %s", i, len(units)-1, uid)
            result = join_func(result, next_seg)
        
        # Export result
        output_io = io.BytesIO()
        result.export(output_io, format="wav")
        audio_hex = output_io.getvalue().hex()
        
        log.info("✔  Synthesis complete: %d ms, %.1f KB", len(result), len(audio_hex)/2/1024)
        
        return JSONResponse({
            "audio_hex": audio_hex, 
            "duration_ms": len(result), 
            "method_used": transition_method,
            "units_count": len(units)
        })
    except Exception as e:
        log.error("   Synthesis failed: %s", e)
        raise HTTPException(500, detail=f"Synthesis failed: {str(e)}")


# ✅ FIXED: Compare endpoint - parses JSON string for methods list
@app.post("/api/compare-transitions")
async def compare_transitions(
    unit_a: str = Form(...), 
    unit_b: str = Form(...), 
    methods: str = Form(...)  # ← Receive as JSON string
):
    log.info("🔍 Comparing transitions for %s + %s", unit_a, unit_b)
    
    # Parse methods from JSON string
    try:
        methods_list = json.loads(methods) if isinstance(methods, str) else methods
    except json.JSONDecodeError:
        methods_list = [methods] if isinstance(methods, str) else methods
    
    # Validate files exist
    for uid in [unit_a, unit_b]:
        filepath = UPLOAD_DIR / uid
        if not filepath.exists():
            raise HTTPException(404, detail=f"Unit not found: {uid}")
    
    seg_a = AudioSegment.from_wav(UPLOAD_DIR / unit_a)
    seg_b = AudioSegment.from_wav(UPLOAD_DIR / unit_b)
    
    results = {}
    for method in methods_list:
        if method in TRANSITION_METHODS:
            try:
                log.info("   Testing method: %s", method)
                joined = TRANSITION_METHODS[method](seg_a, seg_b)
                io_buf = io.BytesIO()
                joined.export(io_buf, format="wav")
                results[method] = {
                    "audio_hex": io_buf.getvalue().hex(), 
                    "duration_ms": len(joined)
                }
            except Exception as e:
                log.warning("   Method %s failed: %s", method, e)
                results[method] = {"error": str(e)}
        else:
            log.warning("   Unknown method skipped: %s", method)
    
    return {"results": results, "units": [unit_a, unit_b]}


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="127.0.0.1", port=8000)