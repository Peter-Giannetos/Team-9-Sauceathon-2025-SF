/* Slave Config Driver */

/* Includes */
#include "slave_config.h"


/* Statics */
struct_message incomingData;
struct_message outgoingData;
char outgoingMsg[32] = "Hello ESP-NOW";  // Default message - Modify with new messages

// Master's MAC address
uint8_t masterMAC[] = {0x88, 0x13, 0xBF, 0x0B, 0xC4, 0x58};

QueueHandle_t printQueue;
const int PRINT_BUFFER_SIZE = 128;

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
#define PRINT_BUFFER_SIZE   (128)
#define PRINT_BUFFER_COUNT  (10)

/* Private Function Definitions */


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

// FUNCTION: ESP-NOW Send Message
void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  // enqueuePrint("Last Packet Send Status: %s\n",
  //              status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
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

/* Public Function Definitions */

// FUNCTION: Sends strings to global queue
void enqueuePrint(const char* fmt, ...) {
  char tempBuf[PRINT_BUFFER_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tempBuf, PRINT_BUFFER_SIZE, fmt, args);
  va_end(args);
  xQueueSend(printQueue, tempBuf, 10 / portTICK_PERIOD_MS);
}

void startSlave() {
  
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