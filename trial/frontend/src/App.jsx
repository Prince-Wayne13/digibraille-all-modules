// frontend/src/App.jsx
import { useState, useEffect } from 'react';
import axios from 'axios';
import WaveformEditor from './WaveformEditor';
import SynthesisLab from './SynthesisLab';
import { startRecording } from './utils/mic';

export default function App() {
  const [recordingState, setRecordingState] = useState('idle');
  const [editorStatus,   setEditorStatus]   = useState('idle');
  const [statusMsg,      setStatusMsg]      = useState('');
  const [audioData,      setAudioData]      = useState(null);
  const [libraryUnits,   setLibraryUnits]   = useState([]);
  const [micError,       setMicError]       = useState(null);
  const [transcriptModal,setTranscriptModal]= useState(false);
  const [pendingBlob,    setPendingBlob]    = useState(null);
  const [transcript,     setTranscript]     = useState('');
  const [toast,          setToast]          = useState(null);
  const [activeView,     setActiveView]     = useState('editor');

  const fetchLibrary = async () => {
    try {
      const res = await axios.get('/api/library');
      setLibraryUnits(res.data);
    } catch (err) {
      console.warn('Library fetch failed:', err);
    }
  };

  useEffect(() => { fetchLibrary(); }, []);

  const showToast = (message, type = 'success') => {
    setToast({ message, type });
    setTimeout(() => setToast(null), 2600);
  };

  // ── Recording ─────────────────────────────────────────────────────────────
  const handleStartRecording = async () => {
    setMicError(null);
    const recorder = await startRecording();
    if (recorder.error) { setMicError(recorder.error); return; }
    setRecordingState('recording');
    recorder.start();
    const timeout = setTimeout(() => handleStopRecording(recorder), 30000);
    window.currentRecorder = { recorder, timeout };
  };

  const handleStopRecording = async (recorder) => {
    clearTimeout(window.currentRecorder?.timeout);
    try {
      const audioBlob = await recorder.stop();
      setPendingBlob(audioBlob);
      setTranscriptModal(true);
      setRecordingState('idle');
    } catch (err) {
      console.error(err);
      setMicError('Failed to stop recording');
      setRecordingState('idle');
    }
  };

  // ── Process audio (shared by record + upload) ─────────────────────────────
  const processAudio = async (blob, text) => {
    try {
      setRecordingState('processing');
      setEditorStatus('uploading');
      setStatusMsg('Uploading audio…');

      const formData = new FormData();
      formData.append('file', blob, blob.name ?? 'recording.webm');
      formData.append('transcript', text);

      setEditorStatus('processing');
      setStatusMsg('Cleaning waveform & detecting morpheme regions…');

      const response = await axios.post('/api/record', formData);

      setStatusMsg('Decoding cleaned audio…');
      const audioBytes = new Uint8Array(
        response.data.cleaned_audio.match(/.{1,2}/g).map(b => parseInt(b, 16))
      );
      const cleanedBlob = new Blob([audioBytes], { type: 'audio/wav' });

      setAudioData({
        audioBlob:        cleanedBlob,
        sampleRate:       response.data.sample_rate,
        proposedSegments: response.data.proposed_segments,
        transcript:       text,
      });

      setRecordingState('ready');
      setEditorStatus('ready');
      setStatusMsg(
        `${response.data.proposed_segments.length} segment(s) detected — ` +
        `${(response.data.duration_ms / 1000).toFixed(2)}s`
      );
      showToast('Audio processed successfully');
    } catch (err) {
      console.error(err);
      setMicError(err.message);
      setRecordingState('idle');
      setEditorStatus('error');
      setStatusMsg(err.message);
    }
  };

  const submitTranscript = async () => {
    if (!transcript.trim()) return;
    setTranscriptModal(false);
    await processAudio(pendingBlob, transcript);
    setTranscript('');
    setPendingBlob(null);
  };

  const handleUpload = (e) => {
    const file = e.target.files?.[0];
    if (!file) return;
    e.target.value = '';
    setPendingBlob(file);
    setTranscriptModal(true);
  };

  // ── Save unit ─────────────────────────────────────────────────────────────
  const handleSaveUnit = async (unitData) => {
    try {
      const formData = new FormData();
      formData.append('audio',            unitData.audioBlob);
      formData.append('transcript',       unitData.transcript  ?? '');
      formData.append('morph_type',       unitData.morphType   ?? 'verb_root');
      formData.append('grammatical_role', unitData.grammaticalRole ?? '');
      formData.append('tone_pattern',     unitData.tone        ?? 'M');
      formData.append('duration_ms',      unitData.duration_ms ?? 0);
      await axios.post('/api/save-unit', formData);
      await fetchLibrary();
      showToast('Unit saved');
    } catch (err) {
      console.error(err);
      showToast('Failed to save unit', 'error');
    }
  };

  // ── DELETE Library Unit ───────────────────────────────────────────────────
  const handleDeleteLibraryUnit = async (unitId, filename) => {
    if (!filename) {
      showToast('Cannot delete: missing filename', 'error');
      return;
    }
    if (!confirm(`Delete "${unitId}" permanently?`)) return;
    try {
      await axios.delete(`/api/unit/${filename}`);
      await fetchLibrary();
      showToast('Unit deleted');
    } catch (err) {
      console.error('Delete failed:', err);
      showToast(err.response?.data?.detail || 'Delete failed', 'error');
    }
  };

  // ── Render ────────────────────────────────────────────────────────────────
  return (
    <div className="app-shell">
      {/* SIDEBAR */}
      <aside className="sidebar">
        <div className="brand">
          <div className="brand-icon">◉</div>
          <div>
            <h1>Chichewa Lab</h1>
            <p>Audio Morphology Workstation</p>
          </div>
        </div>

        <div className="sidebar-group">
          <span className="sidebar-label">Workspace</span>
          <button
            className={`sidebar-item ${activeView === 'editor' ? 'active' : ''}`}
            onClick={() => setActiveView('editor')}
          >Waveform Editor</button>
          <button
            className={`sidebar-item ${activeView === 'synthesis' ? 'active' : ''}`}
            onClick={() => setActiveView('synthesis')}
          >Synthesis Lab</button>
          <button
            className={`sidebar-item ${activeView === 'library' ? 'active' : ''}`}
            onClick={() => setActiveView('library')}
          >Unit Library</button>
        </div>

        <div className="sidebar-group">
          <span className="sidebar-label">Library Units</span>
          <div className="library-scroll">
            {libraryUnits?.slice(0, 10).map((unit, i) => (
              <div className="library-card" key={unit.id || unit.filename || i}>
                <div className="library-title">{unit.transcript || 'Untitled'}</div>
                <div className="library-meta">{unit.morph_type}</div>
              </div>
            ))}
          </div>
        </div>
      </aside>

      {/* MAIN */}
      <main className="workspace">
        <header className="topbar">
          <div>
            <h2>Waveform Workspace</h2>
            <p>Segment • Tag • Analyze • Synthesize</p>
          </div>
          <div className="topbar-actions">
            <button
              className={`record-btn ${recordingState === 'recording' ? 'recording' : ''}`}
              disabled={recordingState === 'processing'}
              onClick={
                recordingState === 'recording'
                  ? () => handleStopRecording(window.currentRecorder?.recorder)
                  : handleStartRecording
              }
            >
              {recordingState === 'recording' ? '⏹ Stop Recording' : '⏺ Start Recording'}
            </button>

            <label className={`upload-btn ${recordingState === 'processing' ? 'disabled' : ''}`}>
              ↑ Upload Audio
              <input
                type="file"
                accept=".wav,.mp3,.m4a,.mp4,.ogg,.flac,.webm,audio/*"
                hidden
                onChange={handleUpload}
                disabled={recordingState === 'processing'}
              />
            </label>
          </div>
        </header>

        {micError && <div className="error-banner">{micError}</div>}

        <div className="editor-layout">
          <section className="editor-panel">
            <WaveformEditor
              audioBlob={audioData?.audioBlob ?? null}
              sampleRate={audioData?.sampleRate ?? 16000}
              proposedSegments={audioData?.proposedSegments ?? []}
              onSegmentAccepted={handleSaveUnit}
              status={editorStatus}
              statusMessage={statusMsg}
            />
          </section>

          <aside className="context-panel">
            <div className="panel-card">
              <div className="panel-header">Session</div>
              <div className="session-grid">
                <div><span>Status</span><strong>{recordingState}</strong></div>
                <div><span>Library Units</span><strong>{libraryUnits?.length || 0}</strong></div>
                {audioData && (
                  <div><span>Segments</span><strong>{audioData.proposedSegments?.length ?? 0}</strong></div>
                )}
              </div>
            </div>
            <div className="panel-card">
              <div className="panel-header">Notes</div>
              <p className="panel-note">
                Prefixes, tense markers, roots, and suffixes should remain reusable independent units.
              </p>
            </div>
          </aside>
        </div>

        {/* ✅ Synthesis View - FIXED: proper closing bracket */}
        {activeView === 'synthesis' && (
          <section className="synthesis-section" style={{ marginTop: '16px' }}>
            <SynthesisLab libraryUnits={libraryUnits} />
          </section>
        )}

        {/* ✅ Library View - FIXED: at same level as synthesis, not nested */}
        {activeView === 'library' && (
          <section style={{ 
            padding: '24px', background: '#0b0e18', borderRadius: '12px', 
            marginTop: '16px', color: '#e2e8f0' 
          }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '16px' }}>
              <h3 style={{ margin: 0 }}>Unit Library ({libraryUnits?.length || 0})</h3>
              <button className="we-btn" onClick={fetchLibrary} style={{ fontSize: '0.75rem' }}>🔄 Refresh</button>
            </div>

            {libraryUnits?.length === 0 ? (
              <p style={{ color: '#64748b', textAlign: 'center' }}>No units saved yet</p>
            ) : (
              <div style={{ 
                display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(280px, 1fr))', 
                gap: '12px' 
              }}>
                {libraryUnits.map((unit) => {
                  const unitId = unit.id ?? unit.filename ?? `unit-${Math.random()}`;
                  return (
                    <div key={unitId} style={{
                      background: '#141b2e', border: '1px solid #1e2535', 
                      borderRadius: '8px', padding: '12px', display: 'flex', 
                      flexDirection: 'column', gap: '8px'
                    }}>
                      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'start' }}>
                        <div>
                          <div style={{ fontWeight: 600 }}>{unit.transcript || 'Untitled'}</div>
                          <div style={{ fontSize: '0.75rem', color: '#64748b' }}>
                            {unit.morph_type} • {unit.duration_ms}ms
                          </div>
                        </div>
                        <button 
                          onClick={(e) => { e.stopPropagation(); handleDeleteLibraryUnit(unitId, unit.filename); }}
                          style={{ background: 'none', border: 'none', color: '#ef4444', cursor: 'pointer', fontSize: '1.2rem' }}
                          title="Delete unit"
                        >🗑️</button>
                      </div>
                      <audio 
                        src={`/api/unit/${unit.filename}`} 
                        controls 
                        style={{ width: '100%', height: '32px', background: '#0b0e18' }}
                        preload="none"
                      />
                    </div>
                  );
                })}
              </div>
            )}
          </section>
        )}

      </main>

      {/* TRANSCRIPT MODAL */}
      {transcriptModal && (
        <div className="modal-overlay">
          <div className="modal-card">
            <h3>Enter Transcript</h3>
            <p>Provide the spoken Chichewa phrase for this audio.</p>
            <input
              value={transcript}
              onChange={e => setTranscript(e.target.value)}
              onKeyDown={e => e.key === 'Enter' && submitTranscript()}
              placeholder="e.g. ndikupita"
              className="transcript-input"
              autoFocus
            />
            <div className="modal-actions">
              <button className="secondary-btn" onClick={() => setTranscriptModal(false)}>Cancel</button>
              <button className="primary-btn" onClick={submitTranscript}>Process Audio</button>
            </div>
          </div>
        </div>
      )}

      {/* TOAST */}
      {toast && (
        <div className={`toast ${toast.type === 'error' ? 'toast-error' : ''}`}>
          {toast.message}
        </div>
      )}
    </div>
  );
}