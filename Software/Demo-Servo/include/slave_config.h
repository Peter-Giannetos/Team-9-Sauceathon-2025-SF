#ifndef SLAVE_CONFIG_H
#define SLAVE_CONFIG_H

#include <WiFi.h>
#include <esp_now.h>
#include <Arduino.h> // for volatile and helper functions

#define WIFI_SLAVE_TASK (1U)
#define UNIQUE_NAME     (0xDEAD)

typedef struct struct_message {
  char msg[32];
  int value;
} struct_message;

// FSM states enum matching your master states
enum state {
    STATE_B_DETECT_BUTTON,
    STATE_B_DROP,
    STATE_B_BUTTER,
    STATE_B_TOAST,
    STATE_B_DISPENSE,
    STATE_T_DETECT_BUTTON,
    STATE_T_DROP,
    STATE_T_BUTTER,
    STATE_T_TOAST,
    STATE_T_DISPENSE,
    STATE_UNKNOWN  // catch-all
};

void enqueuePrint(const char* fmt, ...);
void startSlave();
void disableWifi();
void enableWifi();

extern volatile state lastKnownState;  // Updated from incomingData.msg

#endif // SLAVE_CONFIG_H
