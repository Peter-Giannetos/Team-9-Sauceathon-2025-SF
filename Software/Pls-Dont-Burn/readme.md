# Burger Bun Toast Controller

Voice-first MVP that decides how hard to toast your burger bun. The system listens to a five-second plea, streams it to the Gemini Live API, plays the model's audio judgment, and emits a burn level (0-9) that can drive hardware. This README captures the current implementation so you can iterate quickly during the hackathon.

---

## Project Overview

- **Goal**: Convince the AI not to burn the bun. Louder, more compelling arguments earn a lower burn level.
- **Stack**: `python 3.9+`, `sounddevice`, `numpy`, `librosa`, `soundfile`, and `google-genai 1.49`.
- **AI Model**: `gemini-2.5-flash-native-audio-preview-09-2025` using the Live API in turn-based mode (single request -> audio response).
- **Cycle Time Target**: <10 seconds from taunt to burn-level output.
- **Key Files**
  - `bun_controller.py` - main loop and Gemini integration.
  - `hardware_example.py` - serial/GPIO/HTTP sketches for wiring the burn level to devices.
  - `test_setup.py` - environment and audio sanity tests.
  - `SETUP.md` - step-by-step install guide (useful for teammates).

---

## Setup

1. **Dependencies**
   ```bash
   pip install -r requirements.txt
   ```
2. **Environment**
   ```bash
   cp .env.example .env
   # edit .env and set GOOGLE_API_KEY
   ```
3. **Sanity Check (optional)**
   ```bash
   python test_setup.py
   ```

Run the controller:

```bash
python bun_controller.py            # normal mode
python bun_controller.py --debug    # verbose logging
```

> Note: `GOOGLE_API_KEY` must point to an account with access to the Gemini native audio preview model.

---

## Execution Flow

1. **Taunt** - Console prints the AI challenge so participants know the rules.
2. **Capture** - `AudioRecorder.record_audio` grabs 5 seconds at 16 kHz mono and computes RMS/decibel loudness.
3. **Context Building** - Burn controller crafts a sassy prompt that includes the measured dB so the model can factor in loudness.
4. **Gemini Request** - Prompt text and WAV audio are packaged into one `send_client_content` call using the Live API; response modality is audio (text still arrives via mixed parts).
5. **Response Handling** - Audio chunks stream directly to the speaker via `AudioStreamPlayer` while any accompanying text parts are parsed on the fly for the burn level. The loop exits once the score is known and ~1.5 s of audio have aired.
6. **Playback** - Live streaming removes the need to block on the full clip; audio is still saved as `ai_response.wav` for debugging.
7. **Burn Level Extraction** - Regex looks for `Burn level X` and clamps to 0-9. Missing values default to 5.
8. **Return Value** - `run_cycle()` returns the burn level so hardware integrations can act on it.

The entire flow lives in `BunController.run_cycle_async()` and is wrapped by `run_cycle()` for synchronous callers.

---

## Implementation Details

### InteractionConfig (`bun_controller.py:46-103`)
- Captures persona, taunt, prompt template, outcome label/range, and optional parsing logic.
- Pass a new instance into `BunController(config=...)` to change topics, scoring scales, or even switch to string-based outputs without touching controller code.
- Supports custom regex patterns and parsers (e.g., map phrases like "extra crispy" to numbers).

### AudioRecorder (`bun_controller.py:34-84`)
- Centralizes microphone sampling and loudness calculations.
- Provides `audio_to_wav` which uses `soundfile` to encode 16-bit PCM WAV. This container is required for Gemini's turn-based ingestion.

### AudioStreamPlayer (`bun_controller.py:86-161`)
- Wraps `sounddevice.OutputStream` (float32) to write each Gemini audio chunk immediately. Tracks frames so the controller can fall back to post-playback if streaming failed.

### AudioStreamPlayer (`bun_controller.py:232-278`)
- Wraps `sounddevice.OutputStream` (float32) to write each Gemini audio chunk immediately. Tracks frames so the controller can fall back to post-playback if streaming failed.

### Gemini Client (`bun_controller.py:283-420`)
- Uses `google.genai.Client().aio.live.connect`.
- **Config**: `LiveConnectConfig(response_modalities=["AUDIO"])` (Gemini still emits text metadata alongside audio chunks).
- **Message Structure**: a single `types.Content` with two `types.Part` objects (prompt text + inline WAV bytes).
- Response loop inspects each `model_turn.part`:
  - Streams inline audio bytes to speakers while buffering for saving.
  - Collects text parts (excluding `thought` content); once the burn level is detected and ≥1.5 s of audio have played, it exits early to minimize latency.
- **Prompt logging**: The entire prompt length and text are logged so you can confirm the full instruction set is sent over the websocket.

### Burn Level Parsing (`bun_controller.py:183-201`)
- Regex supports `Burn level X`, `Level X`, and `BURN_LEVEL X`.
- Falls back to 5 with a warning when not found.

### Output + Hardware
- `play_audio` uses `sounddevice` (24 kHz) so the model's voice is audible immediately.
- `save_audio` writes a debugging WAV file for demos.
- Returned burn level can feed any transport; see `hardware_example.py` for serial, GPIO PWM, and HTTP snippets. Each helper is isolated so you can drop it into the main loop or a microservice.

---

## Data Flow Summary

| Stage | Input | Processing | Output |
| --- | --- | --- | --- |
| Capture | Microphone (16 kHz) | `numpy` RMS + `sounddevice` buffer | raw float array + dB |
| Packaging | raw audio + dB | `audio_to_wav`, context prompt | Gemini Live turn |
| AI | turn payload | Native audio model | streamed audio chunks + optional text |
| Post-processing | audio chunks | `numpy.frombuffer`, `wave` save | playback + file |
| Control | AI text | regex clamp | burn level 0-9 |

---

## Debugging Tips

- **No Audio Output**: Ensure Gemini returned audio chunks (enable `--debug`; should see "Received audio chunk"). If zero, check API access or response modalities. The streamer falls back gracefully if chunks are empty.
- **API Key Issues**: Run `python test_setup.py` to confirm token and network access.
- **Audio Device Errors**: Use `test_setup.py` (microphone/speaker tests) to verify OS permissions.
- **Slow Cycles**: Network latency is most common; confirm you are not running VPNs or heavy downloads.
- **Gemini Frame Errors (1007)**: Occur if you send raw PCM via `send_client_content`. Always wrap audio in a standard container (our current code handles this).
- **No Text Extracted**: The Live API sometimes delays text when only audio is requested. We parse parts manually, but if nothing arrives the controller falls back to the default burn level (5). Adjust the prompt to insist on the "Burn level X" line if this happens often.

---

## Extending the System

1. **Custom Prompts / Topics**  
   Instantiate `BunController(config=InteractionConfig(...))` with a new persona, scenario, and taunt to swap themes (e.g., buttering instead of burning). Example:
   ```python
   buttery_config = InteractionConfig(
       taunt="Convince me to lavish your bun with butter.",
       persona="an indulgent sous-chef",
       scenario="You control a butter dispenser instead of the toaster.",
       instructions="Judge their plea and decide how buttery the bun should be.",
       outcome_label="Butter level",
       outcome_range=(0, 5),
       outcome_scale_description="0 = dry, 5 = drenched in butter.",
       default_value=2,
   )
   controller = BunController(config=buttery_config)
   ```

2. **Hardware Hooks**  
   - Serial: `hardware_example.send_to_serial(burn_level, port="COM4")`.
   - GPIO: Map burn level to PWM duty cycle on microcontrollers.
   - HTTP: Post burn levels to a REST API for remote toasters or dashboards.

3. **Multi-Round Play**  
   Wrap `run_cycle()` in a loop with a delay or integrate into a Flask/FastAPI endpoint to serve multiple kiosks.

4. **Alternative Output Scales**  
   Change `InteractionConfig.outcome_label` and `outcome_range` (even use strings via custom parser) to represent butter levels, timer penalties, etc.

5. **Alternative Models**  
   Swap `self.model` for other Gemini Live variants. Ensure `response_modalities` and MIME types match model capabilities.

6. **Testing**  
   - Add unit tests for `parse_model_output`.
   - Mock `AudioRecorder` for automated runs (feed saved WAV files).

---

## Technical Learnings

- **Live API Modes**: Do not mix `send_client_content` and `send_realtime_input` unless you know how the context is ordered. Turn-based mode expects the whole request at once.
- **Audio Encoding**: Inline audio must be a complete container when sent as part of `client_content`. Raw PCM is only valid via `send_realtime_input`.
- **Response Parsing**: `LiveServerMessage.text` warns when non-text parts are present; skip it on frames that contain `inline_data` to keep logs clean.
- **Performance**: Recording, encoding, and Gemini round-trip typically finish within 6-7 seconds on a stable connection, well inside the 10-second requirement.

The combination of these patterns keeps the MVP simple while leaving clear seams for future expansion (multiple players, hardware feedback loops, alternative scoring, etc.). Use this README as the canonical reference when onboarding teammates or pitching improvements.
