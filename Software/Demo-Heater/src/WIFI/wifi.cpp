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

const int pwmFreq = 1000;    // 1 kHz
const int pwmChannel = 0;
const int pwmResolution = 8; // 8-bit: 0-255
int pwmDutyCycle = 0;      // default 50%

// Example data structure for ESP-NOW
typedef struct struct_message {
  char msg[32];
  int value;
} struct_message;

struct_message incomingData;
struct_message outgoingData;

// Task handle for WiFi/ESP-NOW task
TaskHandle_t TaskWiFiHandle = NULL;

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingData, incomingData, sizeof(incomingData));
  Serial.print("Received data: ");
  Serial.println(incomingData.msg);
  // You can add custom processing here
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void wifiTask(void * parameter) {
  Serial.println("WiFi Task started on core: " + String(xPortGetCoreID()));

  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    vTaskDelete(NULL);
  }

  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  // Add a peer example — change MAC address to your receiver MAC
  esp_now_peer_info_t peerInfo = {};
  uint8_t broadcastAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(broadcastAddress)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("Failed to add peer");
    }
  }

  while (1) {
    // Example sending every 5 seconds
    strcpy(outgoingData.msg, "Hello ESP-NOW");
    outgoingData.value = millis() / 1000;

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));

    if (result == ESP_OK) {
      Serial.println("Sent with success");
    } else {
      Serial.println("Error sending the data");
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  Serial.begin(115200);

  // Setup PWM
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(PWM_PIN, pwmChannel);
  ledcWrite(pwmChannel, pwmDutyCycle);

  Serial.println("Type 'GO' then press Enter to start:");

  String input;
  while (!ready) {
    if (Serial.available()) {
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.equalsIgnoreCase("GO")) {
        ready = true;
        Serial.println("Starting main loop...");
        Serial.println("You can enter 'ON', 'OFF', or a PWM value (0–100).");
      } else {
        Serial.println("Waiting for 'GO'...");
      }
    }
  }

  // Start WiFi/ESP-NOW task pinned to core 1
  xTaskCreatePinnedToCore(
    wifiTask,   // Function to run
    "WiFi Task",// Name of task
    10000,     // Stack size
    NULL,      // Parameter to pass
    1,          // Priority
    &TaskWiFiHandle, // Task handle
    1);         // Core to run on (1 = second core)


  Serial.println(WiFi.macAddress());

}

void loop() {
  unsigned long currentMillis = millis();

  // Non-blocking LED blink every 1 second
  if (currentMillis - previousMillisLED >= ledInterval) {
    previousMillisLED = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }

  // Print "Hello" every 10 seconds
  if (currentMillis - previousMillisHello >= helloInterval) {
    previousMillisHello = currentMillis;
    Serial.print("Hello! Time since boot: ");
    Serial.print(currentMillis);
    Serial.println(" ms");
  }

  // Serial input for commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("ON")) {
      digitalWrite(OUT_PIN, HIGH);
      Serial.println("OUT_PIN turned ON");
    } else if (cmd.equalsIgnoreCase("OFF")) {
      digitalWrite(OUT_PIN, LOW);
      Serial.println("OUT_PIN turned OFF");
    } else if (cmd.toInt() >= 0 && cmd.toInt() <= 100) {
      int userValue = cmd.toInt();
      pwmDutyCycle = map(userValue, 0, 100, 0, 255);
      ledcWrite(pwmChannel, pwmDutyCycle);
      Serial.print("PWM duty cycle set to ");
      Serial.print(userValue);
      Serial.println("%");
    } else {
      Serial.println("Unknown command. Use ON, OFF, or a number (0–100).");
    }
  }
}
