/* Common Utils for Servos Driver */

/* Includes */
#include "servo_util.h"

/* Public Function Definitions */
// -----------------------------
// Generic single-frame pulse (manual PWM)
// -----------------------------
void servoOneFrameWriteUs(uint8_t pin, uint16_t highUs) {
  if (highUs < 400)  highUs = 400;
  if (highUs > 2700) highUs = 2700;
  delay(SERVO_FRAME_MS);
  digitalWrite(pin, HIGH);
  delayMicroseconds(highUs);
  digitalWrite(pin, LOW);
}
