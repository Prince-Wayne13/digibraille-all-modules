#!/usr/bin/env python3
"""
Digibraille v2 - Audio File Uploader for ESP32 SPI Flash
Uploads .wav/.raw/.mp3 files to external flash via serial protocol.

Usage:
    python upload_audio.py
"""

import serial
import os
import sys
import time
from pathlib import Path

# ─── CONFIGURATION ─────────────────────────────────────────
# ─── CONFIGURATION ─────────────────────────────────────────
PORT = "COM3"          # ← Verify this matches your ESP32 port
BAUD = 115200

# ✅ UPDATE THIS PATH TO YOUR AUDIO FOLDER:
AUDIO_FOLDER = r"C:\Users\PRINCE\Documents\PlatformIO\Projects\flashmemory\data\sfx\ch"

TIMEOUT = 30
ERASE_TIMEOUT = 60
# ──────────────────────────────────────────────────────────    # chip erase can take longer
# ──────────────────────────────────────────────────────────

def wait_for(ser, expected: str, timeout: float = TIMEOUT) -> str:
    """Wait for a line containing expected string. Returns the line."""
    start = time.time()
    while time.time() - start < timeout:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"  ← {line}")
            if expected in line:
                return line
        time.sleep(0.01)  # Prevent CPU spin
    raise TimeoutError(f"Timeout waiting for: '{expected}'")

def connect_esp32(port: str, baud: int) -> serial.Serial:
    """Establish serial connection, force ESP32 reboot, and wait for READY."""
    print(f"Connecting to {port} @ {baud}...")
    
    ser = serial.Serial(port, baud, timeout=1)
    
    # Clear any leftover data
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    # Hardware reset via DTR/RTS (standard for ESP32)
    print("  Triggering ESP32 reset...")
    ser.setDTR(False)
    ser.setRTS(True)
    time.sleep(0.2)
    ser.setDTR(True)
    ser.setRTS(False)
    time.sleep(0.2)
    ser.setDTR(False)
    
    time.sleep(3)  # Give ESP32 time to boot & initialize SPI
    
    # Clear boot logs from buffer
    ser.reset_input_buffer()
    
    print("  Waiting for READY...")
    wait_for(ser, "READY", timeout=10)
    print("✓ ESP32 connected & synced\n")
    return ser

def erase_chip(ser: serial.Serial):
    """Send erase command and wait for completion."""
    print("[1/3] Erasing flash chip...")
    ser.write(b"ERASE\n")
    wait_for(ser, "OK:ERASED", timeout=ERASE_TIMEOUT)
    print("✓ Chip erased\n")

def get_audio_files(folder: str) -> list:
    """Get list of supported audio files."""
    extensions = ('.wav', '.raw', '.mp3', '.bin')
    files = []
    
    for f in os.listdir(folder):
        if f.lower().endswith(extensions):
            files.append(f)
    
    # Sort for consistent ordering
    files.sort()
    return files

def upload_file(ser: serial.Serial, filepath: str, filename: str) -> int:
    """Upload a single file. Returns the flash address where it was stored."""
    filesize = os.path.getsize(filepath)
    print(f"  Uploading: {filename} ({filesize:,} bytes)")
    
    # Send WRITE command
    cmd = f"WRITE:{filename}:{filesize}\n"
    ser.write(cmd.encode('ascii'))
    
    # Wait for READY with address
    response = wait_for(ser, "OK:READY:")
    try:
        address = int(response.split("OK:READY:")[1].strip())
    except (IndexError, ValueError) as e:
        raise RuntimeError(f"Failed to parse address from: {response}") from e
    
    print(f"  → Will write to address {address:,}")
    
    # Read and send file data in chunks
    with open(filepath, 'rb') as f:
        data = f.read()
    
    # Send in 256-byte chunks (matches flash page size)
    chunk_size = 256
    for offset in range(0, len(data), chunk_size):
        chunk = data[offset:offset + chunk_size]
        ser.write(chunk)
        # Small delay prevents buffer overflow on ESP32
        if offset % 1024 == 0:
            time.sleep(0.005)
    
    # Wait for completion
    wait_for(ser, "OK:DONE")
    print(f"  ✓ Complete → stored at {address:,}\n")
    
    return address

def save_file_map(file_map: dict, output_path: str = "flash_map.txt"):
    """Save filename→address mapping to file."""
    print(f"[3/3] Saving file map to {output_path}...")
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("# Digibraille Flash Map\n")
        f.write("# Format: filename:size:flash_address\n")
        f.write(f"# Generated: {time.strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        for name, (addr, size) in sorted(file_map.items()):
            f.write(f"{name}:{size}:{addr}\n")
            print(f"  {name:30} → addr:{addr:8}  size:{size:6}")
    
    print(f"✓ Saved {len(file_map)} entries\n")

def list_remote_files(ser: serial.Serial):
    """Query and display files stored on ESP32."""
    print("Querying ESP32 file list...")
    ser.write(b"LIST\n")
    
    try:
        wait_for(ser, "FILES:START")
        print("\n  Files on ESP32:")
        while True:
            line = wait_for(ser, "", timeout=5)
            if "FILES:END" in line:
                break
            if line.startswith("FILE:"):
                parts = line[5:].split(':')  # Remove "FILE:" prefix
                if len(parts) >= 3:
                    name, size, addr = parts[0], parts[1], parts[2]
                    print(f"    {name:25} {size:>8} bytes @ {addr}")
        wait_for(ser, "OK:LIST_DONE")
        print()
    except TimeoutError:
        print("  (timeout reading file list)\n")

def main():
    print("=== Digibraille Audio Uploader v2 ===\n")
    
    # Validate audio folder
    if not os.path.isdir(AUDIO_FOLDER):
        print(f"❌ Audio folder not found: {AUDIO_FOLDER}")
        sys.exit(1)
    
    # Get files to upload
    files = get_audio_files(AUDIO_FOLDER)
    if not files:
        print(f"❌ No audio files found in {AUDIO_FOLDER}")
        print("   Supported: .wav, .raw, .mp3, .bin")
        sys.exit(1)
    
    print(f"Found {len(files)} audio file(s)\n")
    
    try:
        # Connect to ESP32
        ser = connect_esp32(PORT, BAUD)
        
        # Erase chip
        erase_chip(ser)
        
        # Upload files
        file_map = {}  # {filename: (address, size)}
        print(f"[2/3] Uploading {len(files)} file(s)...\n")
        
        for filename in files:
            filepath = os.path.join(AUDIO_FOLDER, filename)
            address = upload_file(ser, filepath, filename)
            filesize = os.path.getsize(filepath)
            file_map[filename] = (address, filesize)
        
        ser.close()
        
        # Save mapping
        save_file_map(file_map)
        
        # Optional: verify by listing remote files
        # Reconnect briefly to query
        print("Verifying upload...")
        ser = connect_esp32(PORT, BAUD)
        list_remote_files(ser)
        ser.close()
        
        print("✅ All done! Files uploaded successfully.")
        print("   Use flash_map.txt for address references in your firmware.\n")
        
    except serial.SerialException as e:
        print(f"\n❌ Serial error: {e}")
        print("   Check: Is the ESP32 connected? Is the port correct?")
        sys.exit(1)
    except TimeoutError as e:
        print(f"\n❌ Timeout: {e}")
        print("   Check: Is the ESP32 running the correct firmware?")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()