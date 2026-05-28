import sqlite3
from pathlib import Path
from datetime import datetime

DB_PATH = Path(__file__).parent / "units.db"

def init_db():
    conn = sqlite3.connect(DB_PATH)
    conn.execute("""
        CREATE TABLE IF NOT EXISTS units (
            id TEXT PRIMARY KEY,
            filename TEXT NOT NULL,
            transcript TEXT NOT NULL,
            morph_type TEXT NOT NULL,  -- subject_prefix|tense_marker|subject_tense_cluster|verb_root|suffix|final_vowel
            grammatical_role TEXT,     -- e.g. "1sg_subject", "present_continuous"
            tone_pattern TEXT,         -- H|L|HL|LH|M
            duration_ms INTEGER,
            f0_mean REAL,
            f0_onset REAL,
            f0_offset REAL,
            preceding_context TEXT,
            following_context TEXT,
            quality_score REAL DEFAULT 0.5,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            tags TEXT  -- JSON array of custom tags
        )
    """)
    conn.execute("CREATE INDEX IF NOT EXISTS idx_transcript ON units(transcript)")
    conn.execute("CREATE INDEX IF NOT EXISTS idx_morph ON units(morph_type)")
    conn.commit()
    conn.close()

def save_unit(unit_data: dict, audio_bytes: bytes, upload_dir: Path):
    """Save unit with structured naming: {transcript}_{morphType}_{tone}_{timestamp}.wav"""
    import hashlib, json
    from uuid import uuid4
    
    # Generate deterministic but unique ID
    content_hash = hashlib.sha1(audio_bytes).hexdigest()[:8]
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    # Structured filename
    filename = f"{unit_data['transcript']}_{unit_data['morph_type']}_{unit_data['tone_pattern']}_{content_hash}.wav"
    filepath = upload_dir / filename
    
    # Save audio
    with open(filepath, 'wb') as f:
        f.write(audio_bytes)
    
    # Save metadata to DB
    conn = sqlite3.connect(DB_PATH)
    conn.execute("""
        INSERT OR REPLACE INTO units 
        (id, filename, transcript, morph_type, grammatical_role, tone_pattern, 
         duration_ms, f0_mean, f0_onset, f0_offset, preceding_context, 
         following_context, quality_score, tags)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, (
        f"{unit_data['transcript']}_{unit_data['morph_type']}_{unit_data['tone_pattern']}_{content_hash}",
        filename,
        unit_data['transcript'],
        unit_data['morph_type'],
        unit_data.get('grammatical_role'),
        unit_data['tone_pattern'],
        unit_data['duration_ms'],
        unit_data.get('f0_mean'),
        unit_data.get('f0_onset'),
        unit_data.get('f0_offset'),
        unit_data.get('preceding_context'),
        unit_data.get('following_context'),
        unit_data.get('quality_score', 0.5),
        json.dumps(unit_data.get('tags', []))
    ))
    conn.commit()
    conn.close()
    
    return {"id": filename, "path": str(filepath)}

def search_units(query: str, morph_type: str = None, tone: str = None):
    """Search library with filters"""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    sql = "SELECT * FROM units WHERE transcript LIKE ?"
    params = [f"%{query}%"]
    
    if morph_type:
        sql += " AND morph_type = ?"
        params.append(morph_type)
    if tone:
        sql += " AND tone_pattern = ?"
        params.append(tone)
    
    sql += " ORDER BY quality_score DESC, created_at DESC LIMIT 50"
    
    results = [dict(row) for row in conn.execute(sql, params).fetchall()]
    conn.close()
    return results
# backend/db.py - ADD THIS FUNCTION
def delete_unit_from_db(filename: str):
    """Remove a unit record from SQLite by filename"""
    import sqlite3
    conn = sqlite3.connect("units.db")
    try:
        conn.execute("DELETE FROM units WHERE filename = ?", (filename,))
        conn.commit()
        return True
    except Exception as e:
        print(f"DB delete error: {e}")
        return False
    finally:
        conn.close()