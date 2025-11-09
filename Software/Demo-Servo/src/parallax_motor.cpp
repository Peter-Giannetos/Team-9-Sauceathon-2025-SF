/* Parallax Motor Driver */

/* Includes */
#include <Arduino.h>
#include "parallax_motor.h"
#include "servo_util.h"

/* Public Function Definitions */
// -----------------------------
// Continuous (manual pulse)
// -----------------------------
void parallaxServoHalt(uint8_t pin)  { servoOneFrameWriteUs(pin, SERVO_NEUTRAL_US); }
void parallaxServoRight(uint8_t pin) { servoOneFrameWriteUs(pin, 1300); }
void parallaxServoLeft(uint8_t pin)  { servoOneFrameWriteUs(pin, 1700); }
void parallaxServoBounce(uint8_t pin, int cycles) {
  if (cycles < 0) cycles = 0;
  for (int i = 0; i < cycles; ++i) parallaxServoRight(pin);
  for (int i = 0; i < cycles; ++i) parallaxServoLeft(pin);
}