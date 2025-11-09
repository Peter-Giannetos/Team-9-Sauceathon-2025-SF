/* Parallax Motor Driver */

/* Includes */
#include <Arduino.h>
#include "ds_continuous_motor.h"
#include "servo_util.h"

/* Public Function Definitions */
// -----------------------------
// Continuous (manual pulse)
// -----------------------------
void contServoHalt(uint8_t pin)  { servoOneFrameWriteUs(pin, SERVO_NEUTRAL_US); }
void contServoRight(uint8_t pin) { servoOneFrameWriteUs(pin, 1300); }
void contServoLeft(uint8_t pin)  { servoOneFrameWriteUs(pin, 1700); }
void contServoBounce(uint8_t pin, int cycles) {
  if (cycles < 0) cycles = 0;
  for (int i = 0; i < cycles; ++i) contServoRight(pin);
  for (int i = 0; i < cycles; ++i) contServoLeft(pin);
}