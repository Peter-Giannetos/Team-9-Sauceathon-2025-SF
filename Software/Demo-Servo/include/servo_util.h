/* Common Utils for Servo Header */
#ifndef SERVO_UTILS_H
#define SERVO_UTILS_H

/* Includes */
#include <Arduino.h>

/* Defines */

// -----------------------------
// Pins
// -----------------------------
constexpr uint8_t LED_PIN  = 2;
constexpr uint8_t OUT1_PIN = 13;   // Primary servo output
constexpr uint8_t OUT2_PIN = 14;   // Secondary (DS dual-servo use)

// -----------------------------
// Servo timing constants
// -----------------------------
constexpr uint16_t SERVO_FRAME_MS   = 20;    // Typical RC servo frame
// constexpr uint16_t SERVO_NEUTRAL_US = 1500;  // Neutral for continuous
// constexpr uint16_t SERVO_MIN_US     = 500;   // Positional min pulse
// constexpr uint16_t SERVO_MAX_US     = 2500;  // Positional max pulse

/* Public Function Definitions */
void servoOneFrameWriteUs(uint8_t pin, uint16_t highUs);

#endif // SERVO_UTILS_H