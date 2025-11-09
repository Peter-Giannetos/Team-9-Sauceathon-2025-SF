"""Gemini Live API integration helpers."""

from __future__ import annotations

import asyncio
import contextlib
import logging
import os
import re
import time
import textwrap
from dataclasses import dataclass
from typing import Dict, Optional

from google import genai
from google.genai import types

from audio_utils import AudioStreamPlayer
from config import InteractionConfig

logger = logging.getLogger(__name__)

TRANSCRIPTION_TIMEOUT_SECONDS = 20


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
class InteractionResult:
    """Structured result returned by the Gemini judge."""

    audio_bytes: bytes
    text_response: str
    outcome_value: int
    streamed_live: bool
    timings: Dict[str, float]
    transcript: str = ""


class GeminiTauntSpeaker:
    """Lightweight helper that asks Gemini to speak the intro taunt."""

    def __init__(
        self,
        *,
        voice_name: str = "Sadaltager",
        model: str = "gemini-2.5-flash-native-audio-preview-09-2025",
    ):
        api_key = os.getenv("GOOGLE_API_KEY")
        if not api_key:
            raise ValueError("GOOGLE_API_KEY not found in environment")

        self.voice_name = voice_name
        self.model = model
        self.client = genai.Client(api_key=api_key, http_options={"api_version": "v1alpha"})

    async def speak_taunt(self, taunt_text: str, *, persona: Optional[str] = None) -> bytes:
        """Generate audio for the taunt using the Live Audio API."""
        if not taunt_text:
            raise ValueError("taunt_text must be a non-empty string")

        logger.info("Requesting spoken taunt from Gemini...")
        persona_block = persona.strip() if persona else (
            "You are the announcer for a burger bun defense challenge with a sharp, nasally tone."
        )
        prompt = textwrap.dedent(
            f"""
            {persona_block}

            Stay perfectly in-character while delivering the opening taunt.
            Say the following line verbatim with no additions, no lead-ins, and no trailing remarks:
            "{taunt_text.strip()}"
            """
        ).strip()

        config = types.LiveConnectConfig(
            response_modalities=["AUDIO"],
            enable_affective_dialog=True,
            speech_config=types.SpeechConfig(
                voice_config=types.VoiceConfig(
                    prebuilt_voice_config=types.PrebuiltVoiceConfig(voice_name=self.voice_name)
                )
            ),
        )

        audio_chunks = []
        async with self.client.aio.live.connect(model=self.model, config=config) as session:
            await session.send_client_content(
                turns=types.Content(role="user", parts=[types.Part(text=prompt)])
            )

            async for response in session.receive():
                for part in iter_model_parts(response):
                    inline_data = getattr(part, "inline_data", None)
                    if inline_data and inline_data.data:
                        audio_chunks.append(inline_data.data)

        if not audio_chunks:
            raise RuntimeError("Gemini returned no audio for the taunt prompt")

        taunt_audio = b"".join(audio_chunks)
        logger.debug("Received taunt audio (%d bytes)", len(taunt_audio))
        return taunt_audio


class GeminiJudge:
    """Wrapper around the Gemini Live API for evaluating pleas."""

    def __init__(
        self,
        config: InteractionConfig,
        *,
        stream_audio: bool = True,
        voice_name: str = "Sadaltager",
        model: str = "gemini-2.5-flash-native-audio-preview-09-2025",
        transcription_model: str = "gemini-2.0-flash-exp",
    ):
        self.config = config
        self.stream_audio = stream_audio
        self.voice_name = voice_name
        self.model = model
        self.transcription_model = transcription_model

        api_key = os.getenv("GOOGLE_API_KEY")
        if not api_key:
            raise ValueError("GOOGLE_API_KEY not found in environment")

        self.client = genai.Client(api_key=api_key, http_options={"api_version": "v1alpha"})
        self.standard_client = genai.Client(api_key=api_key)
        logger.info(
            "GeminiJudge configured with voice '%s' (model: %s)",
            self.voice_name,
            self.model,
        )

    async def transcribe_audio(self, audio_wav: bytes) -> str:
        """Transcribe audio using the standard Gemini API."""
        if not audio_wav:
            logger.warning("Transcription skipped: empty audio buffer")
            return ""

        try:
            response = await self.standard_client.aio.models.generate_content(
                model=self.transcription_model,
                contents=[
                    types.Content(
                        role="user",
                        parts=[
                            types.Part(text="Transcribe the following audio verbatim."),
                            types.Part(
                                inline_data=types.Blob(data=audio_wav, mime_type="audio/wav")
                            ),
                        ],
                    )
                ],
            )
        except Exception as exc:
            logger.error("Transcription request failed: %s", exc, exc_info=True)
            return ""

        transcript_parts = []
        for candidate in getattr(response, "candidates", []) or []:
            content = getattr(candidate, "content", None)
            if not content:
                continue
            for part in getattr(content, "parts", []) or []:
                text = getattr(part, "text", None)
                if text:
                    transcript_parts.append(text)

        transcript = " ".join(transcript_parts).strip()
        if not transcript:
            logger.warning("Transcription produced no text")
        return transcript

    async def evaluate(self, *, audio_wav: bytes, context_message: str) -> InteractionResult:
        """Process user audio with Gemini Live API."""
        if not audio_wav:
            raise ValueError("audio_wav is required to evaluate the plea")

        logger.info("AI is judging your plea...")
        timings: Dict[str, float] = {}

        config = types.LiveConnectConfig(
            response_modalities=["AUDIO"],
            enable_affective_dialog=True,
            speech_config=types.SpeechConfig(
                voice_config=types.VoiceConfig(
                    prebuilt_voice_config=types.PrebuiltVoiceConfig(voice_name=self.voice_name)
                )
            ),
        )

        transcription_task = None
        transcription_start = None
        if audio_wav and logger.isEnabledFor(logging.DEBUG):
            logger.debug("Starting parallel transcription task (%d bytes)", len(audio_wav))
            transcription_start = time.time()
            transcription_task = asyncio.create_task(self.transcribe_audio(audio_wav))

        inline_audio = types.Part(
            inline_data=types.Blob(
                data=audio_wav,
                mime_type="audio/wav",
            )
        )

        audio_chunks = []
        text_response = ""
        outcome_value: Optional[int] = None
        streamed_audio_seconds = 0.0
        min_stream_seconds_after_outcome = 0.5
        player = AudioStreamPlayer() if self.stream_audio else None

        first_chunk_time = None
        first_response_time = None

        try:
            t0 = time.time()
            async with self.client.aio.live.connect(model=self.model, config=config) as session:
                timings["api_connect"] = time.time() - t0
                logger.debug("Connected to Gemini Live API")

                t0 = time.time()
                await session.send_client_content(
                    turns=types.Content(
                        role="user",
                        parts=[
                            types.Part(text=context_message),
                            inline_audio,
                        ],
                    )
                )
                timings["send_content"] = time.time() - t0

                stream_start = time.time()
                response_count = 0

                async for response in session.receive():
                    response_count += 1
                    if first_response_time is None:
                        first_response_time = time.time()
                        timings["time_to_first_response"] = first_response_time - stream_start
                        logger.info(
                            "⏱️  Time to first response: %.3fs",
                            timings["time_to_first_response"],
                        )

                    for part in iter_model_parts(response):
                        inline_data = getattr(part, "inline_data", None)
                        if inline_data and inline_data.data:
                            chunk = inline_data.data
                            if first_chunk_time is None:
                                first_chunk_time = time.time()
                                timings["time_to_first_audio_chunk"] = first_chunk_time - stream_start
                                logger.info(
                                    "⏱️  Time to first audio chunk: %.3fs",
                                    timings["time_to_first_audio_chunk"],
                                )

                            audio_chunks.append(chunk)
                            streamed_audio_seconds += len(chunk) / (2 * 24000)
                            if player:
                                player.write_chunk(chunk)

                        part_text = getattr(part, "text", None)
                        is_thought = bool(getattr(part, "thought", False))
                        if part_text and not is_thought:
                            text_response += part_text
                            if outcome_value is None:
                                outcome_value = self.try_extract_outcome(text_response)
                                if outcome_value is not None:
                                    timings["time_to_outcome_detection"] = time.time() - stream_start

                    if (
                        outcome_value is not None
                        and streamed_audio_seconds >= min_stream_seconds_after_outcome
                    ):
                        break

                timings["streaming_duration"] = time.time() - stream_start
                logger.info(
                    "Received %d audio chunks and %d characters of text",
                    len(audio_chunks),
                    len(text_response),
                )

        except Exception as exc:
            logger.error("Error during Gemini interaction: %s", exc, exc_info=True)
            if transcription_task:
                transcription_task.cancel()
                with contextlib.suppress(Exception):
                    await transcription_task
            raise
        finally:
            if player:
                player.close()

        audio_bytes = b"".join(audio_chunks) if audio_chunks else b""
        if not text_response:
            logger.warning("No text response received, using default")
            text_response = f"I'm speechless. {self.config.outcome_label} {self.config.default_value}"

        if outcome_value is None:
            outcome_value = self.parse_model_output(text_response)
        elif isinstance(outcome_value, (int, float)) and self.config.outcome_range:
            min_value, max_value = self.config.outcome_range
            outcome_value = min(max(int(outcome_value), min_value), max_value)

        streamed_live = bool(player and player.frames_written > 0)

        transcript = ""
        if transcription_task:
            t0 = time.time()
            try:
                transcript = await asyncio.wait_for(
                    transcription_task, timeout=TRANSCRIPTION_TIMEOUT_SECONDS
                )
            except asyncio.TimeoutError:
                logger.error(
                    "Transcription task timed out after %s seconds",
                    TRANSCRIPTION_TIMEOUT_SECONDS,
                )
            except asyncio.CancelledError:
                logger.warning("Transcription task cancelled before completion")
            except Exception as exc:
                logger.error("Transcription task failed: %s", exc, exc_info=True)
            else:
                transcript = transcript.strip()
                if transcript:
                    logger.info("[Transcript] %s", transcript)
                else:
                    logger.warning("Transcription task completed but returned empty text")
            finally:
                timings["transcription_wait"] = time.time() - t0
                if transcription_start:
                    timings["transcription_total"] = time.time() - transcription_start

        logger.info("=== GEMINI API PERFORMANCE ===")
        if "time_to_first_response" in timings:
            logger.info("  Time to first response: %.3fs", timings["time_to_first_response"])
        if "time_to_first_audio_chunk" in timings:
            logger.info("  Time to first audio chunk: %.3fs", timings["time_to_first_audio_chunk"])
        if "streaming_duration" in timings:
            logger.info("  Total streaming duration: %.3fs", timings["streaming_duration"])
        if "time_to_outcome_detection" in timings:
            logger.info("  Time to outcome detection: %.3fs", timings["time_to_outcome_detection"])
        if "transcription_total" in timings:
            logger.info("  Transcription total time: %.3fs", timings["transcription_total"])
        if "transcription_wait" in timings:
            logger.info("  Transcription await time: %.3fs", timings["transcription_wait"])
        logger.info("  Audio chunks received: %d", len(audio_chunks))
        logger.info("  Total audio duration: %.2fs", streamed_audio_seconds)
        logger.info("==============================")

        return InteractionResult(
            audio_bytes=audio_bytes,
            text_response=text_response,
            outcome_value=outcome_value,
            streamed_live=streamed_live,
            timings=timings,
            transcript=transcript,
        )

    def try_extract_outcome(self, text: str) -> Optional[int]:
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

    def parse_model_output(self, text: str) -> int:
        """Extract configured outcome from response text or default."""
        value = self.try_extract_outcome(text)
        if value is not None:
            if isinstance(value, (int, float)) and self.config.outcome_range:
                min_value, max_value = self.config.outcome_range
                clamped = min(max(int(value), min_value), max_value)
                logger.info("Extracted %s: %s", self.config.outcome_label.lower(), clamped)
                return clamped
            logger.info("Extracted %s: %s", self.config.outcome_label.lower(), value)
            return value

        logger.warning(
            "No %s found in text '%s', defaulting to %s",
            self.config.outcome_label.lower(),
            text,
            self.config.default_value,
        )
        return int(self.config.default_value)


__all__ = [
    "GeminiJudge",
    "GeminiTauntSpeaker",
    "InteractionResult",
    "TRANSCRIPTION_TIMEOUT_SECONDS",
]
