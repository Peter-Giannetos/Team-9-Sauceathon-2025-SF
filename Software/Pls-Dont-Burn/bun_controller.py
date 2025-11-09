"""
AI-Powered Burger Bun Toast Controller
Simplified MVP using Gemini Live API
"""

import asyncio
import sounddevice as sd
import numpy as np
import os
import io
import wave
import time
import logging
import textwrap
from dataclasses import dataclass, field
from typing import Any, Callable, Optional, Sequence, Tuple
from dotenv import load_dotenv
from google import genai
from google.genai import types
import soundfile as sf
import re

# Load environment variables
load_dotenv()

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)

# Personality prompt for the AI voice
PERSONALITY_PROMPT = """You are a chill Gen Z AI from the Toronto GTA with a light roadman vibe. Respond casually and helpfully, mixing in Gen Z slang like "bet," "lit," "sus," and Toronto terms like "wagwan," "fam," "ahlie" naturally. Keep answers short, no cap, and always vibe check the user's needs—be direct but fun, styll."""

DEFAULT_VOICE_NAME = "Sadaltager"


def describe_volume(volume_db: float) -> str:
    """Return a short descriptor for a given volume level."""
    if volume_db < -30:
        return "(they whispered - weak!)"
    if volume_db > -10:
        return "(they yelled - desperate!)"
    return "(normal volume)"


def iter_model_parts(message):
    """Yield parts from a LiveServerMessage if present."""
    server_content = getattr(message, "server_content", None)
    if (
        not server_content
        or not getattr(server_content, "model_turn", None)
        or not getattr(server_content.model_turn, "parts", None)
    ):
        return []
    return server_content.model_turn.parts


@dataclass
class InteractionConfig:
    """Configuration describing the interaction theme and scoring."""

    taunt: str
    persona: str
    scenario: str
    instructions: str
    outcome_label: str
    outcome_range: Optional[Tuple[int, int]]
    outcome_scale_description: str
    default_value: Any = 5
    extraction_patterns: Sequence[str] = field(default_factory=list)
    value_parser: Optional[Callable[[str], Any]] = None

    def __post_init__(self):
        if not self.extraction_patterns:
            label_pattern = re.escape(self.outcome_label)
            self.extraction_patterns = [
                rf'{label_pattern}\s*:?\s*(-?\d+)',
                r'[Ll]evel\s*:?\s*(-?\d+)',
                r'[Ss]core\s*:?\s*(-?\d+)',
            ]

    def build_prompt(self, volume_db: float) -> str:
        """Return the full prompt given the current loudness measurement."""
        if self.outcome_range:
            min_value, max_value = self.outcome_range
            range_clause = (
                f'End with EXACTLY: "{self.outcome_label} X" where X is between {min_value} and {max_value}.\n'
                f"{self.outcome_scale_description}"
            )
        else:
            range_clause = (
                f'End with EXACTLY: "{self.outcome_label} X".\n'
                f"{self.outcome_scale_description}"
            )

        prompt = f"""
        {PERSONALITY_PROMPT}

        You are {self.persona}.
        Scenario: {self.scenario}

        The human's volume level was {volume_db:.1f} dB {describe_volume(volume_db)}

        {self.instructions}

        {range_clause}
        """
        return textwrap.dedent(prompt).strip()


DEFAULT_CONFIG = InteractionConfig(
    taunt="Convince me not to burn your burger bun. You have 5 seconds. Go!",
    persona="a sassy, judgmental AI controlling a burger bun toaster",
    scenario="The human is begging you not to burn their burger bun while you decide how far to crank the toaster.",
    instructions=(
        "Listen carefully and judge their plea in one or two short, snarky sentences (under 5 seconds of audio).\n"
        "Factor in both the quality of their argument and how passionately they spoke."
    ),
    outcome_label="Burn level",
    outcome_range=(0, 9),
    outcome_scale_description=(
        "0 = spared and untoasted, 5 = medium toast, 9 = completely charred. "
        "Make the score match how convincing (or pathetic) they sounded."
    ),
    default_value=5,
)


class AudioRecorder:
    """Handles microphone input and volume detection"""

    def __init__(self, sample_rate=16000, channels=1):
        self.sample_rate = sample_rate
        self.channels = channels

    def calculate_db(self, audio_data):
        """Calculate decibel level from audio data"""
        rms = np.sqrt(np.mean(audio_data**2))
        if rms > 0:
            db = 20 * np.log10(rms)
        else:
            db = -np.inf
        logger.debug(f"Audio RMS: {rms:.6f}, dB: {db:.1f}")
        return db

    def record_audio(self, duration=5):
        """
        Record audio for specified duration
        Returns: tuple (audio_data, peak_volume_db)
        """
        logger.info(f"Recording for {duration} seconds...")

        recording = sd.rec(
            int(duration * self.sample_rate),
            samplerate=self.sample_rate,
            channels=self.channels,
            dtype='float32'
        )
        sd.wait()

        # Calculate peak volume
        peak_db = self.calculate_db(recording[:, 0])
        logger.info(f"Peak volume: {peak_db:.1f} dB")

        return recording[:, 0], peak_db  # Return as 1D array

    def audio_to_wav(self, audio_data):
        """Convert audio data to WAV container (16-bit PCM)"""
        buffer = io.BytesIO()
        sf.write(buffer, audio_data, self.sample_rate, format='WAV', subtype='PCM_16')
        buffer.seek(0)
        wav_data = buffer.read()
        logger.debug(f"WAV data size: {len(wav_data)} bytes")
        return wav_data


class AudioStreamPlayer:
    """Streams Gemini audio chunks directly to the speaker."""

    def __init__(self, sample_rate=24000):
        self.sample_rate = sample_rate
        self.stream: Optional[sd.OutputStream] = None
        self.frames_written = 0

    def start(self):
        if self.stream is None:
            self.stream = sd.OutputStream(
                samplerate=self.sample_rate,
                channels=1,
                dtype='float32',
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


class BunController:
    """Main controller using Gemini Live API"""

    def __init__(
        self,
        config: Optional[InteractionConfig] = None,
        stream_audio: bool = True,
        voice_name: str = DEFAULT_VOICE_NAME,
    ):
        self.recorder = AudioRecorder()
        self.config = config or DEFAULT_CONFIG
        self.stream_audio = stream_audio
        self.voice_name = voice_name
        api_key = os.getenv('GOOGLE_API_KEY')

        if not api_key:
            raise ValueError("GOOGLE_API_KEY not found in environment")

        self.client = genai.Client(
            api_key=api_key,
            http_options={"api_version": "v1alpha"},
        )
        self.model = "gemini-2.5-flash-native-audio-preview-09-2025"
        logger.info(
            "BunController configured with voice '%s' using Gemini Live API (v1alpha, affective dialog enabled)",
            self.voice_name,
        )

    def create_context_message(self, volume_db):
        """Create context message to send before audio input"""
        return self.config.build_prompt(volume_db)

    async def process_interaction(self, audio_data, volume_db, audio_wav=None):
        """
        Process user audio with Gemini Live API
        Returns: (audio_response_bytes, text_response, burn_level, streamed_live)
        """
        logger.info("AI is judging your plea...")

        config = types.LiveConnectConfig(
            response_modalities=["AUDIO"],
            enable_affective_dialog=True,
            speech_config=types.SpeechConfig(
                voice_config=types.VoiceConfig(
                    prebuilt_voice_config=types.PrebuiltVoiceConfig(
                        voice_name=self.voice_name
                    )
                )
            ),
        )
        logger.debug(
            "Live session config: audio response with voice '%s' and affective dialog enabled",
            self.voice_name,
        )

        audio_wav = audio_wav or self.recorder.audio_to_wav(audio_data)
        inline_audio = types.Part(
            inline_data=types.Blob(
                data=audio_wav,
                mime_type="audio/wav",
            )
        )

        audio_chunks = []
        text_response = ""
        outcome_value = None
        streamed_audio_seconds = 0.0
        min_stream_seconds_after_outcome = 1.5
        player = AudioStreamPlayer() if self.stream_audio else None

        try:
            async with self.client.aio.live.connect(model=self.model, config=config) as session:
                logger.debug("Connected to Gemini Live API")

                context_msg = self.create_context_message(volume_db)
                logger.debug(
                    "Sending combined prompt/audio (length %d): %s",
                    len(context_msg),
                    context_msg,
                )
                await session.send_client_content(
                    turns=types.Content(
                        role="user",
                        parts=[
                            types.Part(text=context_msg),
                            inline_audio,
                        ],
                    )
                )

                logger.debug("Awaiting Gemini response stream")

                response_count = 0
                async for response in session.receive():
                    response_count += 1
                    logger.debug(f"Response #{response_count}: {type(response)}")

                    for part in iter_model_parts(response):
                        inline_data = getattr(part, "inline_data", None)
                        if inline_data and inline_data.data:
                            chunk = inline_data.data
                            audio_chunks.append(chunk)
                            streamed_audio_seconds += len(chunk) / (2 * 24000)
                            logger.debug(
                                "Received audio chunk: %d bytes (%.2f total seconds)",
                                len(chunk),
                                streamed_audio_seconds,
                            )
                            if player:
                                player.write_chunk(chunk)

                        part_text = getattr(part, "text", None)
                        is_thought = bool(getattr(part, "thought", False))
                        if part_text and not is_thought:
                            text_response += part_text
                            logger.debug(f"Received text: {part_text}")
                            if outcome_value is None:
                                outcome_value = self.try_extract_outcome(text_response)
                                if outcome_value is not None:
                                    logger.debug(
                                        "Outcome detected early: %s %s",
                                        self.config.outcome_label,
                                        outcome_value,
                                    )

                    if (
                        outcome_value is not None
                        and streamed_audio_seconds >= min_stream_seconds_after_outcome
                    ):
                        logger.debug(
                            "Stopping receive loop after outcome detected and %.2f seconds streamed",
                            streamed_audio_seconds,
                        )
                        break

                logger.info(
                    "Received %d audio chunks and %d characters of text",
                    len(audio_chunks),
                    len(text_response),
                )

        except Exception as e:
            logger.error(f"Error during Gemini interaction: {e}", exc_info=True)
            raise
        finally:
            if player:
                player.close()

        # Combine audio chunks
        audio_bytes = b''.join(audio_chunks) if audio_chunks else b''

        # If no text response, use a default message
        if not text_response:
            logger.warning("No text response received, using default")
            text_response = f"I'm speechless. {self.config.outcome_label} {self.config.default_value}"

        if outcome_value is None:
            outcome_value = self.parse_model_output(text_response)
        else:
            if isinstance(outcome_value, (int, float)) and self.config.outcome_range:
                min_value, max_value = self.config.outcome_range
                outcome_value = min(max(int(outcome_value), min_value), max_value)

        streamed_live = bool(player and player.frames_written > 0)

        return (
            audio_bytes,
            text_response,
            outcome_value,
            streamed_live,
        )

    def try_extract_outcome(self, text):
        """Attempt to extract configured outcome from response text without defaulting."""
        parse_value = self.config.value_parser or (lambda raw: int(raw))
        for pattern in self.config.extraction_patterns:
            match = re.search(pattern, text)
            if match:
                try:
                    raw_value = match.group(1)
                    value = parse_value(raw_value)
                except Exception:
                    continue
                if value is None:
                    continue

                return value
        return None

    def parse_model_output(self, text):
        """Extract configured outcome from response text."""
        value = self.try_extract_outcome(text)
        if value is not None:
            if isinstance(value, (int, float)) and self.config.outcome_range:
                min_value, max_value = self.config.outcome_range
                clamped = min(max(int(value), min_value), max_value)
                logger.info(f"Extracted {self.config.outcome_label.lower()}: {clamped}")
                return clamped

            logger.info(f"Extracted {self.config.outcome_label.lower()}: {value}")
            return value

        logger.warning(
            "No %s found in text '%s', defaulting to %s",
            self.config.outcome_label.lower(),
            text,
            self.config.default_value,
        )
        return self.config.default_value

    def save_audio(self, audio_bytes, filename="response.wav"):
        """Save audio response to WAV file"""
        if not audio_bytes:
            logger.warning("No audio data to save")
            return None

        # Gemini outputs 24kHz audio
        with wave.open(filename, "wb") as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)  # 16-bit
            wf.setframerate(24000)
            wf.writeframes(audio_bytes)

        logger.debug(f"Saved audio to {filename} ({len(audio_bytes)} bytes)")
        return filename

    def play_audio(self, audio_bytes):
        """Play audio response"""
        if not audio_bytes:
            logger.warning("No audio to play")
            return

        # Convert bytes to numpy array for playback
        audio_array = np.frombuffer(audio_bytes, dtype=np.int16)

        logger.debug(f"Playing audio: {len(audio_array)} samples at 24kHz")
        # Play at 24kHz (Gemini's output rate)
        sd.play(audio_array, 24000)
        sd.wait()

    def initial_taunt(self):
        """Display initial taunt"""
        print("\nBURGER BUN TOASTER AI")
        print("=" * 50)

        taunt_text = self.config.taunt
        print(f"AI: {taunt_text}")
        print("\n[Get ready to speak!]")

    async def run_cycle_async(self):
        """Execute one complete cycle (async version)"""
        start_time = time.time()

        # Step 1: Taunt user
        self.initial_taunt()

        # Step 2: Record user response (5 seconds)
        audio_data, volume_db = self.recorder.record_audio(duration=5)
        audio_wav = self.recorder.audio_to_wav(audio_data)

        # Step 3: Process with Gemini Live API
        (
            audio_response,
            text_response,
            outcome_value,
            streamed_live,
        ) = await self.process_interaction(
            audio_data, volume_db, audio_wav=audio_wav
        )

        # Step 4: Display results
        logger.info(f"AI Response: {text_response}")
        if self.config.outcome_range:
            logger.info(
                "%s: %s (range %s-%s)",
                self.config.outcome_label,
                outcome_value,
                self.config.outcome_range[0],
                self.config.outcome_range[1],
            )
        else:
            logger.info("%s: %s", self.config.outcome_label, outcome_value)

        # Step 5: Play audio response (only if we did not already stream it live)
        if not streamed_live:
            logger.info("Playing AI response...")
            self.play_audio(audio_response)
        else:
            logger.info("Audio was streamed live during model response")

        # Save audio for debugging
        self.save_audio(audio_response, "ai_response.wav")

        # Calculate total time
        elapsed = time.time() - start_time
        logger.info(f"Total cycle time: {elapsed:.2f} seconds")

        if elapsed > 10:
            logger.warning(f"Cycle exceeded 10 second requirement! ({elapsed:.2f}s)")

        print("=" * 50)

        return outcome_value

    def run_cycle(self):
        """Execute one complete cycle (sync wrapper)"""
        return asyncio.run(self.run_cycle_async())


def main():
    """Main entry point"""
    import sys

    # Enable debug logging if DEBUG env var is set
    if os.getenv('DEBUG', '').lower() in ('1', 'true', 'yes'):
        logger.setLevel(logging.DEBUG)
        logger.info("Debug logging enabled")

    # Or enable via command line arg
    if '--debug' in sys.argv:
        logger.setLevel(logging.DEBUG)
        logger.info("Debug logging enabled via --debug flag")

    print("Welcome to the AI Burger Bun Toast Controller!")
    print("   Powered by Gemini Live API")
    print("\nMake sure you have set up your .env file with:")
    print("  - GOOGLE_API_KEY")
    print()

    # Verify API key
    if not os.getenv('GOOGLE_API_KEY'):
        print("Error: GOOGLE_API_KEY not found in .env file")
        print("   Get your API key at: https://ai.google.dev/")
        return

    try:
        controller = BunController()
        outcome_value = controller.run_cycle()
        print(f"\nFinal {controller.config.outcome_label.lower()} to send to hardware: {outcome_value}")

    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
    except Exception as e:
        logger.error(f"Error: {e}", exc_info=True)
        print(f"\nError: {e}")
        print("   Run with --debug flag for more details")


if __name__ == "__main__":
    main()
