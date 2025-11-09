# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Burger Bun Toast Controller** - Voice-first MVP that uses AI to decide how hard to toast a burger bun. Users plead with the AI for 5 seconds, and Gemini Live API judges their argument (factoring in loudness and content) to output a burn level (0-9) that controls hardware.

**Tech Stack:**
- **Python**: 3.9+ with sounddevice, numpy, librosa, soundfile, google-genai 1.49
- **AI Model**: `gemini-2.5-flash-native-audio-preview-09-2025` via Gemini Live API (turn-based mode)
- **Hardware**: ESP32 (upesy_wroom) for servo/heater control via PlatformIO
- **Target Cycle Time**: <10 seconds from taunt to burn-level output

## Development Commands

### Python Application

```bash
# Setup
pip install -r requirements.txt
cp .env.example .env  # Then add GOOGLE_API_KEY

# Run
python bun_controller.py          # Normal mode
python bun_controller.py --debug  # Verbose logging

# Test environment
python test_setup.py  # Verify API key, audio devices, Gemini connection
```

### Hardware (ESP32)

```bash
# Build and upload (from Demo-Heater/ or Demo-Servo/)
pio run              # Compile
pio run -t upload    # Upload to board
pio device monitor   # Serial monitor
```

**Hardware Projects:**
- `Demo-Heater/` - Heater control with serial commands (ON/OFF)
- `Demo-Servo/` - Parallax servo control (clockwise/counterclockwise)
- `Demo-Skeleton/` - Minimal template for new devices
- `Demo-NoiseSensor/` - Example sensor integration

## Architecture

### Core Components

**`bun_controller.py`** - Main application orchestrating the burn decision cycle:

1. **InteractionConfig** (lines 46-94) - Configuration system for prompts, scoring, and theming
   - Decouples topic/persona/scoring from controller logic
   - Supports custom regex patterns and value parsers
   - Default: burger bun toaster (0-9 burn scale)

2. **AudioRecorder** (lines 115-160) - Microphone capture and loudness analysis
   - Records at 16 kHz mono, calculates RMS/dB volume
   - `audio_to_wav()` encodes to 16-bit PCM WAV (required for Gemini)

3. **BunController** (lines 163-373) - Gemini Live API integration
   - Turn-based mode: single request with prompt text + audio → audio response
   - Response parsing extracts burn level via regex
   - Audio playback at 24 kHz (Gemini's output rate)

### Execution Flow

```
Taunt → Record (5s) → Build Context → Gemini Request →
Stream Response → Parse Burn Level → Play Audio → Hardware Output
```

**Key Method:** `run_cycle_async()` (lines 324-368)
- Synchronous wrapper: `run_cycle()` (line 370-372)

### Hardware Integration

**`hardware_example.py`** - Three integration patterns:
- `send_to_serial()` - Arduino/ESP32 via pyserial
- `send_to_gpio()` - Raspberry Pi PWM (burn level → duty cycle)
- `send_to_http()` - REST API for remote devices

**Note:** Example includes typo on line 112 (`BurnController` should be `BunController`)

### Gemini Live API Details

**Configuration:**
- `LiveConnectConfig(response_modalities=["AUDIO"])` - Audio-only response
- Message structure: `types.Content` with text prompt + inline WAV
- Response streaming: `session.receive()` yields audio chunks + optional text

**Critical Implementation Notes:**
- Turn-based mode requires complete WAV container (not raw PCM)
- Don't mix `send_client_content` and `send_realtime_input`
- Audio must be properly encoded with soundfile before sending
- Text responses are suppressed when audio arrives (expected behavior)

### Configuration System

**InteractionConfig Pattern** - Swap themes without code changes:

```python
# Example: Butter dispenser instead of toaster
buttery_config = InteractionConfig(
    taunt="Convince me to lavish your bun with butter.",
    persona="an indulgent sous-chef",
    outcome_label="Butter level",
    outcome_range=(0, 5),
    # ... rest of config
)
controller = BunController(config=buttery_config)
```

**Custom Parsing:**
- `extraction_patterns` - Regex list for value extraction
- `value_parser` - Optional callable to transform matched strings
- `default_value` - Fallback when parsing fails

## Key Implementation Constraints

1. **Audio Encoding**: Gemini requires WAV containers, not raw PCM. Use `AudioRecorder.audio_to_wav()`.

2. **Loudness Calculation**: `describe_volume()` provides contextual descriptors:
   - < -30 dB: "whispered - weak"
   - > -10 dB: "yelled - desperate"
   - Default: "normal volume"

3. **Burn Level Parsing**: Regex supports multiple patterns (lines 254-283):
   - `Burn level X`
   - `Level X`
   - `Score X`
   - Falls back to `default_value` (5) with warning

4. **Performance Target**: Complete cycle must finish in <10 seconds
   - Typical: 6-7 seconds on stable connection
   - Warning logged if exceeded (line 364)

## Testing & Debugging

**`test_setup.py`** provides comprehensive environment validation:
- API key configuration
- Audio device enumeration
- Microphone recording test (2s with dB measurement)
- Speaker playback test (440 Hz tone)
- Gemini API connection verification
- Native audio model access check

**Common Issues:**
- **No audio output**: Check `--debug` logs for "Received audio chunk"
- **API errors**: Verify `GOOGLE_API_KEY` and preview model access
- **Audio device errors**: Run `test_setup.py` to verify OS permissions
- **Frame errors (1007)**: Indicates raw PCM sent instead of WAV container

**Saved Files:**
- `ai_response.wav` - Last AI response (24 kHz, debugging)

## Hardware Integration Notes

**ESP32 Pin Configuration** (Demo-Heater):
- LED_PIN: GPIO 2 (status indicator)
- OUT_PIN: GPIO 19 (heater control)
- Serial: 115200 baud with "GO" start command

**Servo Control** (Demo-Servo):
- Parallax continuous rotation servo
- Clockwise: 1300μs pulse
- Counterclockwise: 1700μs pulse
- 20ms frame period

**Serial Protocol:**
- Python sends: `"{burn_level}\n"` (e.g., "7\n")
- ESP32 expects: Text commands or numeric values
- Demo-Heater accepts: "ON", "OFF"

## Extension Points

1. **Multi-round play**: Wrap `run_cycle()` in loop or Flask/FastAPI endpoint
2. **Alternative scales**: Modify `outcome_range` (e.g., butter 0-5, timer penalties)
3. **Custom prompts**: Create new `InteractionConfig` instances
4. **Model swapping**: Change `self.model` to other Gemini Live variants
5. **Hardware feedback**: Add sensor readings to context (e.g., current toast level)

## File Dependencies

**Critical Path:**
- `bun_controller.py` → `google.genai` → Live API
- Audio chain: `sounddevice` → `numpy` → `soundfile` (WAV encoding) → Gemini
- Response chain: Gemini audio → `numpy.frombuffer` → `sounddevice` playback

**Environment:**
- `.env` must contain `GOOGLE_API_KEY` (not committed)
- Virtual environment recommended (`python -m venv .venv`)

## Code Patterns to Follow

1. **Async/Await**: All Gemini Live API calls use `async with` context managers
2. **Error Handling**: Log with `logger.error(..., exc_info=True)` for full tracebacks
3. **Debug Logging**: Use `logger.debug()` for detailed flow, `logger.info()` for milestones
4. **Configuration-Driven**: Prefer InteractionConfig changes over hardcoded prompt modifications
5. **Container Encoding**: Always wrap audio in standard containers (WAV) before Gemini transmission
