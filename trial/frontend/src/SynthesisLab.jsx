// frontend/src/SynthesisLab.jsx
import { useState } from 'react';
import axios from 'axios';

export default function SynthesisLab({ libraryUnits }) {
  const [selectedUnits, setSelectedUnits] = useState([]);
  const [transitionMethod, setTransitionMethod] = useState('crossfade_25ms');
  const [comparisonResults, setComparisonResults] = useState({});
  const [isGenerating, setIsGenerating] = useState(false);
  const [errorMsg, setErrorMsg] = useState('');
  
  const TRANSITION_OPTIONS = [
    { value: 'crossfade_25ms', label: 'Crossfade 25ms (morpheme boundary)' },
    { value: 'crossfade_40ms', label: 'Crossfade 40ms (word boundary)' },
    { value: 'pitch_matched', label: 'Pitch-Matched Join (best for tones)' },
    { value: 'overlap_add', label: 'Overlap-Add (smoothest energy)' }
  ];

  const handleAddUnit = (unit) => {
    if (!selectedUnits.find(u => u.id === unit.id || u.filename === unit.filename)) {
      setSelectedUnits([...selectedUnits, unit]);
    }
  };

  const handleRemoveUnit = (unitId, filename) => {
    setSelectedUnits(prev => prev.filter(u => 
      (u.id && u.id !== unitId) || (u.filename && u.filename !== filename)
    ));
  };

  const hexToBlob = (hexString) => {
    try {
      const bytes = new Uint8Array(
        hexString.match(/.{1,2}/g)?.map(byte => parseInt(byte, 16)) || []
      );
      return new Blob([bytes], { type: 'audio/wav' });
    } catch (e) {
      console.error('Hex decode failed:', e);
      return null;
    }
  };

  const playAudioBlob = (blob) => {
    try {
      const url = URL.createObjectURL(blob);
      const audio = new Audio(url);
      audio.play().catch(err => console.warn('Playback failed:', err));
      // Clean up object URL after playback
      audio.onended = () => URL.revokeObjectURL(url);
    } catch (e) {
      console.error('Play failed:', e);
    }
  };

  const handleGenerate = async () => {
    if (selectedUnits.length < 2) {
      setErrorMsg('Select at least 2 units to concatenate');
      return;
    }
    
    setErrorMsg('');
    setIsGenerating(true);
    
    try {
      const formData = new FormData();
      selectedUnits.forEach((unit, idx) => {
        // Send each unit as separate field (backend expects this)
        formData.append(`units`, unit.filename || unit.id);
        formData.append(`transcripts`, unit.transcript || '');
      });
      formData.append('transition_method', transitionMethod);

      const response = await axios.post('/api/synthesize', formData, {
        headers: { 'Content-Type': 'multipart/form-data' },
        responseType: 'json' // Backend returns JSON with audio_hex or blob URL
      });
      
      // Handle both hex and direct blob responses
      let audioBlob = null;
      if (response.data.audio_hex) {
        audioBlob = hexToBlob(response.data.audio_hex);
      } else if (response.data.audio_url) {
        // Backend returned a URL to fetch
        const blobRes = await axios.get(response.data.audio_url, { responseType: 'blob' });
        audioBlob = blobRes.data;
      }
      
      if (audioBlob) {
        playAudioBlob(audioBlob);
        setErrorMsg('');
      } else {
        setErrorMsg('No audio data received from server');
      }
      
    } catch (err) {
      console.error("Synthesis failed:", err);
      setErrorMsg(err.response?.data?.detail || 'Failed to generate audio. Check console.');
    } finally {
      setIsGenerating(false);
    }
  };

  const handleCompare = async () => {
    if (selectedUnits.length < 2) {
      setErrorMsg('Select at least 2 units to compare');
      return;
    }
    
    setErrorMsg('');
    setIsGenerating(true);
    
    try {
      const formData = new FormData();
      formData.append('unit_a', selectedUnits[0].filename || selectedUnits[0].id);
      formData.append('unit_b', selectedUnits[1].filename || selectedUnits[1].id);
      formData.append('methods', JSON.stringify(['crossfade_25ms', 'pitch_matched', 'overlap_add']));

      const response = await axios.post('/api/compare-transitions', formData, {
        headers: { 'Content-Type': 'multipart/form-data' }
      });
      
      setComparisonResults(response.data.results || response.data);
      
      // Auto-play first result for quick preview
      const firstMethod = Object.keys(response.data.results || response.data)[0];
      if (firstMethod) {
        const result = (response.data.results || response.data)[firstMethod];
        if (result?.audio_hex) {
          const blob = hexToBlob(result.audio_hex);
          if (blob) playAudioBlob(blob);
        }
      }
      
    } catch (err) {
      console.error("Comparison failed:", err);
      setErrorMsg(err.response?.data?.detail || 'Comparison failed');
    } finally {
      setIsGenerating(false);
    }
  };

  return (
    <div className="we-shell" style={{ padding: '16px', gap: '16px' }}>
      {/* Unit Selector */}
      <div>
        <h3 className="we-segments__header" style={{ padding: '0 0 8px', borderBottom: '1px solid #1e2535' }}>
          <span>Select Units to Concatenate</span>
          <span className="we-segments__count">{selectedUnits.length}</span>
        </h3>
        <div style={{ 
          display: 'flex', flexWrap: 'wrap', gap: '6px', 
          maxHeight: '120px', overflowY: 'auto', 
          padding: '8px', background: '#0f1421', borderRadius: '6px'
        }}>
          {libraryUnits?.slice(0, 50).map(unit => {
            const isSelected = selectedUnits.some(u => 
              (u.id && u.id === unit.id) || (u.filename && u.filename === unit.filename)
            );
            return (
              <button
                key={unit.id || unit.filename}
                onClick={() => handleAddUnit(unit)}
                disabled={isSelected}
                style={{
                  padding: '4px 10px',
                  borderRadius: '4px',
                  fontSize: '0.75rem',
                  border: '1px solid ' + (isSelected ? '#22c55e' : '#2d3553'),
                  background: isSelected ? '#14532d' : '#1a2035',
                  color: isSelected ? '#86efac' : '#e2e8f0',
                  cursor: isSelected ? 'default' : 'pointer',
                  opacity: isSelected ? 0.7 : 1
                }}
                title={`${unit.transcript} (${unit.morph_type})`}
              >
                {unit.transcript || 'Untitled'}
                {unit.tone_pattern && <span style={{ opacity: 0.7, marginLeft: '4px' }}>[{unit.tone_pattern}]</span>}
              </button>
            );
          })}
        </div>
      </div>
      
      {/* Selected Sequence */}
      {selectedUnits.length > 0 && (
        <div style={{ 
          padding: '12px', background: '#0f1421', borderRadius: '6px',
          border: '1px solid #1e2535'
        }}>
          <h4 style={{ margin: '0 0 8px', fontSize: '0.8rem', color: '#94a3b8' }}>
            Sequence ({selectedUnits.length} units)
          </h4>
          <div style={{ display: 'flex', flexWrap: 'wrap', alignItems: 'center', gap: '8px' }}>
            {selectedUnits.map((unit, idx) => (
              <div key={unit.id || unit.filename} style={{ display: 'flex', alignItems: 'center', gap: '6px' }}>
                <span style={{ 
                  padding: '4px 8px', background: '#1e3a5f', 
                  borderRadius: '4px', fontSize: '0.75rem', color: '#93c5fd'
                }}>
                  {unit.transcript || 'unit'}
                </span>
                <button 
                  onClick={() => handleRemoveUnit(unit.id, unit.filename)}
                  style={{ 
                    background: 'none', border: 'none', color: '#ef4444', 
                    cursor: 'pointer', fontSize: '0.75rem', padding: '2px' 
                  }}
                  title="Remove from sequence"
                >✕</button>
                {idx < selectedUnits.length - 1 && (
                  <span style={{ color: '#334155' }}>→</span>
                )}
              </div>
            ))}
            <button 
              onClick={() => setSelectedUnits([])}
              style={{ 
                marginLeft: 'auto', fontSize: '0.7rem', color: '#64748b', 
                background: 'none', border: 'none', cursor: 'pointer' 
              }}
            >
              Clear all
            </button>
          </div>
        </div>
      )}
      
      {/* Transition Method Selector */}
      <div>
        <label style={{ display: 'block', fontSize: '0.8rem', color: '#94a3b8', marginBottom: '6px' }}>
          Transition Method
        </label>
        <select 
          value={transitionMethod}
          onChange={e => setTransitionMethod(e.target.value)}
          style={{
            width: '100%', padding: '8px 12px', background: '#1a2035',
            border: '1px solid #2d3553', borderRadius: '6px', color: '#e2e8f0',
            fontSize: '0.8rem', outline: 'none'
          }}
        >
          {TRANSITION_OPTIONS.map(opt => (
            <option key={opt.value} value={opt.value} style={{ background: '#0f1421' }}>
              {opt.label}
            </option>
          ))}
        </select>
      </div>
      
      {/* Error Message */}
      {errorMsg && (
        <div style={{ 
          padding: '8px 12px', background: '#4c0519', 
          borderRadius: '6px', color: '#fca5a5', fontSize: '0.8rem' 
        }}>
          ⚠️ {errorMsg}
        </div>
      )}
      
      {/* Action Buttons */}
      <div style={{ display: 'flex', gap: '8px' }}>
        <button
          onClick={handleGenerate}
          disabled={selectedUnits.length < 2 || isGenerating}
          style={{
            flex: 1, padding: '10px', background: selectedUnits.length >= 2 && !isGenerating ? '#22c55e' : '#334155',
            color: '#fff', border: 'none', borderRadius: '6px', cursor: selectedUnits.length >= 2 && !isGenerating ? 'pointer' : 'not-allowed',
            fontWeight: 600, fontSize: '0.85rem', transition: 'background 0.15s'
          }}
        >
          {isGenerating ? '⏳ Generating...' : '▶ Generate & Play'}
        </button>
        
        {selectedUnits.length >= 2 && (
          <button
            onClick={handleCompare}
            disabled={isGenerating}
            style={{
              flex: 1, padding: '10px', background: !isGenerating ? '#7B6B8E' : '#334155',
              color: '#fff', border: 'none', borderRadius: '6px', cursor: !isGenerating ? 'pointer' : 'not-allowed',
              fontWeight: 600, fontSize: '0.85rem', transition: 'background 0.15s'
            }}
          >
            🔍 Compare
          </button>
        )}
      </div>
      
      {/* Comparison Results */}
      {Object.keys(comparisonResults).length > 0 && (
        <div style={{ marginTop: '16px' }}>
          <h4 style={{ margin: '0 0 8px', fontSize: '0.8rem', color: '#94a3b8' }}>Transition Comparison</h4>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
            {Object.entries(comparisonResults).map(([method, result]) => (
              <div key={method} style={{ 
                padding: '10px', border: '1px solid #1e2535', 
                borderRadius: '6px', background: '#0f1421' 
              }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '8px' }}>
                  <span style={{ fontWeight: 500, fontSize: '0.8rem' }}>{method}</span>
                  <span style={{ fontSize: '0.7rem', color: '#64748b' }}>{result.duration_ms}ms</span>
                </div>
                <button
                  onClick={() => {
                    if (result?.audio_hex) {
                      const blob = hexToBlob(result.audio_hex);
                      if (blob) playAudioBlob(blob);
                    }
                  }}
                  style={{
                    width: '100%', padding: '6px', background: '#1a2035',
                    border: '1px solid #2d3553', borderRadius: '4px', color: '#93c5fd',
                    cursor: 'pointer', fontSize: '0.75rem', transition: 'background 0.12s'
                  }}
                  onMouseOver={e => e.currentTarget.style.background = '#243050'}
                  onMouseOut={e => e.currentTarget.style.background = '#1a2035'}
                >
                  ▶ Play Preview
                </button>
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}