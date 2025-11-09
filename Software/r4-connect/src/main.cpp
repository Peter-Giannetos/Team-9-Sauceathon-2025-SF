/*
 * Arduino UNO R4 WiFi - Proximity Sensor HTTP Trigger
 * 
 * This sketch monitors a proximity sensor and sends an HTTP GET request
 * to a computer server when close proximity is detected.
 * 
 * Hardware Setup:
 * - Proximity sensor connected to SENSOR_PIN (digital or analog)
 * - Adjust SENSOR_THRESHOLD based on your sensor type
 * 
 * Server Setup:
 * - The computer should run a server listening on SERVER_PORT
 * - When triggered, Arduino sends GET request to SERVER_PATH
 * - Server should play taunt_prompt.wav upon receiving the request
 */

#include "WiFiS3.h"

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

void connectToWiFi();
void printWiFiStatus();
long measureDistance();
bool checkProximity();
void sendTriggerRequest();

// ============================================================================
// CONFIGURATION - CUSTOMIZE THESE VALUES
// ============================================================================

// WiFi Credentials (create arduino_secrets.h or define here)
const char* WIFI_SSID = "impulse_guest";        // TODO: Replace with your WiFi network name
const char* WIFI_PASSWORD = "m0repower!"; // TODO: Replace with your WiFi password

// Server Configuration
const char* SERVER_HOST = "10.11.1.33";  // TODO: Replace with your computer's IP address
const int SERVER_PORT = 8080;                // TODO: Match this with your server's port
const char* SERVER_PATH = "/trigger";        // Endpoint path on the server

// Ultrasonic Sensor Configuration (HC-SR04 or similar)
const int TRIG_PIN = 8;                      // Ultrasonic sensor trigger pin
const int ECHO_PIN = 9;                      // Ultrasonic sensor echo pin
const int DISTANCE_THRESHOLD = 20;           // Distance in cm to trigger (closer than this triggers)

// Timing Configuration
const unsigned long COOLDOWN_MS = 2000;      // Minimum time between triggers (milliseconds)
const unsigned long WIFI_RETRY_DELAY = 10000; // Delay between WiFi connection attempts

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

int wifiStatus = WL_IDLE_STATUS;
unsigned long lastTriggerTime = 0;
bool lastSensorState = false;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect (needed for native USB)
  }
  
  Serial.println("=== Arduino UNO R4 WiFi - Proximity Trigger ===");
  
  // Initialize ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.print("Ultrasonic sensor initialized - Trig: Pin ");
  Serial.print(TRIG_PIN);
  Serial.print(", Echo: Pin ");
  Serial.println(ECHO_PIN);
  Serial.print("Distance threshold: ");
  Serial.print(DISTANCE_THRESHOLD);
  Serial.println(" cm");
  
  // Initialize built-in LED for visual feedback
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("LED indicator initialized");
  
  // Check for WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ERROR: Communication with WiFi module failed!");
    Serial.println("Please check your board and connections.");
    while (true); // Halt execution
  }
  
  // Check firmware version
  String firmwareVersion = WiFi.firmwareVersion();
  Serial.print("WiFi firmware version: ");
  Serial.println(firmwareVersion);
  
  // Connect to WiFi
  connectToWiFi();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectToWiFi();
  }
  
  // Read sensor value
  bool proximityDetected = checkProximity();
  
  // Trigger on proximity detection (with debounce)
  if (proximityDetected && !lastSensorState) {
    unsigned long currentTime = millis();
    
    // Check if cooldown period has passed
    if (currentTime - lastTriggerTime >= COOLDOWN_MS) {
      Serial.println("\n>>> PROXIMITY DETECTED! <<<");
      sendTriggerRequest();
      lastTriggerTime = currentTime;
    } else {
      Serial.println("Trigger ignored (cooldown active)");
    }
  }
  
  lastSensorState = proximityDetected;
  
  // Small delay to prevent excessive polling
  delay(50);
}

// ============================================================================
// WIFI FUNCTIONS
// ============================================================================

void connectToWiFi() {
  Serial.println("\n--- Connecting to WiFi ---");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  
  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Attempting connection...");
    wifiStatus = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    if (wifiStatus == WL_CONNECTED) {
      Serial.println(" SUCCESS!");
      printWiFiStatus();
    } else {
      Serial.println(" FAILED");
      Serial.print("Retrying in ");
      Serial.print(WIFI_RETRY_DELAY / 1000);
      Serial.println(" seconds...");
      delay(WIFI_RETRY_DELAY);
    }
  }
}

void printWiFiStatus() {
  Serial.println("\n--- WiFi Status ---");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  long rssi = WiFi.RSSI();
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.println("-------------------\n");
}

// ============================================================================
// SENSOR FUNCTIONS
// ============================================================================

long measureDistance() {
  // Send ultrasonic pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read echo pulse duration
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  
  // Calculate distance in cm (speed of sound = 343 m/s)
  // Distance = (duration / 2) / 29.1
  long distance = duration / 58;
  
  return distance;
}

bool checkProximity() {
  long distance = measureDistance();
  
  // Debug output (uncomment to see distance values)
  // Serial.print("Distance: ");
  // Serial.print(distance);
  // Serial.println(" cm");
  
  // Return true if object is closer than threshold
  // Also check for valid reading (0 means timeout/no echo)
  if (distance > 0 && distance < DISTANCE_THRESHOLD) {
    return true;
  }
  
  return false;
}

// ============================================================================
// HTTP REQUEST FUNCTIONS
// ============================================================================

void sendTriggerRequest() {
  WiFiClient client;
  
  // Flash LED to indicate trigger
  digitalWrite(LED_BUILTIN, HIGH);
  
  Serial.println("\n========================================");
  Serial.println(">>> SENDING TRIGGER REQUEST <<<");
  Serial.println("========================================");
  
  // Debug: Print current WiFi status
  Serial.print("WiFi Status: ");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("CONNECTED");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("NOT CONNECTED!");
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }
  
  // Debug: Print connection details
  Serial.print("Target Server: ");
  Serial.print(SERVER_HOST);
  Serial.print(":");
  Serial.println(SERVER_PORT);
  Serial.print("Endpoint: ");
  Serial.println(SERVER_PATH);
  Serial.println();
  
  Serial.print("Attempting TCP connection to ");
  Serial.print(SERVER_HOST);
  Serial.print(":");
  Serial.print(SERVER_PORT);
  Serial.println("...");
  
  // Attempt to connect to server
  if (client.connect(SERVER_HOST, SERVER_PORT)) {
    Serial.println("✓ TCP connection established!");
    
    // Send HTTP GET request
    client.print("GET ");
    client.print(SERVER_PATH);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(SERVER_HOST);
    client.println("Connection: close");
    client.println("User-Agent: Arduino-UNO-R4-WiFi");
    client.println();
    
    Serial.println("Request sent!");
    
    // Wait for response (with timeout)
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Timeout waiting for response!");
        client.stop();
        return;
      }
    }
    
    // Read and print response
    Serial.println("\n--- Server Response ---");
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    Serial.println("\n-----------------------");
    
    // Close connection
    client.stop();
    Serial.println("Connection closed.\n");
    
    // Flicker LED to indicate success (3 quick blinks)
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
    }
    digitalWrite(LED_BUILTIN, LOW);
    
  } else {
    Serial.println("ERROR: Failed to connect to server!");
    Serial.println("Check that:");
    Serial.println("  1. Server is running on the computer");
    Serial.println("  2. SERVER_HOST IP address is correct");
    Serial.println("  3. SERVER_PORT matches the server configuration");
    Serial.println("  4. Firewall allows incoming connections\n");
    
    // Turn off LED on error
    digitalWrite(LED_BUILTIN, LOW);
  }
}
