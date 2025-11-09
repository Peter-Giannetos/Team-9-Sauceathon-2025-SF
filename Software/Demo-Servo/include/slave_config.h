#ifndef SLAVE_CONFIG_H
#define SLAVE_CONFIG_H

/* Slave Config Header */

/* Includes */
#include <WiFi.h>
#include <esp_now.h>

/* Defines */
#define WIFI_SLAVE_TASK (1U)
#define UNIQUE_NAME     (0xDEAD)

/* Typedefs */
// ESP-NOW message structure
typedef struct struct_message {
  char msg[32];
  int value;
} struct_message;

/* Public Functions Declarations */
void enqueuePrint(const char* fmt, ...);
void startSlave();

#endif // CONFIG_H