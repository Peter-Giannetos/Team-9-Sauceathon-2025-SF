# Debugging Guide - Arduino UNO R4 WiFi Proximity Trigger

This guide will help you troubleshoot connection issues between the Arduino and the server.

## 🔍 Monitoring with PlatformIO

### Method 1: Using PlatformIO Serial Monitor (Recommended)

1. **Open PlatformIO Terminal** in VS Code:
   - Click on the PlatformIO icon in the left sidebar
   - Or press `Ctrl+Shift+P` (Windows/Linux) or `Cmd+Shift+P` (Mac)
   - Type "PlatformIO: Serial Monitor" and select it

2. **Or use the command line:**
   ```bash
   cd Software/r4-connect
   pio device monitor
   ```

3. **Set the correct baud rate** (if needed):
   ```bash
   pio device monitor --baud 115200
   ```

### Method 2: Using PlatformIO Upload and Monitor

Upload code and immediately start monitoring:
```bash
cd Software/r4-connect
pio run --target upload && pio device monitor
```

### Method 3: Using VS Code Tasks

1. Press `Ctrl+Shift+P` (Windows/Linux) or `Cmd+Shift+P` (Mac)
2. Type "Tasks: Run Task"
3. Select "PlatformIO: Monitor"

## 📊 Understanding Debug Output

### Successful Connection Flow

```
=== Arduino UNO R4 WiFi - Proximity Trigger ===
Ultrasonic sensor initialized - Trig: Pin 8, Echo: Pin 9
Distance threshold: 20 cm
LED indicator initialized
WiFi firmware version: 1.0.0

--- Connecting to WiFi ---
SSID: YourWiFiName
Attempting connection... SUCCESS!

--- WiFi Status ---
SSID: YourWiFiName
IP Address: 192.168.1.150        ← Note this IP
Signal Strength (RSSI): -45 dBm
-------------------

>>> PROXIMITY DETECTED! <<<

========================================
>>> SENDING TRIGGER REQUEST <<<
========================================
WiFi Status: CONNECTED
Local IP: 192.168.1.150           ← Arduino's IP
Target Server: 192.168.1.100:8080 ← Server IP and port
Endpoint: /trigger

Attempting TCP connection to 192.168.1.100:8080...
✓ TCP connection established!      ← SUCCESS!
Request sent!

--- Server Response ---
HTTP/1.0 200 OK
Content-type: text/plain

Trigger received! Playing audio...
-----------------------
Connection closed.
```

### Failed Connection - Common Issues

#### Issue 1: WiFi Not Connected
```
WiFi Status: NOT CONNECTED!
```
**Solution:** Check WiFi credentials in `main.cpp`

#### Issue 2: Cannot Reach Server
```
Attempting TCP connection to 192.168.1.100:8080...
ERROR: Failed to connect to server!
```
**Possible causes:**
- Server not running
- Wrong IP address
- Firewall blocking connection
- Arduino and computer on different networks

## 🛠️ Troubleshooting Steps

### Step 1: Verify Arduino WiFi Connection

Look for this in the serial output:
```
--- WiFi Status ---
SSID: YourWiFiName
IP Address: 192.168.1.150  ← Arduino got an IP!
```

If you see `Attempting connection... FAILED`, check:
- ✅ WiFi SSID is correct
- ✅ WiFi password is correct
- ✅ WiFi is 2.4GHz (UNO R4 doesn't support 5GHz)

### Step 2: Verify Server is Running

1. **Start the server:**
   ```bash
   cd Software/r4-connect
   python3 server.py
   ```

2. **Note the IP address displayed:**
   ```
   Local IP Address: 192.168.1.100  ← Use this in Arduino code
   ```

3. **Verify server is listening:**
   ```bash
   # On macOS/Linux
   netstat -an | grep 8080
   
   # Should show:
   tcp4  0  0  *.8080  *.*  LISTEN
   ```

### Step 3: Test Network Connectivity

#### From your computer, ping the Arduino:
```bash
ping 192.168.1.150  # Use Arduino's IP from serial monitor
```

#### Test server from browser:
Open in browser: `http://192.168.1.100:8080/trigger`

You should see:
```
Trigger received! Playing audio...
```

### Step 4: Check Firewall Settings

#### macOS:
```bash
# Allow Python through firewall
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /usr/bin/python3
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblockapp /usr/bin/python3
```

#### Linux:
```bash
# Allow port 8080
sudo ufw allow 8080/tcp
sudo ufw status
```

#### Windows:
1. Windows Defender Firewall → Advanced Settings
2. Inbound Rules → New Rule
3. Port → TCP → 8080
4. Allow the connection

### Step 5: Verify Same Network

Both devices must be on the same network:

**Arduino IP:** `192.168.1.150` (from serial monitor)  
**Computer IP:** `192.168.1.100` (from server output)

✅ Both start with `192.168.1.` - Same network!  
❌ Different prefixes - Different networks!

## 🔧 Common Fixes

### Fix 1: Update SERVER_HOST in Arduino Code

In `main.cpp`, update with the IP from the server:
```cpp
const char* SERVER_HOST = "192.168.1.100";  // Use YOUR computer's IP
```

### Fix 2: Restart Everything

1. Stop the server (Ctrl+C)
2. Unplug Arduino
3. Start server: `python3 server.py`
4. Note the IP address
5. Update `SERVER_HOST` in code if needed
6. Upload to Arduino: `pio run --target upload`
7. Monitor: `pio device monitor`

### Fix 3: Enable Distance Debug Output

In `main.cpp`, uncomment these lines in `checkProximity()`:
```cpp
// Serial.print("Distance: ");
// Serial.print(distance);
// Serial.println(" cm");
```

This will show continuous distance readings to verify sensor is working.

## 📝 Debug Checklist

When troubleshooting, verify each item:

- [ ] Arduino connects to WiFi (check serial output)
- [ ] Arduino gets an IP address (note it down)
- [ ] Server is running (`python3 server.py`)
- [ ] Server shows its IP address (note it down)
- [ ] `SERVER_HOST` in Arduino code matches server IP
- [ ] `SERVER_PORT` matches (default: 8080)
- [ ] Both devices on same network (same IP prefix)
- [ ] Firewall allows port 8080
- [ ] Can ping Arduino from computer
- [ ] Can access server from browser
- [ ] Sensor triggers (LED lights up)
- [ ] Serial monitor shows "TCP connection established"

## 🎯 Quick Test Commands

### Test 1: Manual Trigger from Computer
```bash
# Test if server receives requests
curl http://localhost:8080/trigger
```

### Test 2: Check if Port is Open
```bash
# macOS/Linux
nc -zv 192.168.1.100 8080

# Should show: Connection to 192.168.1.100 port 8080 [tcp/*] succeeded!
```

### Test 3: Monitor Network Traffic
```bash
# macOS/Linux - watch for incoming connections
sudo tcpdump -i any port 8080
```

## 💡 Tips

1. **Keep serial monitor open** while testing to see real-time debug output
2. **Use the LED** as a quick indicator - if it lights up, sensor detected proximity
3. **Test server independently** with curl or browser before testing with Arduino
4. **Check IP addresses** - they change if you reconnect to WiFi
5. **Restart both** Arduino and server if you change network settings

## 📞 Still Having Issues?

If the Arduino lights up but server receives no requests:

1. **Verify the debug output shows:**
   ```
   WiFi Status: CONNECTED
   Local IP: [some IP]
   Target Server: [your server IP]:8080
   Attempting TCP connection...
   ```

2. **If it shows "TCP connection established"** but server doesn't respond:
   - Check server console for errors
   - Verify server is listening on all interfaces (0.0.0.0)
   - Try restarting the server

3. **If it shows "Failed to connect to server":**
   - Ping the server IP from another device
   - Check firewall settings
   - Verify both devices are on same network
   - Try using computer's hostname instead of IP

---

**Remember:** The serial monitor is your best friend for debugging! Always check it first.
