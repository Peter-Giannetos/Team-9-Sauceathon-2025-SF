/* DS Motor Header */

#ifndef DS_MOTOR_H
#define DS_MOTOR_H

/* Includes */

/* Constants */
// -----------------------------
// DS positional presets (per pin)
// -----------------------------
constexpr uint8_t TOP_BUN_CLOSED     = 170;  // OUT1_PIN
constexpr uint8_t TOP_BUN_OPEN       = 80;   // OUT1_PIN
constexpr uint8_t BOTTOM_BUN_CLOSED  = 90;   // OUT2_PIN
constexpr uint8_t BOTTOM_BUN_OPEN    = 0;    // OUT2_PIN
constexpr uint16_t DS_BOUNCE_MOVE_MS = 500;  // time to hold each target

/* Public Function Definitions */
uint8_t dsClosedAngleForPin(uint8_t pin);
uint8_t dsOpenAngleForPin(uint8_t pin);
void driveDsServoAngle(uint8_t pin, int angleDeg);
void dsHoldAngle(uint8_t pin, int angleDeg, uint16_t holdMs);
void dsBounce(uint8_t pin, uint16_t pauseMs);

#endif // DS_MOTOR_H