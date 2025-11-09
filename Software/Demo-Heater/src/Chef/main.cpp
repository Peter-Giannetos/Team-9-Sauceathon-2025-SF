#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Adafruit_NeoPixel.h>
// #include "tower_pro_motor.h"


//===================================================================================================
// Pin Definitions

#define LED_PIN         (2)
#define PWM_PIN         (12)
#define NEOPIXEL_PIN    (14)
#define NUMPIXELS       (16)
#define SOUND_PIN       (34)
// #define MOTOR_PIN_GATE  (27)

#define PERIOD_LED_ERROR  (200)
#define PERIOD_LED_GOOD   (1000)

#define PWM_DEFAULT_FREQ          (1000)    // 1kHz
#define PWM_DEFAULT_DUTY          (0)       // 0%
#define PWM_DEFAULT_RESOLUTION    (8)       // 8-bit: 0-255
#define PWM_DEFAULT_CHANNEL       (0)

#define SERIAL_RATE (115200)
#define SERIAL_DEBUG                        // Define to enable serial debugging





//===================================================================================================
// Hardware Interrupt

#define INTERRUPT_PIN (26)
volatile bool interruptTriggered = false;
volatile unsigned long lastInterruptTime = 0;
const unsigned long interruptDebounce = 100; // 100 ms minimum gap between interrupts

#define MIN_PRESS_TIME 200  // Minimum press duration in ms to register (adjust as needed)

volatile unsigned long pressStartTime = 0;
volatile bool pressRegistered = false;
volatile bool pinIsPressed = false;


// ISR function
void IRAM_ATTR handleInterrupt() {
  unsigned long currentTime = millis();

  // Simple debounce check
  if ((currentTime - lastInterruptTime) < interruptDebounce) {
    return;
  }
  lastInterruptTime = currentTime;

  int pinState = digitalRead(INTERRUPT_PIN);

  if (pinState == LOW) {
    // Button press started
    pressStartTime = currentTime;
    pinIsPressed = true;
  } else {
    // Button released
    pinIsPressed = false;
    unsigned long pressDuration = currentTime - pressStartTime;
    if (pressDuration >= MIN_PRESS_TIME) {
      pressRegistered = true;
    }
  }
}



//===================================================================================================
// Finite State Machine

enum state {
    STATE_B_DETECT_BUTTON,  // 1. Go to STATE_B_DROP 
    STATE_B_DROP,           // 1. Reset gate (butter/toast), Reset flipper (Open Top) | 2. Open bottom dropper | 3. Wait
    STATE_B_BUTTER,         // 1. Apply butter | 2. Open gate butter | 3. Wait
    STATE_B_TOAST,          // 1. Measure sound & heat | 2. Brand | 3. open gate toast | 4. Wait
    STATE_B_DISPENSE,       // 1. Dispense pusher | 2. Wait
    STATE_T_DETECT_BUTTON,  // 1. Go to STATE_T_DROP 
    STATE_T_DROP,           // 1. Reset gate (butter/toast), Reset flipper (Close Top) | 2. Open top dropper | 3. Wait
    STATE_T_BUTTER,         // 1. Apply butter | 2. Open gate butter | 3. Wait
    STATE_T_TOAST,          // 1. Measure sound & heat | 2. Brand | 3. open gate toast | 4. Wait
    STATE_T_DISPENSE,       // 1. Dispense flipper | 2. Wait
};

state fsm = STATE_B_DETECT_BUTTON;

#define DELAY_B_DROP_WAIT            5000   // STATE_B_DROP: Wait after opening bottom dropper
#define DELAY_B_BUTTER_WAIT          4000   // STATE_B_BUTTER: Wait after opening butter gate
#define DELAY_B_TOAST_WAIT           7000   // STATE_B_TOAST: Wait after opening toast gate
#define DELAY_B_DISPENSE_WAIT        3000   // STATE_B_DISPENSE: Wait after dispensing pusher

#define DELAY_T_DROP_WAIT            5000   // STATE_T_DROP: Wait after opening top dropper
#define DELAY_T_BUTTER_WAIT          4000   // STATE_T_BUTTER: Wait after opening butter gate
#define DELAY_T_TOAST_WAIT           7000   // STATE_T_TOAST: Wait after opening toast gate
#define DELAY_T_DISPENSE_WAIT        3000   // STATE_T_DISPENSE: Wait after dispensing flipper


// Board Specific Values
#define DELAY_STATE_TOAST  (4000)
// servo calibration

//===================================================================================================
// Initialization Variables

bool ready = false;

unsigned long previousMillisLED = 0;
unsigned long previousMillisHello = 0;
unsigned long previousMillisSound = 0;

long ledInterval = PERIOD_LED_GOOD;      // 1 second
long helloInterval = 10000;   // 10 seconds
const long soundInterval = 10;  // More frequent updates for faster reaction


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
      // enqueuePrint("Sent with success\n");
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
  // pinMode(MOTOR_PIN_GATE, OUTPUT);

  // 1.1 Attach hardware interrupt
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);  // Expecting a LOW signal to trigger
attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, CHANGE);

  // 1.2 Init Servo
  // towerProInit(MOTOR_PIN_GATE);

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

  // 4. Update Lights via Sound - Gauge that builds up as user yells
  if (currentMillis - previousMillisSound >= soundInterval) {
    previousMillisSound = currentMillis;

    int soundValue = analogRead(SOUND_PIN);

    // Smooth the raw sound input
    static float smoothValue = 0;
    float alpha = 0.4;
    smoothValue = smoothValue * (1 - alpha) + soundValue * alpha;

    // Gauge system: accumulates when sound is detected, drains when quiet
    static float gaugeLevel = 0.0;  // Gauge level from 0.0 to 1.0
    const float maxGaugeLevel = 1.0;
    const float minGaugeLevel = 0.0;
    
    // Thresholds for detecting sound activity (very sensitive for lower sounds)
    const int quietThreshold = 300;   // Below this is considered quiet
    const int activeThreshold = 400;  // Above this actively charges the gauge
    const int loudThreshold = 600;   // Very loud sounds charge faster
    
    // Charging and draining rates (much faster for quick response)
    const float chargeRateSlow = 0.03;   // Slow charge for moderate sounds
    const float chargeRateFast = 0.06;  // Fast charge for loud sounds
    const float drainRate = 0.002;      // Rate at which gauge drains when quiet (faster decay)
    
    // Determine if sound is active and how intense
    bool isQuiet = smoothValue < quietThreshold;
    bool isActive = smoothValue >= activeThreshold;
    bool isLoud = smoothValue >= loudThreshold;
    
    // Update gauge level based on sound activity
    if (isQuiet) {
      // Drain the gauge when quiet
      gaugeLevel = max(minGaugeLevel, gaugeLevel - drainRate);
    } else if (isLoud) {
      // Fast charge for loud sounds
      gaugeLevel = min(maxGaugeLevel, gaugeLevel + chargeRateFast);
    } else if (isActive) {
      // Slow charge for moderate sounds
      gaugeLevel = min(maxGaugeLevel, gaugeLevel + chargeRateSlow);
    }
    // If between quiet and active, charge slowly (even lower sounds help)
    else {
      gaugeLevel = min(maxGaugeLevel, gaugeLevel + chargeRateSlow * 0.5f);
    }
    
    // Map gauge level (0.0 to 1.0) to LED steps (0 to NUMPIXELS - 1)
    int step = (int)(gaugeLevel * (NUMPIXELS - 1));
    step = constrain(step, 0, NUMPIXELS - 1);


    displayEnhancedBrightnessGradient(step);

    // Map gauge level to PWM duty cycle (0–255)
    const int PWM_MAX_60 = 153; // 60% of 255
    int pwmValue = (int)(gaugeLevel * PWM_MAX_60);
    pwmValue = constrain(pwmValue, 0, PWM_MAX_60);

    // Smooth PWM response for stability
    static int smoothPWM = 0;
    float pwmAlpha = 0.1; // lower values make it smoother
    smoothPWM = smoothPWM * (1 - pwmAlpha) + pwmValue * pwmAlpha;

    // Write PWM signal based on sound intensity
    if(audioMode && fsm == DELAY_B_TOAST_WAIT || fsm == DELAY_T_TOAST_WAIT)
    {
      ledcWrite(PWM_DEFAULT_CHANNEL, (int)smoothPWM);
    }
    else if (audioMode) {
      ledcWrite(PWM_DEFAULT_CHANNEL, (int)0); // Turn off othersie
    }
  }

  // 5. Service interrupt
if (pressRegistered) {
  pressRegistered = false;
  enqueuePrint("Press registered after min time at %lu ms\n", millis());

  if(fsm == STATE_T_DETECT_BUTTON)
  {
    fsm = STATE_T_DROP; // Begin 2nd FSM sequence
  }
  else
  {
    fsm = STATE_B_DROP; // Begin FSM sequence
  }
}

    // 5. FSM State Machine Execution
  static unsigned long fsmStartTime = 0;
  static bool fsmActive = false;
  static unsigned long lastBroadcastTime = 0;
  const unsigned long broadcastInterval = 500; // ms between state broadcasts

  // Start FSM cycle if just triggered
  if (!fsmActive && fsm != STATE_B_DETECT_BUTTON) {
    fsmActive = true;
    fsmStartTime = currentMillis;
    // enqueuePrint("FSM entered state %d at %lu ms\n", fsm, currentMillis);
  }

  // Regular FSM loop
  if (fsmActive) {
    unsigned long elapsed = currentMillis - fsmStartTime;
    unsigned long waitTime = 0;
    state nextState = fsm; // Default: no change

    // Determine wait time and next state
    switch (fsm) {

      // Buttom Bun
      case STATE_B_DROP:
        waitTime = DELAY_B_DROP_WAIT;
        nextState = STATE_B_BUTTER;
        break;
      case STATE_B_BUTTER:
        waitTime = DELAY_B_BUTTER_WAIT;
        nextState = STATE_B_TOAST;
        break;
      case STATE_B_TOAST:
        waitTime = DELAY_B_TOAST_WAIT;
        nextState = STATE_B_DISPENSE;
        break;
      case STATE_B_DISPENSE:
        waitTime = DELAY_B_DISPENSE_WAIT;
        nextState = STATE_T_DETECT_BUTTON;
        break;

      // Top Bun
      case STATE_T_DROP:
        waitTime = DELAY_T_DROP_WAIT;
        nextState = STATE_T_BUTTER;
        break;
      case STATE_T_BUTTER:
        waitTime = DELAY_T_BUTTER_WAIT;
        nextState = STATE_T_TOAST;
        break;
      case STATE_T_TOAST:
        waitTime = DELAY_T_TOAST_WAIT;
        nextState = STATE_T_DISPENSE;
        break;
      case STATE_T_DISPENSE:
        waitTime = DELAY_T_DISPENSE_WAIT;
        nextState = STATE_B_DETECT_BUTTON;
        break;

      default:
        fsmActive = false;
        break;
    }

    // Transition when delay passes
    if (fsmActive && elapsed >= waitTime && waitTime > 0) {
      fsm = nextState;
      fsmStartTime = currentMillis;
      enqueuePrint("FSM transitioned to state %d at %lu ms\n", fsm, currentMillis);
    }

    // Always broadcast current FSM state at a fixed interval
    if (currentMillis - lastBroadcastTime >= broadcastInterval) {
      lastBroadcastTime = currentMillis;
      snprintf(outgoingMsg, sizeof(outgoingMsg), "FSM STATE %d", fsm);
      strcpy(outgoingData.msg, outgoingMsg);
      esp_now_send(NULL, (uint8_t*)&outgoingData, sizeof(outgoingData));
    }
  }

  // // 6. Run Toast Arm
  // if ((fsm == STATE_B_TOAST || fsm == STATE_T_TOAST) &&
  //     (millis() - fsmStartTime >= DELAY_STATE_TOAST)) {
  //       enqueuePrint("DRIVE MOTOR");
  //       towerProBounce(3000, 3000);
  // }


  // 6. Parse serial input
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
