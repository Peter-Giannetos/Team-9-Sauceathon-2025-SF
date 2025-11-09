#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define LED_PIN (2)
#define OUT_PIN (19)
#define PWM_PIN (12)

bool ready = false;

unsigned long previousMillisLED = 0;
unsigned long previousMillisHello = 0;
const long ledInterval = 1000;      // 1 second
const long helloInterval = 10000;   // 10 seconds

bool ledState = LOW;

// PWM configuration
const int pwmFreq = 1000;      // 1 kHz
const int pwmChannel = 0;
const int pwmResolution = 8;   // 8-bit: 0–255
int pwmDutyCycle = 0;

// ESP-NOW message structure
typedef struct struct_message {
  char msg[32];
  int value;
} struct_message;

struct_message incomingData;
struct_message outgoingData; 

// Task handles
TaskHandle_t TaskWiFiHandle = NULL;
TaskHandle_t TaskPrintHandle = NULL;

// Print queue and buffer
QueueHandle_t printQueue;
const int PRINT_BUFFER_SIZE = 128;

// Sends formatted strings to queue (copies data instead of pointer)
void enqueuePrint(const char* fmt, ...) {
  char tempBuf[PRINT_BUFFER_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tempBuf, PRINT_BUFFER_SIZE, fmt, args);
  va_end(args);
  xQueueSend(printQueue, tempBuf, 10 / portTICK_PERIOD_MS);
}

// Print task: reads messages and prints them
void printTask(void* parameter) {
  char localBuffer[PRINT_BUFFER_SIZE];
  while (true) {
    if (xQueueReceive(printQueue, localBuffer, portMAX_DELAY)) {
      Serial.print(localBuffer);
    }
  }
}

// ESP-NOW receive callback
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  enqueuePrint("Received data: %s\n", incomingData.msg);
}

// ESP-NOW send callback
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  enqueuePrint("Last Packet Send Status: %s\n",
               status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ESP-NOW WiFi task
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
  esp_now_peer_info_t peerInfo = {};
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(broadcastAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      enqueuePrint("Failed to add peer\n");
    }
  }

  while (1) {
    strcpy(outgoingData.msg, "Hello ESP-NOW");
    outgoingData.value = millis() / 1000;

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&outgoingData, sizeof(outgoingData));

    if (result == ESP_OK) {
      enqueuePrint("Sent with success\n");
    } else {
      enqueuePrint("Error sending the data\n");
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  Serial.begin(115200);

  // Create a queue with 10 message slots
  printQueue = xQueueCreate(10, PRINT_BUFFER_SIZE);

  if (printQueue == NULL) {
    Serial.println("Failed to create print queue!");
    while (1);
  }

  // Start print task (core 0)
  xTaskCreatePinnedToCore(
    printTask,
    "Print Task",
    4096,
    NULL,
    1,
    &TaskPrintHandle,
    0
  );

  // Setup PWM
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(PWM_PIN, pwmChannel);
  ledcWrite(pwmChannel, pwmDutyCycle);

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

  // Start WiFi task on core 1
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

void loop() {
  unsigned long currentMillis = millis();

  // Blink LED every second
  if (currentMillis - previousMillisLED >= ledInterval) {
    previousMillisLED = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }

  // Print "Hello" every 10 seconds
  if (currentMillis - previousMillisHello >= helloInterval) {
    previousMillisHello = currentMillis;
    enqueuePrint("Hello! Time since boot: %lu ms\n", currentMillis);
  }

  // Command handling
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("ON")) {
      digitalWrite(OUT_PIN, HIGH);
      enqueuePrint("OUT_PIN turned ON\n");
    } else if (cmd.equalsIgnoreCase("OFF")) {
      digitalWrite(OUT_PIN, LOW);
      enqueuePrint("OUT_PIN turned OFF\n");
    } else if (cmd.toInt() >= 0 && cmd.toInt() <= 100) {
      int userValue = cmd.toInt();
      pwmDutyCycle = map(userValue, 0, 100, 0, 255);
      ledcWrite(pwmChannel, pwmDutyCycle);
      enqueuePrint("PWM duty cycle set to %d%%\n", userValue);
    } else {
      enqueuePrint("Unknown command. Use ON, OFF, or a number (0–100).\n");
    }
  }

  vTaskDelay(20 / portTICK_PERIOD_MS);
}
