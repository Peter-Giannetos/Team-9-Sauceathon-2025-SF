/* DS Motor Driver */

/* Includes */
#include <Arduino.h>
#include "ds_motor.h"
#include "servo_util.h"

/* Defines */
#define DEBUG (0U)

/* Private Fnction Definitions */
static inline uint16_t angleToUs(uint8_t deg) {
  return static_cast<uint16_t>(SERVO_MIN_US + ((static_cast<uint32_t>(deg) * 2000U) / 180U));
}

/* Public Function Definitions */

// -----------------------------
// DS positional (manual pulse)
// -----------------------------
uint8_t dsClosedAngleForPin(uint8_t pin) {
  return (pin == OUT2_PIN) ? BOTTOM_BUN_CLOSED : TOP_BUN_CLOSED;
}

uint8_t dsOpenAngleForPin(uint8_t pin) {
  return (pin == OUT2_PIN) ? BOTTOM_BUN_OPEN : TOP_BUN_OPEN;
}

void driveDsServoAngle(uint8_t pin, int angleDeg) {
  angleDeg = constrain(angleDeg, 0, 180);
  uint16_t us = angleToUs(static_cast<uint8_t>(angleDeg));
  // if (DEBUG) { enqueuePrint("DS angle="); enqueuePrint(angleDeg); enqueuePrint(" us="); enqueuePrint("%d\n", us); }
  servoOneFrameWriteUs(pin, us);
}

// Hold a position for holdMs by refreshing every frame
void dsHoldAngle(uint8_t pin, int angleDeg, uint16_t holdMs) {
  uint32_t start = millis();
  while (millis() - start < holdMs) {
    driveDsServoAngle(pin, angleDeg);
  }
}

// CLOSED->OPEN, pause, OPEN->CLOSED
void dsBounce(uint8_t pin, uint16_t pauseMs) {
  uint8_t openA   = dsOpenAngleForPin(pin);
  uint8_t closedA = dsClosedAngleForPin(pin);
  dsHoldAngle(pin, openA,   DS_BOUNCE_MOVE_MS);
  delay(pauseMs);
  dsHoldAngle(pin, closedA, DS_BOUNCE_MOVE_MS);
}