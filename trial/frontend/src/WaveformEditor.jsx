// frontend/src/WaveformEditor.jsx
import { useEffect, useRef, useState, useCallback } from 'react';
import WaveSurfer from 'wavesurfer.js';
import RegionsPlugin from 'wavesurfer.js/dist/plugins/regions.esm.js';
import './WaveformEditor.css';

function encodeWav(audioBuffer) {
  const sr = audioBuffer.sampleRate;
  const samples = audioBuffer.getChannelData(0);
  const dataSize = samples.length * 2;
  const buf = new ArrayBuffer(44 + dataSize);
  const v = new DataView(buf);
  const s = (o, x) => { for (let i = 0; i < x.length; i++) v.setUint8(o + i, x.charCodeAt(i)); };
  s(0,'RIFF'); v.setUint32(4,36+dataSize,true); s(8,'WAVE'); s(12,'fmt ');
  v.setUint32(16,16,true); v.setUint16(20,1,true); v.setUint16(22,1,true);
  v.setUint32(24,sr,true); v.setUint32(28,sr*2,true);
  v.setUint16(32,2,true);  v.setUint16(34,16,true);
  s(36,'data'); v.setUint32(40,dataSize,true);
  let off = 44;
  for (let i = 0; i < samples.length; i++) {
    const x = Math.max(-1, Math.min(1, samples[i]));
    v.setInt16(off, x < 0 ? x * 0x8000 : x * 0x7FFF, true);
    off += 2;
  }
  return new Blob([buf], { type: 'audio/wav' });
}

async function detectSilenceBounds(blob) {
  const arrBuf  = await blob.arrayBuffer();
  const ctx     = new AudioContext();
  const decoded = await ctx.decodeAudioData(arrBuf);
  await ctx.close();
  const data      = decoded.getChannelData(0);
  const frameSize = Math.floor(decoded.sampleRate * 0.01);
  const threshold = 0.015;
  let inSample = 0;
  for (let i = 0; i < data.length - frameSize; i += frameSize) {
    let rms = 0;
    for (let j = i; j < i + frameSize; j++) rms += data[j] * data[j];
    if (Math.sqrt(rms / frameSize) > threshold) { inSample = i; break; }
  }
  let outSample = data.length;
  for (let i = data.length - frameSize; i >= 0; i -= frameSize) {
    let rms = 0;
    for (let j = i; j < i + frameSize; j++) rms += data[j] * data[j];
    if (Math.sqrt(rms / frameSize) > threshold) { outSample = i + frameSize; break; }
  }
  return {
    inPoint:  inSample  / decoded.sampleRate,
    outPoint: outSample / decoded.sampleRate,
  };
}

const MORPH_TYPES = [
  { value: 'subject_prefix', label: 'Subject Prefix', color: '#5B7C99' },
  { value: 'tense_marker',   label: 'Tense Marker',   color: '#7B6B8E' },
  { value: 'verb_root',      label: 'Verb Root',      color: '#6B8E7B' },
  { value: 'suffix',         label: 'Suffix',         color: '#C4A35A' },
];

const STATUS_CONFIG = {
  idle:       { color: '#475569', label: 'No audio loaded'           },
  uploading:  { color: '#3b82f6', label: 'Uploading…'                },
  processing: { color: '#f59e0b', label: 'Processing audio…'         },
  ready:      { color: '#22c55e', label: 'Ready'                     },
  error:      { color: '#ef4444', label: 'Error'                     },
};

export default function WaveformEditor({
  audioBlob,
  sampleRate        = 16000,
  proposedSegments  = [],
  onSegmentAccepted,
  status            = 'idle',
  statusMessage     = '',
}) {
  const containerRef = useRef(null);
  const wrapRef      = useRef(null);
  const wsRef        = useRef(null);
  const regionsRef   = useRef(null);
  const blobRef      = useRef(null);
  const audioBufferRef = useRef(null);

  const [ready,      setReady]      = useState(false);
  const [playing,    setPlaying]    = useState(false);
  const [duration,   setDuration]   = useState(0);
  const [inPoint,    setInPoint]    = useState(0);
  const [outPoint,   setOutPoint]   = useState(null);
  const [regionList, setRegionList] = useState([]);
  const [activeId,   setActiveId]   = useState(null);
  const [dragging,   setDragging]   = useState(null);

  // ── Initialize WaveSurfer v7 ─────────────────────────────────────────────
  useEffect(() => {
    if (!containerRef.current || !audioBlob) return;
    blobRef.current = audioBlob;

    // Clean up previous instance
    wsRef.current?.destroy();
    wsRef.current = null;
    regionsRef.current = null;

    // Create WaveSurfer (default MediaElement backend is stable for regions)
    wsRef.current = WaveSurfer.create({
      container: containerRef.current,
      waveColor: '#1e3a5f',
      progressColor: '#C4A35A',
      cursorColor: '#f8fafc',
      cursorWidth: 2,
      height: 140,
      barWidth: 3,
      barGap: 1,
      barRadius: 3,
      normalize: true,
      minPxPerSec: 100,
      // Smooth cursor without drift
      autoScroll: true,
      hideScrollbar: false,
    });

    const wsRegions = RegionsPlugin.create();
    wsRef.current.registerPlugin(wsRegions);
    regionsRef.current = wsRegions;

    // Region events
    wsRegions.on('region-clicked', (region, e) => {
      e.stopPropagation();
      setActiveId(region.id);
    });

    wsRegions.on('region-updated', (region) => {
      setRegionList(prev => prev.map(r =>
        r.id === region.id ? { ...r, start: region.start, end: region.end } : r
      ));
    });

    // Playback events
    wsRef.current.on('play', () => setPlaying(true));
    wsRef.current.on('pause', () => setPlaying(false));
    wsRef.current.on('finish', () => setPlaying(false));

    // Native v7 ready event
    wsRef.current.on('ready', async () => {
      const dur = wsRef.current.getDuration();
      setDuration(dur);
      setInPoint(0);
      setOutPoint(dur);
      setReady(true);

      // Decode audio to get actual sample rate for slicing
      try {
        const arrayBuffer = await audioBlob.arrayBuffer();
        const audioContext = new (window.AudioContext || window.webkitAudioContext)();
        const decodedBuffer = await audioContext.decodeAudioData(arrayBuffer);
        await audioContext.close();
        audioBufferRef.current = decodedBuffer;
      } catch (err) {
        console.warn('Audio decode for slicing failed:', err);
      }

      // Add proposed regions
      const newList = [];
      proposedSegments.forEach((seg, idx) => {
        const mt = seg.morph_type ?? 'verb_root';
        const typeInfo = MORPH_TYPES.find(m => m.value === mt) ?? MORPH_TYPES[2];
        
        const region = wsRegions.addRegion({
          start: (seg.start_ms ?? 0) / 1000,
          end: (seg.end_ms ?? 1000) / 1000,
          color: typeInfo.color + '35',
          drag: true,
          resize: true,
          data: { morphType: mt, transcript: seg.transcript ?? '', f0_mean: seg.f0_mean ?? null },
        });
        
        if (region.element) {
          region.element.setAttribute('data-label', seg.transcript || `seg ${idx + 1}`);
        }
        
        newList.push({
          id: region.id,
          morphType: mt, 
          transcript: seg.transcript ?? '',
          start: region.start, 
          end: region.end,
          f0_mean: seg.f0_mean ?? null,
          _wsRegion: region,
        });
      });
      setRegionList(newList);

      // Auto-trim silence
      try {
        const bounds = await detectSilenceBounds(audioBlob);
        setInPoint(bounds.inPoint);
        setOutPoint(bounds.outPoint);
      } catch (e) {
        console.warn('Auto-trim failed:', e);
      }
    });

    wsRef.current.loadBlob(audioBlob);

    return () => {
      wsRef.current?.destroy();
      wsRef.current = null;
      regionsRef.current = null;
      audioBufferRef.current = null;
      setReady(false); 
      setPlaying(false);
      setRegionList([]); 
      setActiveId(null);
    };
  }, [audioBlob, proposedSegments]);

  // ── Marker drag handlers ────────────────────────────────────────────────
  const startDragIn = useCallback((e) => { 
    e.preventDefault(); 
    e.stopPropagation(); 
    setDragging('in');  
  }, []);
  
  const startDragOut = useCallback((e) => { 
    e.preventDefault(); 
    e.stopPropagation(); 
    setDragging('out'); 
  }, []);

  const handleMouseMove = useCallback((e) => {
    if (!dragging || !wrapRef.current || !duration) return;
    const rect = wrapRef.current.getBoundingClientRect();
    const t = Math.max(0, Math.min(duration, ((e.clientX - rect.left) / rect.width) * duration));
    if (dragging === 'in') {
      setInPoint(prev => Math.min(t, (outPoint ?? duration) - 0.05));
    } else {
      setOutPoint(prev => Math.max(t, inPoint + 0.05));
    }
  }, [dragging, duration, inPoint, outPoint]);

  const stopDrag = useCallback(() => setDragging(null), []);

  // ── Region mutations (IMMUTABLE) ────────────────────────────────────────
  const updateMorphType = useCallback((id, morphType) => {
    const ti = MORPH_TYPES.find(m => m.value === morphType) ?? MORPH_TYPES[2];
    setRegionList(prev => prev.map(r => {
      if (r.id !== id) return r;
      if (r._wsRegion) {
        r._wsRegion.setOptions({ color: ti.color + '35' });
        r._wsRegion.data = { ...r._wsRegion.data, morphType };
      }
      return { ...r, morphType };
    }));
  }, []);

  const updateTranscript = useCallback((id, transcript) => {
    setRegionList(prev => prev.map(r => {
      if (r.id !== id) return r;
      if (r._wsRegion) {
        r._wsRegion.data = { ...r._wsRegion.data, transcript };
      }
      if (r._wsRegion?.element) {
        r._wsRegion.element.setAttribute('data-label', transcript || `seg`);
      }
      return { ...r, transcript };
    }));
  }, []);

  const previewRegion = useCallback((r) => {
    if (wsRef.current && r.start < r.end) {
      wsRef.current.play(r.start, r.end);
    }
  }, []);

  const saveRegion = useCallback(async (regionId) => {
    if (!blobRef.current || !audioBufferRef.current) {
      console.error('No audio buffer available');
      return;
    }
    
    const currentRegion = regionList.find(r => r.id === regionId);
    if (!currentRegion || !currentRegion._wsRegion) {
      console.error('Region not found:', regionId);
      return;
    }
    
    try {
      const decoded = audioBufferRef.current;
      const sr2 = decoded.sampleRate;
      const s0 = Math.max(0, Math.floor(currentRegion.start * sr2));
      const s1 = Math.min(decoded.length, Math.floor(currentRegion.end * sr2));
      
      if (s1 <= s0) {
        console.warn('Invalid region bounds');
        return;
      }
      
      const offlineCtx = new AudioContext({ sampleRate: sr2 });
      const sliced = offlineCtx.createBuffer(1, s1 - s0, sr2);
      sliced.copyToChannel(decoded.getChannelData(0).slice(s0, s1), 0);
      await offlineCtx.close();
      
      console.log('💾 Saving region:', {
        id: regionId,
        transcript: currentRegion.transcript,
        morphType: currentRegion.morphType,
        start_ms: Math.round(currentRegion.start * 1000),
        end_ms: Math.round(currentRegion.end * 1000),
      });
      
      onSegmentAccepted?.({
        transcript: currentRegion.transcript || '',
        morphType: currentRegion.morphType || 'verb_root',
        f0_mean: currentRegion._wsRegion.data.f0_mean ?? null,
        audioBlob: encodeWav(sliced),
        start_ms: Math.round(currentRegion.start * 1000),
        end_ms: Math.round(currentRegion.end * 1000),
        duration_ms: Math.round((currentRegion.end - currentRegion.start) * 1000),
      });
    } catch (err) {
      console.error('Save region failed:', err);
    }
  }, [regionList, onSegmentAccepted]);

  const deleteRegion = useCallback((id) => {
    setRegionList(prev => {
      const region = prev.find(r => r.id === id);
      if (region?._wsRegion) region._wsRegion.remove();
      return prev.filter(r => r.id !== id);
    });
    setActiveId(a => a === id ? null : a);
  }, []);

  const manualTrim = useCallback(async () => {
    if (!blobRef.current) return;
    try {
      const bounds = await detectSilenceBounds(blobRef.current);
      setInPoint(bounds.inPoint);
      setOutPoint(bounds.outPoint);
    } catch (e) { console.warn('Trim failed:', e); }
  }, []);

  const setInToCurrentTime = useCallback(() => {
    const t = wsRef.current?.getCurrentTime() ?? 0;
    setInPoint(prev => Math.min(t, (outPoint ?? duration) - 0.05));
  }, [outPoint, duration]);

  const setOutToCurrentTime = useCallback(() => {
    const t = wsRef.current?.getCurrentTime() ?? 0;
    setOutPoint(prev => Math.max(t, inPoint + 0.05));
  }, [inPoint]);

  // ── Render ─────────────────────────────────────────────────────────────
  const sc = STATUS_CONFIG[status] ?? STATUS_CONFIG.idle;
  const outSec = outPoint ?? duration;
  const spin = status === 'uploading' || status === 'processing';
  const pct = (t) => `${duration ? (t / duration) * 100 : 0}%`;

  return (
    <div
      className="we-shell"
      onMouseMove={handleMouseMove}
      onMouseUp={stopDrag}
      onMouseLeave={stopDrag}
    >
      {/* Status */}
      <div className="we-status" style={{ '--sc': sc.color }}>
        <span className={`we-status__dot ${spin ? 'we-status__dot--pulse' : ''}`} />
        <span className="we-status__text">{statusMessage || sc.label}</span>
        {spin && <span className="we-status__spinner" />}
      </div>

      {/* Waveform */}
      <div ref={wrapRef} className="we-waveform-wrap">
        <div ref={containerRef} className="we-waveform" />

        {ready && (
          <>
            <div
              className="we-marker we-marker--in"
              style={{ left: pct(inPoint) }}
              onMouseDown={startDragIn}
              title="Drag to adjust IN point"
            >
              <div className="we-marker__handle" />
              <div className="we-marker__label">IN</div>
            </div>

            <div
              className="we-marker we-marker--out"
              style={{ left: pct(outSec) }}
              onMouseDown={startDragOut}
              title="Drag to adjust OUT point"
            >
              <div className="we-marker__handle" />
              <div className="we-marker__label">OUT</div>
            </div>
          </>
        )}
      </div>

      {/* Toolbar */}
      <div className="we-toolbar">
        <div className="we-toolbar__group">
          <button className="we-btn" disabled={!ready}
            onClick={() => wsRef.current?.playPause()}>
            {playing ? '⏸ Pause' : '▶ Play'}
          </button>
          <button className="we-btn" disabled={!ready}
            onClick={() => { wsRef.current?.stop(); setPlaying(false); }}>
            ⏹ Stop
          </button>
        </div>

        <div className="we-toolbar__group">
          <button className="we-btn we-btn--tool" disabled={!ready} onClick={manualTrim}>
            ✂ Auto-Trim
          </button>
          <button className="we-btn we-btn--tool" disabled={!ready} onClick={setInToCurrentTime}>
            ⟦ Set IN
          </button>
          <button className="we-btn we-btn--tool" disabled={!ready} onClick={setOutToCurrentTime}>
            Set OUT ⟧
          </button>
        </div>

        {ready && (
          <div className="we-toolbar__meta">
            <span>IN <strong>{inPoint.toFixed(2)}s</strong></span>
            <span>OUT <strong>{outSec.toFixed(2)}s</strong></span>
            <span>Sel <strong>{(outSec - inPoint).toFixed(2)}s</strong></span>
          </div>
        )}
      </div>

      {/* Segments Panel */}
      <div className="we-segments">
        <div className="we-segments__header">
          <h4>Segments</h4>
          <span className="we-segments__count">{regionList.length}</span>
        </div>

        {ready && regionList.length === 0 && (
          <p className="we-segments__empty">No segments detected. Try re-uploading with a transcript.</p>
        )}

        <div className="we-segments__list">
          {regionList.map((r, idx) => {
            const ti = MORPH_TYPES.find(m => m.value === r.morphType) ?? MORPH_TYPES[2];
            return (
              <div
                key={r.id}
                className={`we-seg ${activeId === r.id ? 'we-seg--active' : ''}`}
                style={{ '--seg-color': ti.color }}
                onClick={() => setActiveId(r.id)}
              >
                <div className="we-seg__idx">{idx + 1}</div>

                <select
                  className="we-seg__select"
                  value={r.morphType}
                  onChange={e => {
                    e.stopPropagation();
                    updateMorphType(r.id, e.target.value);
                  }}
                  onClick={e => e.stopPropagation()}
                >
                  {MORPH_TYPES.map(m => (
                    <option key={m.value} value={m.value}>{m.label}</option>
                  ))}
                </select>

                <input
                  className="we-seg__input"
                  type="text"
                  value={r.transcript}
                  placeholder={`Name segment ${idx + 1}…`}
                  onChange={e => {
                    e.stopPropagation();
                    updateTranscript(r.id, e.target.value);
                  }}
                  onClick={e => e.stopPropagation()}
                />

                <div className="we-seg__times">
                  <span>{Math.round(r.start * 1000)}</span>
                  <span className="we-seg__arrow">→</span>
                  <span>{Math.round(r.end * 1000)} ms</span>
                  <span className="we-seg__dur">({Math.round((r.end - r.start) * 1000)}ms)</span>
                  {r.f0_mean && <span className="we-seg__f0">• {Math.round(r.f0_mean)}Hz</span>}
                </div>

                <div className="we-seg__actions" onClick={e => e.stopPropagation()}>
                  <button title="Preview" onClick={() => previewRegion(r)}>▶</button>
                  <button 
                    title="Save unit" 
                    onClick={() => saveRegion(r.id)}
                    className="we-seg__save"
                  >
                    💾
                  </button>
                  <button title="Delete" onClick={() => deleteRegion(r.id)} className="we-seg__del">✕</button>
                </div>
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
}