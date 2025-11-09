# 🔥 Firewall Fix for macOS

## Quick Diagnosis

If IPs match but connection fails, it's likely macOS firewall blocking Python.

## ✅ Quick Fix (Choose One)

### Option 1: Allow Python Through Firewall (Recommended)

```bash
# Allow Python to accept incoming connections
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /usr/bin/python3
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblockapp /usr/bin/python3
```

### Option 2: Temporarily Disable Firewall (Testing Only)

```bash
# Turn off firewall temporarily
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setglobalstate off

# Test your connection

# Turn it back on when done
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setglobalstate on
```

### Option 3: Use GUI Settings

1. **System Settings** → **Network** → **Firewall**
2. Click **Options**
3. Click **+** button
4. Navigate to `/usr/bin/python3`
5. Click **Add**
6. Ensure it's set to **Allow incoming connections**

## 🧪 Test Server Accessibility

### Test 1: From Same Computer
```bash
# This should work (localhost)
curl http://localhost:8080/trigger
```

### Test 2: From Another Device on Network
```bash
# Replace with your computer's IP
curl http://10.11.1.X:8080/trigger
```

### Test 3: Check if Port is Listening
```bash
# Check if server is listening on all interfaces
lsof -i :8080

# Should show something like:
# Python  12345 user  3u  IPv4  0x... TCP *:8080 (LISTEN)
```

## 🔍 Verify Server is Listening on All Interfaces

The server should bind to `0.0.0.0` (all interfaces), not just `127.0.0.1` (localhost).

Check `server.py` - it should have:
```python
with socketserver.TCPServer(("", PORT), TriggerHandler) as httpd:
```

The empty string `""` means listen on all interfaces. ✅

## 📊 Expected Behavior

### When Working:
```bash
$ curl http://10.11.1.X:8080/trigger
Trigger received! Playing audio...
```

### When Blocked by Firewall:
```bash
$ curl http://10.11.1.X:8080/trigger
curl: (7) Failed to connect to 10.11.1.X port 8080: Connection refused
```

## 🎯 Complete Test Sequence

Run these commands in order:

```bash
# 1. Allow Python through firewall
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --add /usr/bin/python3
sudo /usr/libexec/ApplicationFirewall/socketfilterfw --unblockapp /usr/bin/python3

# 2. Start server
cd Software/r4-connect
python3 server.py

# 3. In another terminal, test locally
curl http://localhost:8080/trigger

# 4. Test from network IP
curl http://$(ipconfig getifaddr en0):8080/trigger

# 5. If both work, Arduino should connect!
```

## 🚨 Still Not Working?

### Check Server Output
When Arduino tries to connect, you should see in server console:
```
[timestamp] "GET /trigger HTTP/1.1" 200 -
```

If you see nothing, the request isn't reaching the server.

### Check Network Isolation
Some WiFi networks have **client isolation** enabled:
```bash
# Try pinging Arduino from computer
ping 10.11.1.29

# If this fails, client isolation is enabled
# Solution: Ask network admin to disable it, or use different network
```

### Alternative: Use mDNS/Bonjour

Instead of IP, try using `.local` hostname:
```cpp
const char* SERVER_HOST = "your-mac-name.local";
```

Find your Mac's name:
```bash
scutil --get LocalHostName
# Then use: hostname.local
