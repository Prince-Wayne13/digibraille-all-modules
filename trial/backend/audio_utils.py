# backend/audio_utils.py  ← replace the entire file
import io
from pathlib import Path

import numpy as np
import noisereduce as nr
from pydub import AudioSegment
from scipy import signal

_EXT_FORMAT = {
    ".wav" : "wav",
    ".mp3" : "mp3",
    ".m4a" : "mp4",
    ".mp4" : "mp4",
    ".ogg" : "ogg",
    ".oga" : "ogg",
    ".webm": "webm",
    ".flac": "flac",
    ".aac" : "aac",
}


def load_and_clean(filepath: str) -> tuple[np.ndarray, int]:
    path = Path(filepath)
    fmt  = _EXT_FORMAT.get(path.suffix.lower())

    seg = (
        AudioSegment.from_file(filepath, format=fmt)
        if fmt else
        AudioSegment.from_file(filepath)
    )

    seg   = seg.set_channels(1).set_frame_rate(16000).set_sample_width(2)
    audio = np.array(seg.get_array_of_samples(), dtype=np.float32) / 32768.0
    sr    = 16000

    b, a  = signal.butter(4, 80 / (sr / 2), btype="high")
    audio = signal.filtfilt(b, a, audio)

    audio = nr.reduce_noise(y=audio, sr=sr, stationary=False, prop_decrease=0.85)
    audio = np.append(audio[0], audio[1:] - 0.97 * audio[:-1]).astype(np.float32)

    peak = np.max(np.abs(audio))
    if peak > 0:
        audio = audio / peak * 0.95

    return audio.astype(np.float32), sr


def extract_f0(audio: np.ndarray, sr: int, hop_ms: int = 10) -> np.ndarray:
    hop        = int(sr * hop_ms / 1000)
    frame_len  = int(sr * 0.025)
    min_period = int(sr / 400)
    max_period = int(sr / 60)
    n_frames   = max(1, (len(audio) - frame_len) // hop + 1)
    f0         = np.full(n_frames, np.nan)
    window     = np.hanning(frame_len)

    for i in range(n_frames):
        frame = audio[i * hop : i * hop + frame_len]
        if len(frame) < frame_len:
            break
        corr     = np.correlate(frame * window, frame * window, mode="full")
        corr     = corr[len(corr) // 2:]
        if max_period >= len(corr):
            continue
        peak_idx = np.argmax(corr[min_period:max_period]) + min_period
        if corr[0] > 0 and corr[peak_idx] / corr[0] > 0.3:
            f0[i] = sr / peak_idx

    return f0


def propose_morpheme_cuts(
    transcript: str,
    audio: np.ndarray,
    sr: int,
    f0: np.ndarray,
    frame_ms: int = 10,
) -> list[dict]:
    hop    = int(sr * frame_ms / 1000)
    n      = len(audio) // hop
    energy = np.array([np.sum(audio[i*hop:(i+1)*hop]**2) for i in range(n)], dtype=np.float32)
    energy = np.convolve(energy, np.ones(5) / 5, mode="same")
    is_sp  = energy > np.max(energy) * 0.04

    gap = int(30 / frame_ms)
    for i in range(1, len(is_sp) - 1):
        if not is_sp[i] and np.any(is_sp[max(0, i-gap):i]) and np.any(is_sp[i+1:i+gap+1]):
            is_sp[i] = True

    boundaries, in_sp, start = [], False, 0
    for i, sp in enumerate(is_sp):
        if sp and not in_sp:
            start, in_sp = i, True
        elif not sp and in_sp:
            if (i - start) * frame_ms >= 40:
                boundaries.append((start, i))
            in_sp = False
    if in_sp and (len(is_sp) - start) * frame_ms >= 40:
        boundaries.append((start, len(is_sp)))

    words     = transcript.split() if transcript else []
    morph_seq = ["subject_prefix", "tense_marker", "verb_root", "suffix"]

    return [
        {
            "start_ms"   : s * frame_ms,
            "end_ms"     : e * frame_ms,
            "duration_ms": (e - s) * frame_ms,
            "transcript" : words[idx] if idx < len(words) else "",
            "morph_type" : morph_seq[idx % len(morph_seq)],
            "f0_mean"    : float(np.mean(f0[s:e][~np.isnan(f0[s:e])])) if len(f0[s:e][~np.isnan(f0[s:e])]) else None,
            "f0_onset"   : float(f0[s:e][~np.isnan(f0[s:e])][0])       if len(f0[s:e][~np.isnan(f0[s:e])]) else None,
            "f0_offset"  : float(f0[s:e][~np.isnan(f0[s:e])][-1])      if len(f0[s:e][~np.isnan(f0[s:e])]) else None,
            "confidence" : round(float(np.mean(energy[s:e]) / (np.max(energy) + 1e-9)), 3),
        }
        for idx, (s, e) in enumerate(boundaries)
    ]