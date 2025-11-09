"""Audio recording, conversion, and playback helpers."""

from __future__ import annotations

import io
import logging
import wave
import os
from typing import Optional, Tuple

import numpy as np
import sounddevice as sd
import soundfile as sf

logger = logging.getLogger(__name__)


class AudioRecorder:
    """Handles microphone input and volume detection."""

    def __init__(self, sample_rate: int = 16000, channels: int = 1):
        self.sample_rate = sample_rate
        self.channels = channels

    def calculate_db(self, audio_data: np.ndarray) -> float:
        """Calculate decibel level from audio data."""
        rms = np.sqrt(np.mean(audio_data**2))
        if rms > 0:
            db = 20 * np.log10(rms)
        else:
            db = -np.inf
        logger.debug("Audio RMS: %.6f, dB: %.1f", rms, db)
        return db

    def record_audio(self, duration: int = 5) -> Tuple[np.ndarray, float]:
        """Record audio for the specified duration."""
        logger.info("Recording for %s seconds...", duration)
        recording = sd.rec(
            int(duration * self.sample_rate),
            samplerate=self.sample_rate,
            channels=self.channels,
            dtype="float32",
        )
        sd.wait()

        peak_db = self.calculate_db(recording[:, 0])
        logger.info("Peak volume: %.1f dB", peak_db)
        return recording[:, 0], peak_db

    def audio_to_wav(self, audio_data: np.ndarray) -> bytes:
        """Convert audio data to WAV container (16-bit PCM)."""
        buffer = io.BytesIO()
        sf.write(buffer, audio_data, self.sample_rate, format="WAV", subtype="PCM_16")
        buffer.seek(0)
        wav_data = buffer.read()
        logger.debug("WAV data size: %d bytes", len(wav_data))
        return wav_data


class AudioStreamPlayer:
    """Streams Gemini audio chunks directly to the speaker."""

    def __init__(self, sample_rate: int = 24000):
        self.sample_rate = sample_rate
        self.stream: Optional[sd.OutputStream] = None
        self.frames_written = 0

    def start(self):
        if self.stream is None:
            self.stream = sd.OutputStream(
                samplerate=self.sample_rate,
                channels=1,
                dtype="float32",
                blocksize=0,
            )
            self.stream.start()

    def write_chunk(self, audio_bytes: bytes):
        if not audio_bytes:
            return
        self.start()
        audio_array = np.frombuffer(audio_bytes, dtype=np.int16).astype(np.float32) / 32768.0
        audio_array = audio_array.reshape(-1, 1)
        self.stream.write(audio_array)
        self.frames_written += audio_array.shape[0]

    def close(self):
        if self.stream is not None:
            self.stream.stop()
            self.stream.close()
            self.stream = None


def play_audio(audio_bytes: bytes, sample_rate: int = 24000):
    """Play audio response."""
    if not audio_bytes:
        logger.warning("No audio to play")
        return

    audio_array = np.frombuffer(audio_bytes, dtype=np.int16)
    logger.debug("Playing audio: %d samples at %dkHz", len(audio_array), sample_rate // 1000)
    sd.play(audio_array, sample_rate)
    sd.wait()


def save_audio(audio_bytes: bytes, filename: str = "response.wav", sample_rate: int = 24000):
    """Persist audio bytes to a WAV file for debugging."""
    if not audio_bytes:
        logger.warning("No audio data to save")
        return None

    with wave.open(filename, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.writeframes(audio_bytes)

    logger.debug("Saved audio to %s (%d bytes)", filename, len(audio_bytes))
    return filename


def load_audio(filename: str) -> Optional[bytes]:
    """Load raw PCM data from a WAV file if it exists."""
    if not os.path.exists(filename):
        return None

    try:
        with wave.open(filename, "rb") as wf:
            frames = wf.readframes(wf.getnframes())
    except Exception as exc:
        logger.error("Failed to load audio from %s: %s", filename, exc)
        return None

    return frames


__all__ = [
    "AudioRecorder",
    "AudioStreamPlayer",
    "load_audio",
    "play_audio",
    "save_audio",
]
