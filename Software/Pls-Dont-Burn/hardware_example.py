"""
Example Hardware Integration
Demonstrates how to send burn level to external hardware
Using simplified Gemini Live API version
"""

from bun_controller import BunController
import os
from dotenv import load_dotenv

load_dotenv()


def send_to_serial(burn_level, port='/dev/ttyUSB0', baudrate=9600):
    """
    Send burn level to Arduino/microcontroller via serial

    Args:
        burn_level: Integer 0-9
        port: Serial port (e.g., '/dev/ttyUSB0', 'COM3')
        baudrate: Serial communication speed
    """
    try:
        import serial

        ser = serial.Serial(port, baudrate, timeout=1)
        command = f"{burn_level}\n"
        ser.write(command.encode())
        ser.close()

        print(f"✅ Sent burn level {burn_level} to {port}")
        return True

    except ImportError:
        print("❌ pyserial not installed. Install with: pip install pyserial")
        return False
    except Exception as e:
        print(f"❌ Serial communication error: {e}")
        return False


def send_to_gpio(burn_level):
    """
    Control GPIO pins on Raspberry Pi
    Maps burn level to PWM duty cycle (0-100%)
    """
    try:
        import RPi.GPIO as GPIO

        # Setup
        HEATER_PIN = 18  # GPIO pin for heater control
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(HEATER_PIN, GPIO.OUT)

        # Create PWM instance (100 Hz frequency)
        pwm = GPIO.PWM(HEATER_PIN, 100)

        # Convert burn level (0-9) to duty cycle (0-100%)
        duty_cycle = (burn_level / 9) * 100

        pwm.start(duty_cycle)
        print(f"✅ Set heater PWM to {duty_cycle:.1f}% (burn level {burn_level})")

        return True

    except ImportError:
        print("❌ RPi.GPIO not installed (only works on Raspberry Pi)")
        return False
    except Exception as e:
        print(f"❌ GPIO error: {e}")
        return False


def send_to_http(burn_level, endpoint="http://192.168.1.100/burn"):
    """
    Send burn level to HTTP endpoint (e.g., ESP32, web service)
    """
    try:
        import requests

        response = requests.post(
            endpoint,
            json={"burn_level": burn_level},
            timeout=2
        )

        if response.status_code == 200:
            print(f"✅ Sent burn level {burn_level} to {endpoint}")
            return True
        else:
            print(f"⚠️ HTTP {response.status_code}: {response.text}")
            return False

    except Exception as e:
        print(f"❌ HTTP error: {e}")
        return False


def main():
    """Main hardware integration example"""

    print("🔥 Burger Bun Controller - Hardware Integration Example")
    print("   Powered by Gemini Live API\n")

    # Verify API key
    if not os.getenv('GOOGLE_API_KEY'):
        print("❌ Error: GOOGLE_API_KEY not found in .env file")
        print("   Get your API key at: https://ai.google.dev/")
        return

    # Create controller
    controller = BurnController()

    try:
        # Run the AI interaction cycle
        burn_level = controller.run_cycle()

        print(f"\n{'='*50}")
        print(f"🔥 FINAL BURN LEVEL: {burn_level}/9")
        print(f"{'='*50}\n")

        # Send to hardware (choose your method)

        # Option 1: Serial (Arduino, ESP32, etc.)
        # send_to_serial(burn_level, port='/dev/ttyUSB0')

        # Option 2: GPIO (Raspberry Pi)
        # send_to_gpio(burn_level)

        # Option 3: HTTP (ESP32, web service)
        # send_to_http(burn_level, endpoint="http://192.168.1.100/burn")

        # For now, just print it
        print(f"💡 To send to hardware, uncomment the appropriate method above")
        print(f"   and configure the connection parameters.\n")

    except KeyboardInterrupt:
        print("\n\n👋 Interrupted by user")
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
