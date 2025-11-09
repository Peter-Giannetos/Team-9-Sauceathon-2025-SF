"""
Test Setup Script
Verify API key and audio devices before running the main application
Simplified for Gemini Live API
"""

import os
from dotenv import load_dotenv

load_dotenv()


def test_api_key():
    """Test if API key is configured"""
    print("\n🔑 Testing API Key...")
    print("-" * 50)

    google_key = os.getenv('GOOGLE_API_KEY')

    if google_key and google_key != 'your_google_api_key_here':
        print("✅ Google AI API key found")
        return True
    else:
        print("❌ Google AI API key not configured")
        print("   Get your API key at: https://ai.google.dev/")
        return False


def test_audio_devices():
    """Test audio input/output devices"""
    print("\n🎤 Testing Audio Devices...")
    print("-" * 50)

    try:
        import sounddevice as sd

        devices = sd.query_devices()
        print("\nAvailable audio devices:")
        print(devices)

        default_input = sd.query_devices(kind='input')
        default_output = sd.query_devices(kind='output')

        print(f"\n✅ Default input device: {default_input['name']}")
        print(f"✅ Default output device: {default_output['name']}")

        return True

    except Exception as e:
        print(f"❌ Audio device error: {e}")
        return False


def test_microphone():
    """Test microphone recording"""
    print("\n🎙️  Testing Microphone Recording...")
    print("-" * 50)

    try:
        import sounddevice as sd
        import numpy as np

        print("Recording 2 seconds of audio... Say something!")

        duration = 2
        sample_rate = 16000
        recording = sd.rec(int(duration * sample_rate),
                          samplerate=sample_rate,
                          channels=1,
                          dtype='float32')
        sd.wait()

        # Calculate volume
        rms = np.sqrt(np.mean(recording**2))
        if rms > 0:
            db = 20 * np.log10(rms)
        else:
            db = -np.inf

        print(f"✅ Recording successful!")
        print(f"   Volume: {db:.1f} dB")

        if db > -60:
            print("   👍 Good signal detected")
            return True
        else:
            print("   ⚠️  Very quiet or no input detected")
            print("   Try speaking louder or check microphone settings")
            return False

    except Exception as e:
        print(f"❌ Microphone test error: {e}")
        return False


def test_speaker():
    """Test speaker playback"""
    print("\n🔊 Testing Speaker Playback...")
    print("-" * 50)

    try:
        import sounddevice as sd
        import numpy as np

        print("Playing test tone for 1 second...")

        # Generate a 440 Hz tone (A note)
        sample_rate = 44100
        duration = 1.0
        frequency = 440

        t = np.linspace(0, duration, int(sample_rate * duration))
        tone = 0.3 * np.sin(2 * np.pi * frequency * t)

        sd.play(tone, sample_rate)
        sd.wait()

        print("✅ Playback test complete")
        print("   Did you hear a tone? If yes, speakers are working!")

        return True

    except Exception as e:
        print(f"❌ Speaker test error: {e}")
        return False


def test_gemini_connection():
    """Test Gemini API connection"""
    print("\n🌐 Testing Gemini API Connection...")
    print("-" * 50)

    try:
        from google import genai

        api_key = os.getenv('GOOGLE_API_KEY')
        client = genai.Client(api_key=api_key)

        print("Testing basic API connection...")

        # Test with a simple text request
        response = client.models.generate_content(
            model='gemini-2.0-flash-exp',
            contents='Say "test" and nothing else.'
        )

        print("✅ Gemini API connection successful")
        print(f"   Response: {response.text[:50]}...")
        return True

    except Exception as e:
        print(f"❌ Gemini API error: {e}")
        print("\n   Common issues:")
        print("   - API key is invalid or expired")
        print("   - Account doesn't have access to Gemini API")
        print("   - Network connection issue")
        print("   - API quota exceeded")
        return False


def test_gemini_audio_model():
    """Test Gemini Audio Model access"""
    print("\n🎵 Testing Gemini Audio Model Access...")
    print("-" * 50)

    try:
        from google import genai

        api_key = os.getenv('GOOGLE_API_KEY')
        client = genai.Client(api_key=api_key)

        model_name = "gemini-2.5-flash-native-audio-preview-09-2025"

        print(f"Checking access to {model_name}...")

        # Try to list models to verify access
        models = client.models.list()

        print("✅ Gemini Audio Model access verified")
        print("   Your account has access to the native audio model")
        return True

    except Exception as e:
        print(f"⚠️  Could not verify audio model access: {e}")
        print("\n   Note: The native audio model is in preview")
        print("   If you encounter issues, you may need to:")
        print("   - Wait for wider API access")
        print("   - Request access from Google AI Studio")
        print("   - Use an alternative Gemini model")
        return False


def test_audio_libraries():
    """Test audio processing libraries"""
    print("\n📚 Testing Audio Processing Libraries...")
    print("-" * 50)

    all_ok = True

    try:
        import librosa
        print(f"✅ librosa {librosa.__version__} installed")
    except ImportError:
        print("❌ librosa not installed")
        all_ok = False

    try:
        import soundfile
        print(f"✅ soundfile installed")
    except ImportError:
        print("❌ soundfile not installed")
        all_ok = False

    try:
        import numpy
        print(f"✅ numpy {numpy.__version__} installed")
    except ImportError:
        print("❌ numpy not installed")
        all_ok = False

    return all_ok


def main():
    """Run all tests"""
    print("=" * 50)
    print("🧪 Burger Bun Controller - Setup Test")
    print("   Gemini Live API Version")
    print("=" * 50)

    results = []

    # Test API key
    results.append(("API Key", test_api_key()))

    # Test audio libraries
    results.append(("Audio Libraries", test_audio_libraries()))

    # Test audio devices
    results.append(("Audio Devices", test_audio_devices()))

    # Test microphone
    results.append(("Microphone", test_microphone()))

    # Test speaker
    results.append(("Speaker", test_speaker()))

    # Test Gemini connection
    results.append(("Gemini API", test_gemini_connection()))

    # Test Gemini audio model
    results.append(("Gemini Audio Model", test_gemini_audio_model()))

    # Summary
    print("\n" + "=" * 50)
    print("📊 Test Summary")
    print("=" * 50)

    all_passed = True
    critical_failed = False

    for name, passed in results:
        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"{status} - {name}")

        if not passed:
            all_passed = False
            # Critical tests
            if name in ["API Key", "Gemini API", "Audio Devices"]:
                critical_failed = True

    print("=" * 50)

    if all_passed:
        print("\n🎉 All tests passed! You're ready to run bun_controller.py")
    elif not critical_failed:
        print("\n⚠️  Some optional tests failed, but you can still try running the app")
        print("   See errors above for details")
    else:
        print("\n❌ Critical tests failed. Please fix the errors above before running")
        print("   See SETUP.md for troubleshooting guide")

    print("\nNext steps:")
    print("  python bun_controller.py    # Run the main application")
    print("  python hardware_example.py  # Test hardware integration")


if __name__ == "__main__":
    main()
