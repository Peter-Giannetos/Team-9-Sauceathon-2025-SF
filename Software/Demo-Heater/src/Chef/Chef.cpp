#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>


//===================================================================================================
// Pin Definitions

#define LED_PIN         (2)
#define PWM_PIN         (12)
#define NEOPIXEL_PIN    (14)
#define NUMPIXELS       (16)
#define SOUND_PIN       (34)

#define PERIOD_LED_ERROR  (200)
#define PERIOD_LED_GOOD   (1000)

#define PWM_DEFAULT_FREQ          (1000)    // 1kHz
#define PWM_DEFAULT_DUTY          (0)       // 0%
#define PWM_DEFAULT_RESOLUTION    (8)       // 8-bit: 0-255
#define PWM_DEFAULT_CHANNEL       (0)

#define SERIAL_RATE (115200)
#define SERIAL_DEBUG                        // Define to enable serial debugging


//===================================================================================================
// Initialization Variables

bool ready = false;

unsigned long previousMillisLED = 0;
unsigned long previousMillisHello = 0;
unsigned long previousMillisSound = 0;

long ledInterval = PERIOD_LED_GOOD;      // 1 second
long helloInterval = 10000;   // 10 seconds
const long soundInterval = 20;  // More frequent updates for faster reaction


bool ledState = LOW;
bool audioMode = false;  // Default: Manual-based PWM updates

Adafruit_NeoPixel strip(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// PWM configuration
int pwmDutyCycle = PWM_DEFAULT_DUTY;

// ESP-NOW message structure
typedef struct struct_message {
  char msg[32];
  int value;
} struct_message;

struct_message incomingData;
struct_message outgoingData; 
char outgoingMsg[32] = "Hello ESP-NOW";  // Default message - Modify with new messages

/**
 * @brief WIFI MESSAGE PROTOCOL
 * 
 * 1. Edit outgoingMsg with new char
 * 2. Send wifi
 * 3. Keep reboradcasting message until ready for next state (Incase packet was not received)
 * 4. Read message and react once until message from device changes
 * 
 */


// Task handles
TaskHandle_t TaskWiFiHandle = NULL;
TaskHandle_t TaskPrintHandle = NULL;

// Print queue and buffer
QueueHandle_t printQueue;
#define PRINT_BUFFER_SIZE   (128)
#define PRINT_BUFFER_COUNT  (10)

//===================================================================================================
// Function Definitions


// FUNCTION: Color Interpolate
uint32_t interpolateColor(uint8_t r1, uint8_t g1, uint8_t b1,
                          uint8_t r2, uint8_t g2, uint8_t b2,
                          float t) {
  uint8_t r = r1 + (r2 - r1) * t;
  uint8_t g = g1 + (g2 - g1) * t;
  uint8_t b = b1 + (b2 - b1) * t;
  return strip.Color(r, g, b);
}

// FUNCTION: Brightness & Color Gradient
void displayEnhancedBrightnessGradient(int step) {
  float progress = (float)step / (NUMPIXELS - 1);
  float globalBrightness = pow(progress, 2.2) * 0.9 + 0.1;

  for (int i = 0; i < NUMPIXELS; i++) {
    float ratio = (float)i / (NUMPIXELS - 1);
    uint32_t color;

    if (ratio < 0.15) {
      float t = ratio / 0.15;
      color = interpolateColor(0, 255, 0, 255, 255, 0, t);
    } else if (ratio < 0.35) {
      float t = (ratio - 0.15) / 0.20;
      color = interpolateColor(255, 255, 0, 255, 120, 0, t);
    } else {
      float t = (ratio - 0.35) / 0.65;
      color = interpolateColor(255, 120, 0, 255, 0, 0, t);
    }

    uint8_t r = ((color >> 16) & 0xFF) * globalBrightness;
    uint8_t g = ((color >> 8) & 0xFF) * globalBrightness;
    uint8_t b = (color & 0xFF) * globalBrightness;

    if (i <= step)
      strip.setPixelColor(i, r, g, b);
    else
      strip.setPixelColor(i, 0, 0, 0);
  }

  strip.show();
}



// FUNCTION: Sends strings to global queue
void enqueuePrint(const char* fmt, ...) {
  char tempBuf[PRINT_BUFFER_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tempBuf, PRINT_BUFFER_SIZE, fmt, args);
  va_end(args);
  xQueueSend(printQueue, tempBuf, 10 / portTICK_PERIOD_MS);
}

// FUNCTION: Prints strings from global queue
void printTask(void* parameter) {
  char localBuffer[PRINT_BUFFER_SIZE];
  while (true) {
    if (xQueueReceive(printQueue, localBuffer, portMAX_DELAY)) {
      Serial.print(localBuffer);
    }
  }
}

// FUNCTION: ESP-NOW Read Message
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingDataPtr, int len) {

  // 1. Fetch message
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  enqueuePrint("Received data: %s\n", incomingData.msg);

  // 2. Fetch device MAC
  char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    // enqueuePrint("Received from MAC: %s\n", macStr);
    memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
    // enqueuePrint("Received data: %s\n", incomingData.msg);

  // 3. Process message
  if (strcmp(incomingData.msg, "CHANGE_ME") == 0) {
    // Function Call 1
  }
  else if (strcmp(incomingData.msg, "CHANGE_ME") == 0) {
    // Function Call 2
  }
  else if (strcmp(incomingData.msg, "CHANGE_ME") == 0) {
    // Function Call 3
  }
}

// FUNCTION: ESP-NOW Send Message
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  // enqueuePrint("Last Packet Send Status: %s\n",
  //              status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// FUNCTION: ESP-NOW WiFi task
void wifiTask(void* parameter) {
  enqueuePrint("WiFi Task started on core: %d\n", xPortGetCoreID());

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    enqueuePrint("Error initializing ESP-NOW\n");
    vTaskDelete(NULL);
  }

  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  // Add broadcast peer
  esp_now_peer_info_t peerInfo = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // Listen to everyone
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};    // Broadcast to everyone
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(broadcastAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      enqueuePrint("Failed to add peer\n");
    }
  }

  while (1) {
    strcpy(outgoingData.msg, outgoingMsg);
    outgoingData.value = millis() / 1000;

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&outgoingData, sizeof(outgoingData));

    if (result == ESP_OK) {
      enqueuePrint("Sent with success\n");
    } else {
      enqueuePrint("Error sending the data\n");
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);    // Sleep 500 mS
  }
}

//===================================================================================================
// Setup Function

void setup() {

  // 1. Init LED pin
  pinMode(LED_PIN, OUTPUT);
  pinMode(SOUND_PIN, INPUT);

  // 2. Pin setup PWM
  ledcSetup(PWM_DEFAULT_CHANNEL, PWM_DEFAULT_FREQ, PWM_DEFAULT_RESOLUTION);
  ledcAttachPin(PWM_PIN, PWM_DEFAULT_CHANNEL);
  ledcWrite(PWM_DEFAULT_CHANNEL, PWM_DEFAULT_DUTY);

  // 3. Begin serial
  Serial.begin(SERIAL_RATE);

  // 4. Create shared core print queue
  printQueue = xQueueCreate(PRINT_BUFFER_COUNT, PRINT_BUFFER_SIZE);
  if (printQueue == NULL) {
    Serial.println("Failed to create print queue!");
    ledInterval = PERIOD_LED_ERROR;
  }

  // 5. Pin shared print task (core 0)
  xTaskCreatePinnedToCore(
    printTask,
    "Print Task",
    4096,
    NULL,
    1,
    &TaskPrintHandle,
    0
  );

  // 6. Hold until "GO" inputed by user serial
  enqueuePrint("Type 'GO' then press Enter to start:\n");
  String input;
  while (!ready) {
    if (Serial.available()) {
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.equalsIgnoreCase("GO")) {
        ready = true;
        enqueuePrint("Starting main loop...\n");
        enqueuePrint("You can enter 'ON', 'OFF', or a PWM value (0–100).\n");
      } else {
        enqueuePrint("Waiting for 'GO'...\n");
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  // 7. Pin WIFI task on core 1
  xTaskCreatePinnedToCore(
    wifiTask,
    "WiFi Task",
    10000,
    NULL,
    1,
    &TaskWiFiHandle,
    1
  );
  enqueuePrint("MAC Address: %s\n", WiFi.macAddress().c_str());

  // 8. Start Neopixels
  strip.begin();
  strip.show();

}

//===================================================================================================
// Loop Function (Core 0)

void loop() {

  // 1. Get fake RTOS scheduler
  unsigned long currentMillis = millis();

  // 2. Blink status LED (1 Second)
  if (currentMillis - previousMillisLED >= ledInterval) {
    previousMillisLED = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }

  // 3. Serial print status "Hello" (10 Seconds)
  if (currentMillis - previousMillisHello >= helloInterval) {
    previousMillisHello = currentMillis;
    enqueuePrint("Hello! Time since boot: %lu ms\n", currentMillis);
  }

  // 4. Update Lights via Sound
  if (currentMillis - previousMillisSound >= soundInterval) {
    previousMillisSound = currentMillis;

    int soundValue = analogRead(SOUND_PIN);

    // Faster smoothing for spikes
    static float smoothValue = 0;
    float alpha = 0.15;
    smoothValue = smoothValue * (1 - alpha) + soundValue * alpha;

    // Envelope detector with dynamic decay based on activity
    static float level = 0;
    float decayRate = 0.92; // base decay (faster when quiet)

    if (smoothValue > 1500) {
      decayRate = 0.97; // slow decay for loud sounds
    } else if (smoothValue > 800) {
      decayRate = 0.95; // medium decay
    } // else keep base decay

    if (smoothValue > level) {
      // Fast rise for responsiveness
      level = smoothValue + (smoothValue - level) * 0.5;
    } else {
      // Dynamic decay
      level *= decayRate;
    }

    if (level < 70) level = 70;

    float amplified = (level - 500) * 3.2;
    int amplifiedValue = constrain(amplified, 0, 4095);
    int step = map(amplifiedValue, 0, 4095, 0, NUMPIXELS - 1);
    step = constrain(step, 0, NUMPIXELS - 1);

    displayEnhancedBrightnessGradient(step);

    // Map sound level to PWM duty cycle (0–255)
    int pwmGain = 3;
    int pwmValue = map(amplifiedValue * 3.2, 0, 4095, 0, 255);
    pwmValue = constrain(pwmValue, 0, 255);

    // Smooth PWM response for stability
    static int smoothPWM = 0;
    float pwmAlpha = 0.1; // lower values make it smoother
    smoothPWM = smoothPWM * (1 - pwmAlpha) + pwmValue * pwmAlpha;

    // Write PWM signal based on sound intensity
    if(audioMode)
    {
      ledcWrite(PWM_DEFAULT_CHANNEL, (int)smoothPWM);
    }
  }

  // 5. Parse serial input
  if (Serial.available()) {
    
    // 4.1 Read serial
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // 4.2 Decode PWM Mode
    if (cmd.equalsIgnoreCase("A")) {
      audioMode = true;
      enqueuePrint("Switched to AUDIO mode (PWM follows sound input).\n");
    } else if (cmd.equalsIgnoreCase("M")) {
      audioMode = false;
      enqueuePrint("Switched to MANUAL mode (PWM set via serial).\n");
    } else if (!audioMode && cmd.toInt() >= 0 && cmd.toInt() <= 100) {
      int userValue = cmd.toInt();
      pwmDutyCycle = map(userValue, 0, 100, 0, 255);
      ledcWrite(PWM_DEFAULT_CHANNEL, pwmDutyCycle);
      enqueuePrint("Manual PWM set to %d%%\n", userValue);
    }

    
    // 4.4 Update ESP-NOW message if not a PWM number //TODO COPY THIS FORMAT TO SEND MESSAGES
    else if (cmd.length() < sizeof(outgoingMsg)) {
      cmd.toCharArray(outgoingMsg, sizeof(outgoingMsg));
      enqueuePrint("Updated message to send: %s\n", outgoingMsg);
    }
    
    // 4.5 Unknown command error
    else {
      enqueuePrint("Unknown command or message too long. Use PWM (0–100) or shorter text message.\n");
    }
  }

  // 6. Yield to other tasks
  vTaskDelay(20 / portTICK_PERIOD_MS);
}
