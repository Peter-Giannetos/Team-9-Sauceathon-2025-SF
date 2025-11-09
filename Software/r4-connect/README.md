# Arduino UNO R4 WiFi - Proximity Trigger System

A simple system where an Arduino UNO R4 WiFi monitors a proximity sensor and triggers audio playback on a computer when proximity is detected.

## 🎯 Overview

**How it works:**
1. Arduino UNO R4 WiFi monitors a proximity sensor
2. When close proximity is detected, Arduino sends an HTTP GET request to your computer
3. Python server running on your computer receives the request
4. Server plays `taunt_prompt.wav` audio file

## 📋 Requirements

### Hardware
- Arduino UNO R4 WiFi board
- HC-SR04 Ultrasonic Distance Sensor (or compatible)
- USB cable for Arduino
- Computer on the same WiFi network

### Software
- Arduino IDE or PlatformIO
- Python 3.6 or higher (for the server)
- WiFi network (both Arduino and computer must be connected)

## 🚀 Quick Start

### Step 1: Set Up the Server (Computer)

1. **Navigate to the project directory:**
   ```bash
   cd Software/r4-connect
   ```

2. **Run the Python server:**
   ```bash
   python3 server.py
   ```

3. **Note the IP address displayed:**
   ```
   Local IP Address: 192.168.1.100
   ```
   You'll need this for the Arduino configuration.

### Step 2: Configure the Arduino Code

1. **Open `src/main.cpp` in Arduino IDE or PlatformIO**

2. **Update WiFi credentials:**
   ```cpp
   const char* WIFI_SSID = "YourWiFiName";
   const char* WIFI_PASSWORD = "YourWiFiPassword";
   ```

3. **Update server configuration:**
   ```cpp
   const char* SERVER_HOST = "192.168.1.100";  // Use IP from Step 1
   const int SERVER_PORT = 8080;                // Match server port
   ```

4. **Configure sensor settings (optional - already configured for HC-SR04):**
   ```cpp
   const int TRIG_PIN = 8;                      // Trigger pin (already set)
   const int ECHO_PIN = 9;                      // Echo pin (already set)
   const int DISTANCE_THRESHOLD = 20;           // Distance in cm to trigger
   ```

### Step 3: Upload to Arduino

1. **Connect Arduino UNO R4 WiFi via USB**

2. **Upload the sketch:**
   - **Arduino IDE:** Click Upload button
   - **PlatformIO:** Run `pio run --target upload`

3. **Open Serial Monitor (115200 baud)** to see debug output

### Step 4: Test the System

1. **Verify Arduino connects to WiFi:**
   - Check Serial Monitor for "SUCCESS!" message
   - Note the Arduino's IP address

2. **Trigger the sensor:**
   - Move your hand or object close to the proximity sensor
   - Watch Serial Monitor for "PROXIMITY DETECTED!" message
   - Server should play the audio file

## 🔧 Configuration Options

### Arduino Configuration (`main.cpp`)

| Setting | Description | Default |
|---------|-------------|---------|
| `WIFI_SSID` | Your WiFi network name | "YOUR_WIFI_SSID" |
| `WIFI_PASSWORD` | Your WiFi password | "YOUR_WIFI_PASSWORD" |
| `SERVER_HOST` | Computer's IP address | "192.168.1.100" |
| `SERVER_PORT` | Server port number | 8080 |
| `SERVER_PATH` | HTTP endpoint path | "/trigger" |
| `TRIG_PIN` | Ultrasonic trigger pin | 8 |
| `ECHO_PIN` | Ultrasonic echo pin | 9 |
| `DISTANCE_THRESHOLD` | Trigger distance (cm) | 20 |
| `COOLDOWN_MS` | Min time between triggers | 2000 ms |

### Server Configuration (`server.py`)

| Setting | Description | Default |
|---------|-------------|---------|
| `PORT` | Server listening port | 8080 |
| `AUDIO_FILE` | Audio file to play | "taunt_prompt.wav" |

## 🔌 Sensor Wiring

### HC-SR04 Ultrasonic Sensor
```
Sensor VCC  → Arduino 5V
Sensor GND  → Arduino GND
Sensor TRIG → Arduino Pin 8
Sensor ECHO → Arduino Pin 9
```

**Note:** The ultrasonic sensor measures distance and triggers when an object is closer than `DISTANCE_THRESHOLD` (default: 20 cm).

## 🐛 Troubleshooting

### Arduino won't connect to WiFi
- ✅ Verify SSID and password are correct
- ✅ Check that WiFi is 2.4GHz (UNO R4 doesn't support 5GHz)
- ✅ Ensure WiFi network allows device connections
- ✅ Check Serial Monitor for error messages

### Arduino can't reach server
- ✅ Verify computer and Arduino are on same network
- ✅ Check SERVER_HOST IP address is correct
- ✅ Ensure server is running (`python3 server.py`)
- ✅ Check firewall isn't blocking port 8080
- ✅ Try pinging the server IP from another device

### Sensor not triggering
- ✅ Check sensor wiring connections (VCC, GND, TRIG pin 8, ECHO pin 9)
- ✅ Uncomment debug line in `checkProximity()` to see distance values
- ✅ Adjust `DISTANCE_THRESHOLD` (default 20 cm) if needed
- ✅ Ensure sensor has clear line of sight (no obstacles)
- ✅ Check sensor power (VCC/GND connections)
- ✅ Verify sensor is HC-SR04 compatible (5V logic)

### Audio not playing
- ✅ Verify `taunt_prompt.wav` exists in the same folder as `server.py`
- ✅ Check computer volume is not muted
- ✅ On Linux, ensure audio player is installed (`aplay`, `paplay`, etc.)
- ✅ Check server console for error messages

### Getting firewall errors
**macOS:**
```bash
# Allow Python through firewall
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /usr/bin/python3
```

**Linux:**
```bash
# Allow port 8080
sudo ufw allow 8080/tcp
```

**Windows:**
- Windows Defender → Allow an app → Add Python

## 📊 Serial Monitor Output

### Successful Connection
```
=== Arduino UNO R4 WiFi - Proximity Trigger ===
Digital sensor initialized on pin 2
WiFi firmware version: 1.0.0

--- Connecting to WiFi ---
SSID: YourWiFi
Attempting connection... SUCCESS!

--- WiFi Status ---
SSID: YourWiFi
IP Address: 192.168.1.150
Signal Strength (RSSI): -45 dBm
-------------------
```

### Successful Trigger
```
>>> PROXIMITY DETECTED! <<<

--- Sending Trigger Request ---
Connecting to 192.168.1.100:8080...
Connected to server!
Request sent!

--- Server Response ---
HTTP/1.0 200 OK
Content-type: text/plain

Trigger received! Playing audio...
-----------------------
Connection closed.
```

## 🎨 Customization Ideas

### Change trigger cooldown
```cpp
const unsigned long COOLDOWN_MS = 5000;  // 5 seconds between triggers
```

### Use different audio file
```python
# In server.py
AUDIO_FILE = "your_audio.wav"
```

### Add multiple endpoints
Modify `server.py` to handle different paths:
```python
if self.path == '/trigger1':
    self.play_audio("sound1.wav")
elif self.path == '/trigger2':
    self.play_audio("sound2.wav")
```

## 📝 Project Structure

```
Software/r4-connect/
├── src/
│   └── main.cpp          # Arduino sketch
├── server.py             # Python HTTP server
├── taunt_prompt.wav      # Audio file to play
├── platformio.ini        # PlatformIO configuration
└── README.md            # This file
```

## 🤝 Contributing

Feel free to modify and improve this code for your needs!

## 📄 License

This project is part of Team 9 Sauceathon 2025 SF.

## 🆘 Support

If you encounter issues:
1. Check the troubleshooting section above
2. Review Serial Monitor output for error messages
3. Verify all configuration settings match your setup
4. Ensure both devices are on the same network

---

**Happy Making! 🎉**
