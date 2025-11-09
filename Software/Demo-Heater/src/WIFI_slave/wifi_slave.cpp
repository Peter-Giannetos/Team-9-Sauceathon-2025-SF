#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// Structure must match the sender
typedef struct struct_message {
  char msg[32];
  int value;
} struct_message;

struct_message incomingData;
struct_message outgoingData;

// Master's MAC address
uint8_t masterMAC[] = {0x88, 0x13, 0xBF, 0x0B, 0xC4, 0x58};

QueueHandle_t printQueue;
const int PRINT_BUFFER_SIZE = 128;

// Add messages to the print queue
void enqueuePrint(const char* fmt, ...) {
  char tempBuf[PRINT_BUFFER_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tempBuf, PRINT_BUFFER_SIZE, fmt, args);
  va_end(args);
  xQueueSend(printQueue, tempBuf, portMAX_DELAY);
}

// Dedicated print task
void printTask(void* parameter) {
  char localBuffer[PRINT_BUFFER_SIZE];
  while (true) {
    if (xQueueReceive(printQueue, localBuffer, portMAX_DELAY)) {
      Serial.println(localBuffer);
    }
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t* mac, const uint8_t* incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Print received message
  enqueuePrint("Received from %s: %s, value: %d", macStr, incomingData.msg, incomingData.value);

  // Prepare acknowledgment
  strcpy(outgoingData.msg, "Ack from Slave");
  outgoingData.value = millis() / 1000;

  esp_err_t result = esp_now_send(masterMAC, (uint8_t*)&outgoingData, sizeof(outgoingData));
  if (result == ESP_OK) {
    enqueuePrint("Ack sent back to master");
  } else {
    enqueuePrint("Failed to send ack");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Create a print queue
  printQueue = xQueueCreate(10, PRINT_BUFFER_SIZE);
  if (printQueue == NULL) {
    Serial.println("Failed to create print queue");
    while (1);
  }

  // Start printing task
  xTaskCreatePinnedToCore(printTask, "Print Task", 4096, NULL, 1, NULL, 0);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    enqueuePrint("Error initializing ESP-NOW");
    while (1);
  }

  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);

  // Register peer (the master)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, masterMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(masterMAC)) {
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      enqueuePrint("Failed to add master peer");
    }
  }

  enqueuePrint("Slave ready. Waiting for data...");
}

void loop() {
  vTaskDelay(100 / portTICK_PERIOD_MS);
}
