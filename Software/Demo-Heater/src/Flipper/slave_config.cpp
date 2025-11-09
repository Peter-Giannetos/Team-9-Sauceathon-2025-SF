/* Slave Config Driver */

/* Includes */
#include "slave_config.h"
#include "flipper.h"

enum state
{
  STATE_B_DETECT_BUTTON, // 1. Go to STATE_B_DROP
  STATE_B_DROP,          // 1. Reset gate (butter/toast), Reset flipper (Open Top) | 2. Open bottom dropper | 3. Wait
  STATE_B_BUTTER,        // 1. Apply butter | 2. Open gate butter | 3. Wait
  STATE_B_TOAST,         // 1. Measure sound & heat | 2. Brand | 3. open gate toast | 4. Wait
  STATE_B_DISPENSE,      // 1. Dispense pusher | 2. Wait
  STATE_T_DETECT_BUTTON, // 1. Go to STATE_T_DROP
  STATE_T_DROP,          // 1. Reset gate (butter/toast), Reset flipper (Close Top) | 2. Open top dropper | 3. Wait
  STATE_T_BUTTER,        // 1. Apply butter | 2. Open gate butter | 3. Wait
  STATE_T_TOAST,         // 1. Measure sound & heat | 2. Brand | 3. open gate toast | 4. Wait
  STATE_T_DISPENSE,      // 1. Dispense flipper | 2. Wait
};

/* Statics */
struct_message incomingData;
struct_message outgoingData;
char outgoingMsg[32] = "Hello ESP-NOW"; // Default message - Modify with new messages

// Master's MAC address
uint8_t masterMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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
#define PRINT_BUFFER_SIZE (128)
#define PRINT_BUFFER_COUNT (10)

/* Private Function Definitions */

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingDataPtr, int len)
{

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
  if (strcmp(incomingData.msg, "FSM STATE 5") == 0)
  {
    // Function Call 1
    if (getFlipperState() != LOAD)
    {
      triggerFlipper();
    }
  }
  else if ((strcmp(incomingData.msg, "FSM STATE 4") == 0) || (strcmp(incomingData.msg, "FSM STATE 9") == 0))
  {
    // return to idle
    if (strcmp(incomingData.msg, "FSM STATE 9") == 0)
    {
      // Function Call 2
      if (getFlipperState() != IDLE)
      {
        triggerFlipper();
      }
    }
    closeToastServo(true);
  }

  else if ((strcmp(incomingData.msg, "FSM STATE 2") == 0) || (strcmp(incomingData.msg, "FSM STATE 7") == 0))
  {
    // Toast gate control at butter step
    closeToastServo(true);
  }
}

// FUNCTION: ESP-NOW Send Message
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  // enqueuePrint("Last Packet Send Status: %s\n",
  //              status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// FUNCTION: Prints strings from global queue
void printTask(void *parameter)
{
  char localBuffer[PRINT_BUFFER_SIZE];
  while (true)
  {
    if (xQueueReceive(printQueue, localBuffer, portMAX_DELAY))
    {
      Serial.print(localBuffer);
    }
  }
}

// FUNCTION: ESP-NOW WiFi task
void wifiTask(void *parameter)
{
  enqueuePrint("WiFi Task started on core: %d\n", xPortGetCoreID());

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK)
  {
    enqueuePrint("Error initializing ESP-NOW\n");
    vTaskDelete(NULL);
  }

  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  // Add broadcast peer
  esp_now_peer_info_t peerInfo = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Listen to everyone
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};   // Broadcast to everyone
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
      enqueuePrint("Failed to add peer\n");
    }
  }

  while (1)
  {
    strcpy(outgoingData.msg, outgoingMsg);
    outgoingData.value = millis() / 1000;

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&outgoingData, sizeof(outgoingData));

    if (result == ESP_OK)
    {
      enqueuePrint("Sent with success\n");
    }
    else
    {
      enqueuePrint("Error sending the data\n");
    }
    vTaskDelay(500 / portTICK_PERIOD_MS); // Sleep 500 mS
  }
}

/* Public Function Definitions */

// FUNCTION: Sends strings to global queue
void enqueuePrint(const char *fmt, ...)
{
  char tempBuf[PRINT_BUFFER_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(tempBuf, PRINT_BUFFER_SIZE, fmt, args);
  va_end(args);
  xQueueSend(printQueue, tempBuf, 10 / portTICK_PERIOD_MS);
}

void startSlave()
{

  // Create a print queue
  printQueue = xQueueCreate(10, PRINT_BUFFER_SIZE);
  if (printQueue == NULL)
  {
    Serial.println("Failed to create print queue");
    while (1)
      ;
  }

  // Start printing task
  xTaskCreatePinnedToCore(printTask, "Print Task", 4096, NULL, 1, NULL, 0);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK)
  {
    enqueuePrint("Error initializing ESP-NOW");
    while (1)
      ;
  }

  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);

  // Register peer (the master)
  esp_now_peer_info_t peerInfo = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  memcpy(peerInfo.peer_addr, masterMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(masterMAC))
  {
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
      enqueuePrint("Failed to add master peer");
    }
  }

  enqueuePrint("Slave ready. Waiting for data...");
}