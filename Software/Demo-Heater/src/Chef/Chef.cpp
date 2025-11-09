#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

//===================================================================================================
// Pin Definitions

#define LED_PIN (2)
#define PWM_PIN (12)

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
long ledInterval = PERIOD_LED_GOOD;      // 1 second
long helloInterval = 10000;   // 10 seconds

bool ledState = LOW;

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
    enqueuePrint("Received from MAC: %s\n", macStr);
    memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
    enqueuePrint("Received data: %s\n", incomingData.msg);

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
  enqueuePrint("Last Packet Send Status: %s\n",
               status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
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

  // 4. Parse serial input
  if (Serial.available()) {
    
    // 4.1 Read serial
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // 4.2 Decode PWM
    if (cmd.toInt() >= 0 && cmd.toInt() <= 100) {
      int userValue = cmd.toInt();
      pwmDutyCycle = map(userValue, 0, 100, 0, 255);
      ledcWrite(PWM_DEFAULT_CHANNEL, pwmDutyCycle);
      enqueuePrint("PWM duty cycle set to %d%%\n", userValue);
    }
    
    // 4.3 Update ESP-NOW message if not a PWM number //TODO COPY THIS FORMAT TO SEND MESSAGES
    else if (cmd.length() < sizeof(outgoingMsg)) {
      cmd.toCharArray(outgoingMsg, sizeof(outgoingMsg));
      enqueuePrint("Updated message to send: %s\n", outgoingMsg);
    }
    
    // 4.4 Unknown command error
    else {
      enqueuePrint("Unknown command or message too long. Use PWM (0–100) or shorter text message.\n");
    }
  }

  // 5. Yield to other tasks
  vTaskDelay(20 / portTICK_PERIOD_MS);
}
