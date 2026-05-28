import numpy as np
from pydub import AudioSegment
import parselmouth

def crossfade_join(seg_a: AudioSegment, seg_b: AudioSegment, ms: int = 25):
    """Standard overlap-add crossfade"""
    if len(seg_a) < ms or len(seg_b) < ms:
        return seg_a + seg_b  # too short to crossfade
    
    fade_out = seg_a[-ms:].fade_out(ms)
    fade_in = seg_b[:ms].fade_in(ms)
    overlap = fade_out.overlay(fade_in)
    
    return seg_a[:-ms] + overlap + seg_b[ms:]

def pitch_matched_join(seg_a: AudioSegment, seg_b: AudioSegment, ms: int = 25, match_factor: float = 0.5):
    """Shift seg_b onset pitch toward seg_a offset before crossfading"""
    # Extract F0 at boundaries using Parselmouth
    def get_edge_pitch(seg: AudioSegment, edge: str = 'end', window_ms: int = 10):
        samples = np.array(seg.get_array_of_samples())
        if seg.channels == 2:
            samples = samples.reshape((-1, 2)).mean(axis=1)
        sr = seg.frame_rate
        window_samples = int(window_ms / 1000 * sr)
        
        if edge == 'end':
            chunk = samples[-window_samples:].astype(np.float32) / 32768.0
        else:
            chunk = samples[:window_samples].astype(np.float32) / 32768.0
        
        snd = parselmouth.Sound(chunk, sampling_frequency=sr)
        f0 = snd.to_pitch().selected_array['frequency']
        valid_f0 = f0[f0 > 0]
        return np.mean(valid_f0) if len(valid_f0) > 0 else 200.0  # fallback 200Hz
    
    pitch_a_end = get_edge_pitch(seg_a, 'end')
    pitch_b_start = get_edge_pitch(seg_b, 'start')
    
    # Calculate shift in semitones, apply partial correction
    semitone_shift = 12 * np.log2(pitch_a_end / pitch_b_start) * match_factor
    
    if abs(semitone_shift) > 0.1:  # only shift if meaningful
        # Use pydub's simple pitch shift (wrapper around sox)
        try:
            seg_b = seg_b._spawn(
                seg_b.raw_data,
                overrides={"frame_rate": int(seg_b.frame_rate * (2 ** (semitone_shift/12)))}
            ).set_frame_rate(seg_b.frame_rate)
        except:
            pass  # fallback to no shift if pitch shift fails
    
    return crossfade_join(seg_a, seg_b, ms)

def overlap_add_join(seg_a: AudioSegment, seg_b: AudioSegment, ms: int = 40):
    """Phase-aligned overlap-add for smoother joins (simplified)"""
    # Convert to numpy for phase manipulation
    samples_a = np.array(seg_a.get_array_of_samples()).astype(np.float32) / 32768.0
    samples_b = np.array(seg_b.get_array_of_samples()).astype(np.float32) / 32768.0
    
    if seg_a.channels == 2:
        samples_a = samples_a.reshape((-1, 2)).mean(axis=1)
        samples_b = samples_b.reshape((-1, 2)).mean(axis=1)
    
    sr = seg_a.frame_rate
    overlap_samples = int(ms / 1000 * sr)
    
    # Raised cosine window for smooth overlap
    window = np.hanning(overlap_samples * 2)
    
    # Align by minimizing phase discontinuity (simplified: just window)
    overlap_a = samples_a[-overlap_samples:] * window[:overlap_samples]
    overlap_b = samples_b[:overlap_samples] * window[overlap_samples:]
    
    joined_overlap = overlap_a + overlap_b
    
    # Reconstruct
    result = np.concatenate([
        samples_a[:-overlap_samples],
        joined_overlap,
        samples_b[overlap_samples:]
    ])
    
    # Convert back to AudioSegment
    result_int16 = np.int16(result * 32768)
    return AudioSegment(
        result_int16.tobytes(),
        frame_rate=sr,
        sample_width=2,
        channels=1
    )

# Method registry for frontend
TRANSITION_METHODS = {
    "crossfade_25ms": lambda a,b: crossfade_join(a,b,25),
    "crossfade_40ms": lambda a,b: crossfade_join(a,b,40), 
    "pitch_matched": lambda a,b: pitch_matched_join(a,b,25),
    "overlap_add": lambda a,b: overlap_add_join(a,b,40)
}