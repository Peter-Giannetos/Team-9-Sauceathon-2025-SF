"""High-level orchestration for the bun controller application."""

from __future__ import annotations

import asyncio
import logging
import os
import time
from typing import Optional

from dotenv import load_dotenv

from audio_utils import AudioRecorder, load_audio, play_audio, save_audio
from config import DEFAULT_CONFIG, InteractionConfig
from gemini_client import GeminiJudge, GeminiTauntSpeaker, InteractionResult

load_dotenv()

logger = logging.getLogger(__name__)

DEFAULT_VOICE_NAME = "Sadaltager"
DEFAULT_TAUNT_AUDIO_PATH = "taunt_prompt.wav"


class BunController:
    """Main controller responsible for the voice interaction workflow."""

    def __init__(
        self,
        *,
        config: Optional[InteractionConfig] = None,
        stream_audio: bool = True,
        voice_name: str = DEFAULT_VOICE_NAME,
        recorder: Optional[AudioRecorder] = None,
        judge: Optional[GeminiJudge] = None,
        taunt_speaker: Optional[GeminiTauntSpeaker] = None,
        taunt_audio_path: str = DEFAULT_TAUNT_AUDIO_PATH,
    ):
        self.config = config or DEFAULT_CONFIG
        self.stream_audio = stream_audio
        self.voice_name = voice_name
        self.taunt_audio_path = taunt_audio_path
        self.recorder = recorder or AudioRecorder()
        self.judge = judge or GeminiJudge(
            self.config,
            stream_audio=stream_audio,
            voice_name=voice_name,
        )
        self.taunt_speaker = taunt_speaker or GeminiTauntSpeaker(voice_name=voice_name)

    def create_context_message(self, volume_db: float) -> str:
        """Create the context message sent alongside user audio."""
        return self.config.build_prompt(volume_db)

    async def initial_taunt(self):
        """Display (and now speak) the taunt before recording."""
        print("\nBURGER BUN TOASTER AI")
        print("=" * 50)
        print(f"AI: {self.config.taunt}")
        print("\n[Get ready to speak!]")
        await self._speak_taunt_audio()

    async def _speak_taunt_audio(self):
        """Invoke the audio API so the taunt is spoken aloud."""
        if not self.taunt_speaker:
            return

        cached_audio = self._load_cached_taunt()
        if cached_audio:
            logger.info("Playing cached taunt audio from %s", self.taunt_audio_path)
            play_audio(cached_audio)
            return

        try:
            audio_bytes = await self.taunt_speaker.speak_taunt(
                self.config.taunt,
                persona=str(self.config.persona),
            )
        except Exception as exc:  # pragma: no cover - audio prompt best effort
            logger.error("Failed to speak taunt via Gemini: %s", exc, exc_info=True)
            return

        play_audio(audio_bytes)
        self._save_taunt_audio(audio_bytes)

    def _load_cached_taunt(self) -> Optional[bytes]:
        """Return cached taunt audio if the file exists."""
        if not self.taunt_audio_path:
            return None

        if not os.path.exists(self.taunt_audio_path):
            return None

        return load_audio(self.taunt_audio_path)

    def _save_taunt_audio(self, audio_bytes: bytes):
        """Persist taunt audio for future runs."""
        if not self.taunt_audio_path:
            return

        try:
            save_audio(audio_bytes, filename=self.taunt_audio_path)
        except Exception as exc:  # pragma: no cover - best-effort cache
            logger.error("Failed to save taunt audio: %s", exc, exc_info=True)

    async def run_cycle_async(self) -> int:
        """Execute one complete interaction cycle."""
        start_time = time.time()
        timings = {}

        t0 = time.time()
        await self.initial_taunt()
        timings["taunt"] = time.time() - t0

        t0 = time.time()
        audio_data, volume_db = self.recorder.record_audio(duration=5)
        timings["recording"] = time.time() - t0

        t0 = time.time()
        audio_wav = self.recorder.audio_to_wav(audio_data)
        timings["wav_conversion"] = time.time() - t0

        context_message = self.create_context_message(volume_db)
        t0 = time.time()
        interaction: InteractionResult = await self.judge.evaluate(
            audio_wav=audio_wav,
            context_message=context_message,
        )
        timings["gemini_processing"] = time.time() - t0
        timings.update({f"gemini_{k}": v for k, v in interaction.timings.items()})

        logger.info("AI Response: %s", interaction.text_response)
        if self.config.outcome_range:
            logger.info(
                "%s: %s (range %s-%s)",
                self.config.outcome_label,
                interaction.outcome_value,
                self.config.outcome_range[0],
                self.config.outcome_range[1],
            )
        else:
            logger.info("%s: %s", self.config.outcome_label, interaction.outcome_value)

        t0 = time.time()
        if not interaction.streamed_live:
            logger.info("Playing AI response...")
            play_audio(interaction.audio_bytes)
        else:
            logger.info("Audio was streamed live during model response")
        timings["audio_playback"] = time.time() - t0

        t0 = time.time()
        save_audio(interaction.audio_bytes, "ai_response.wav")
        timings["save_audio"] = time.time() - t0

        elapsed = time.time() - start_time
        logger.info("Total cycle time: %.2f seconds", elapsed)
        logger.info("=== TIMING BREAKDOWN ===")
        for stage, duration in timings.items():
            logger.info("  %s: %.3fs (%.1f%%)", stage, duration, duration / elapsed * 100)
        logger.info("========================")

        if elapsed > 10:
            logger.warning("Cycle exceeded 10 second requirement! (%.2fs)", elapsed)

        print("=" * 50)
        return interaction.outcome_value

    def run_cycle(self) -> int:
        """Execute one complete cycle (sync wrapper)."""
        return asyncio.run(self.run_cycle_async())


__all__ = ["BunController", "DEFAULT_VOICE_NAME"]
