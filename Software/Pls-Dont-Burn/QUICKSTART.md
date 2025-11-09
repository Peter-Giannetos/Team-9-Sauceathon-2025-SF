# Quick Start Guide

Get up and running in 3 minutes!

## 1️⃣ Install Dependencies (1 min)

```bash
pip install -r requirements.txt
```

## 2️⃣ Get API Key (1 min)

1. Go to https://ai.google.dev/
2. Click "Get API key"
3. Sign in with Google
4. Create and copy your API key

## 3️⃣ Configure (30 sec)

```bash
cp .env.example .env
```

Edit `.env` and paste your API key:
```
GOOGLE_API_KEY=your_actual_key_here
```

## 4️⃣ Test (optional, 30 sec)

```bash
python test_setup.py
```

## 5️⃣ Run! (30 sec)

```bash
python bun_controller.py
```

## What You'll See

```
🍔 Welcome to the AI Burger Bun Toast Controller!
   Powered by Gemini Live API

🔥 BURGER BUN TOASTER AI
==================================================
🤖 AI: Convince me not to burn your burger bun. You have 5 seconds. Go!
[AI speaks the taunt]

🎤 Recording for 5 seconds...
[Speak your plea!]

🤖 AI is judging your plea...
🤖 AI Response: [Snarky response]

🔥 BURN LEVEL: X/9
▶️  Playing AI response...
```

## Hardware Integration

See `hardware_example.py` for code to send burn level to:
- Arduino/ESP32 (Serial)
- Raspberry Pi (GPIO)
- ESP32 (HTTP)

## Troubleshooting

**No microphone input?**
- Check microphone permissions in System Settings
- Make sure mic is not muted

**API errors?**
- Verify API key is correct
- Check internet connection
- Ensure you have access to Gemini API

**Need help?**
- See `SETUP.md` for detailed troubleshooting
- Run `python test_setup.py` to diagnose issues

## Key Features

✅ **Single API** - Gemini Live handles everything
✅ **Fast** - 5-7 second total cycle time
✅ **Cheap** - ~$0.001-0.003 per interaction
✅ **Simple** - Only 6 Python packages needed
✅ **Real-time** - Native audio streaming

Enjoy burning (or not burning) your burger buns! 🍔🔥
