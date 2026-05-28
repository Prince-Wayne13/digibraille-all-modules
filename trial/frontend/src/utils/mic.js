// Request mic on user gesture, not on load
export async function startRecording() {
  try {
    // Must be called from button click handler
    const stream = await navigator.mediaDevices.getUserMedia({
      audio: {
        echoCancellation: true,
        noiseSuppression: false,  // disable to preserve tonal quality
        autoGainControl: false    // we handle normalization server-side
      }
    });
    
    const mediaRecorder = new MediaRecorder(stream, {
      mimeType: 'audio/webm;codecs=opus'
    });
    
    const chunks = [];
    mediaRecorder.ondataavailable = e => chunks.push(e.data);
    
    return {
      mediaRecorder,
      start: () => {
        chunks.length = 0;
        mediaRecorder.start();
      },
      stop: () => {
        return new Promise(resolve => {
          mediaRecorder.onstop = () => {
            const blob = new Blob(chunks, { type: 'audio/webm' });
            // Convert to WAV for backend (simplified: send webm, backend converts)
            resolve(blob);
            stream.getTracks().forEach(track => track.stop());
          };
          mediaRecorder.stop();
        });
      },
      error: null
    };
  } catch (err) {
    console.error("Mic error:", err);
    return {
      error: err.name === 'NotAllowedError' 
        ? "Microphone access denied. Please allow permissions and try again."
        : "Could not access microphone: " + err.message
    };
  }
}

// Fallback: file upload handler
export function handleFileUpload(file) {
  return new Promise((resolve, reject) => {
    if (!file.type.startsWith('audio/')) {
      reject(new Error("Please select an audio file"));
      return;
    }
    resolve(file);
  });
}