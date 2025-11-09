#include <Arduino.h>
#include "parallax_motor.h"
#include "servo_util.h"
#include "slave_config.h"

// Pins

// Motor instances
extern volatile state lastKnownState;

void setup() {
  Serial.begin(115200);

  startSlave();  // initializes ESP-NOW and starts slave task

  enqueuePrint("Waiting for FSM states from master...\n");

}

void loop() {
  static state lastHandled = STATE_UNKNOWN;
  if (lastHandled != lastKnownState) {
    enqueuePrint("Received State %d\n", lastKnownState);
    // Dispatch based on lastKnownState
    switch (lastKnownState) {
      case STATE_B_DROP:
        // Do something
        // disableWifi();
        parallaxServoBounce(OUT1_PIN, 100, 1000);
        // enableWifi();
        break;
      case STATE_B_BUTTER:
        // disableWifi();
        parallaxServoHalt(OUT1_PIN);
        // enableWifi();
        break;
      // Handle other states
      case STATE_T_DROP:
        // Do something
        // disableWifi();
        parallaxServoBounce(OUT2_PIN, 100, 1000);
        // enableWifi();
        break;
      case STATE_T_BUTTER:
        // disableWifi();
        parallaxServoHalt(OUT2_PIN);
        // enableWifi();
        break;
      default:
        // Unknown or default action
        break;
    }
    lastHandled = lastKnownState;
  }
}
