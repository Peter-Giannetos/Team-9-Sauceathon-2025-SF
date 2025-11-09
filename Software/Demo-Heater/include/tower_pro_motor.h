/* Tower Pro Motor Header */
#ifndef TOWER_PRO_MOTOR_H
#define TOWER_PRO_MOTOR_H

/* Includes */
#include <Servo.h>

/* Constants */
// -----------------------------
// TowerPro MG995 (Servo lib)
// -----------------------------
extern Servo MG995_servo;
constexpr uint8_t TOWER_PRO_FORWARD   = 0;
constexpr uint8_t TOWER_PRO_BACKWARD  = 180;
constexpr uint8_t TOWER_PRO_HALT      = 90;

/* Public Function Definitions */
void towerProInit(int pin);
void towerProClockwise();
void towerProCounterwise();
void towerProHalt();
void towerProBounce(uint16_t durationMs, uint16_t pauseMs);

#endif // TOWER_PRO_MOTOR_H