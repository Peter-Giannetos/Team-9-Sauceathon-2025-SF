# Burger Bun Toast Controller - Setup Guide

**Simplified version using Gemini Live API - One API for everything!**

## Prerequisites

- Python 3.9 or higher
- Microphone access
- Speaker/audio output

## Installation

### 1. Install Python Dependencies

```bash
pip install -r requirements.txt
```

### 2. Get Google AI API Key

1. Go to https://ai.google.dev/
2. Click "Get API key in Google AI Studio"
3. Sign in with your Google account
4. Create a new API key
5. Copy the API key

### 3. Configure API Key

1. Copy the example environment file:
```bash
cp .env.example .env
```

2. Edit `.env` and add your API key:
```
GOOGLE_API_KEY=your_actual_google_api_key
```

## Usage

Run the controller:
```bash
python bun_controller.py
```

### How It Works

1. **Taunt Phase**: AI speaks: "Convince me not to burn your burger bun. You have 5 seconds. Go!"
2. **Recording Phase**: You have 5 seconds to speak your plea
3. **AI Processing**:
   - Gemini Live API processes your audio directly
   - Volume level is measured and considered
   - AI judges your argument
4. **Response Phase**:
   - AI responds with snarky audio feedback
   - Assigns a burn level (0-9)
5. **Output**: Returns burn level for hardware control

### Burn Levels

- **0**: No toasting (perfect argument)
- **1-3**: Light toast (good argument)
- **4-6**: Medium toast (okay argument)
- **7-8**: Heavy toast (weak argument)
- **9**: Completely charred (terrible/no argument)

## Advantages of Gemini Live API

✅ **Single API** - Audio input → Processing → Audio output
✅ **Faster** - No multiple API calls (Whisper + LLM + TTS)
✅ **Cheaper** - One API instead of three
✅ **Simpler** - Less code, fewer dependencies
✅ **Better latency** - Native audio processing

## Troubleshooting

### No microphone input
- Check microphone permissions
- Verify microphone is default input device
- Test: `python -c "import sounddevice as sd; print(sd.query_devices())"`

### Audio playback issues
- Ensure speakers/headphones connected
- Check system volume
- Verify audio device in system settings

### API errors
- Verify API key is correct in `.env`
- Check you have access to Gemini 2.5 Flash Native Audio model
- Ensure internet connection is stable
- Note: The model is in preview, make sure your account has access

### Slow performance
- Check internet connection speed
- Ensure minimal background network usage
- Gemini Live API is optimized for real-time streaming

## Hardware Integration

The burn level (0-9) is returned from `controller.run_cycle()` and can be sent to hardware.

See `hardware_example.py` for examples:
- Serial communication (Arduino/ESP32)
- GPIO control (Raspberry Pi)
- HTTP endpoint (ESP32/web service)

Example:
```python
from bun_controller import BunController

controller = BunController()
burn_level = controller.run_cycle()

# Send to hardware
print(f"Burn level: {burn_level}")
```

## Performance Notes

- **Target**: <10 seconds total cycle time ✅
- **Typical**: 5-7 seconds with good internet
- **Advantages**: Single API call, native audio streaming
- **Optimization**: Already using fastest Gemini model

## Cost Estimates

Per interaction using Gemini Live API:
- **Audio input**: ~$0.00025 per second
- **Audio output**: ~$0.00025 per second
- **Text processing**: Minimal additional cost

**Total per cycle: ~$0.001-0.003** (70% cheaper than multi-API approach!)

## Testing

Run the test script to verify setup:
```bash
python test_setup.py
```

This will check:
- API key configuration
- Audio devices (mic/speakers)
- Gemini API connection
- Audio recording/playback

## API Rate Limits

Google AI Studio (free tier):
- 15 requests per minute
- 1,500 requests per day
- 1 million tokens per minute

This is plenty for testing and demo purposes!
