#!/usr/bin/env python3
"""
Simple HTTP Server for Arduino Proximity Trigger
Listens for GET requests from Arduino UNO R4 WiFi and plays taunt_prompt.wav
"""

import http.server
import socketserver
import os
import sys
import platform
import subprocess
from pathlib import Path

# Configuration
PORT = 8080
AUDIO_FILE = "taunt_prompt.wav"

class TriggerHandler(http.server.BaseHTTPRequestHandler):
    """Custom HTTP request handler that plays audio on trigger"""
    
    def do_GET(self):
        """Handle GET requests"""
        if self.path == '/trigger':
            print("\n" + "="*50)
            print("🚨 TRIGGER RECEIVED FROM ARDUINO!")
            print("="*50)
            
            # Play the audio file
            self.play_audio()
            
            # Send success response
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b'Trigger received! Playing audio...\n')
            
            print("✅ Response sent to Arduino\n")
        else:
            # Handle other paths
            self.send_response(404)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write(b'Not Found. Use /trigger endpoint.\n')
    
    def play_audio(self):
        """Play the audio file using system-appropriate command"""
        audio_path = Path(__file__).parent / AUDIO_FILE
        
        if not audio_path.exists():
            print(f"❌ ERROR: Audio file not found: {audio_path}")
            return
        
        try:
            system = platform.system()
            
            if system == "Darwin":  # macOS
                print(f"🔊 Playing audio on macOS: {AUDIO_FILE}")
                subprocess.Popen(['afplay', str(audio_path)])
                
            elif system == "Linux":
                print(f"🔊 Playing audio on Linux: {AUDIO_FILE}")
                # Try multiple players in order of preference
                players = ['aplay', 'paplay', 'ffplay', 'mpg123']
                for player in players:
                    try:
                        subprocess.Popen([player, str(audio_path)])
                        break
                    except FileNotFoundError:
                        continue
                        
            elif system == "Windows":
                print(f"🔊 Playing audio on Windows: {AUDIO_FILE}")
                import winsound
                winsound.PlaySound(str(audio_path), winsound.SND_FILENAME | winsound.SND_ASYNC)
                
            else:
                print(f"⚠️  Unknown system: {system}. Cannot play audio.")
                
        except Exception as e:
            print(f"❌ Error playing audio: {e}")
    
    def log_message(self, format, *args):
        """Custom log format"""
        print(f"[{self.log_date_time_string()}] {format % args}")


def get_local_ip():
    """Get the local IP address of this computer"""
    import socket
    try:
        # Create a socket to determine local IP
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        local_ip = s.getsockname()[0]
        s.close()
        return local_ip
    except Exception:
        return "Unable to determine"


def main():
    """Start the HTTP server"""
    # Check if audio file exists
    audio_path = Path(__file__).parent / AUDIO_FILE
    if not audio_path.exists():
        print(f"⚠️  WARNING: Audio file not found: {audio_path}")
        print("The server will start, but audio playback will fail.")
        print()
    
    # Get local IP
    local_ip = get_local_ip()
    
    # Create server - bind to all interfaces (0.0.0.0)
    # This allows connections from the local network, not just localhost
    with socketserver.TCPServer(("0.0.0.0", PORT), TriggerHandler) as httpd:
        print("="*60)
        print("🎯 Arduino Proximity Trigger Server")
        print("="*60)
        print(f"Server running on port {PORT}")
        print(f"Binding to: 0.0.0.0 (all network interfaces)")
        print(f"Local IP Address: {local_ip}")
        print(f"Audio file: {AUDIO_FILE}")
        print()
        print("📝 Arduino Configuration:")
        print(f"   SERVER_HOST = \"{local_ip}\"")
        print(f"   SERVER_PORT = {PORT}")
        print(f"   SERVER_PATH = \"/trigger\"")
        print()
        print("🧪 Test server accessibility:")
        print(f"   Local:   curl http://localhost:{PORT}/trigger")
        print(f"   Network: curl http://{local_ip}:{PORT}/trigger")
        print()
        print("Waiting for trigger requests from Arduino...")
        print("Press Ctrl+C to stop the server")
        print("="*60)
        print()
        
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n\n🛑 Server stopped by user")
            sys.exit(0)


if __name__ == "__main__":
    main()
