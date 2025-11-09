/* Tower Pro Motor Driver */

/* Includes */
#include <Arduino.h>
#include "tower_pro_motor.h"
#include "servo_util.h"
#include <Servo.h>

/* Externs */
Servo MG995_servo;

/* Public Function Definitions */
// -----------------------------
// TowerPro MG995 (Servo lib)
// -----------------------------
void towerProInit(int pin) {
  MG995_servo.attach(pin);
}
void towerProClockwise()    { MG995_servo.write(TOWER_PRO_FORWARD); }
void towerProCounterwise()  { MG995_servo.write(TOWER_PRO_BACKWARD); }
void towerProHalt()         { MG995_servo.write(TOWER_PRO_HALT); }
void towerProBounce(uint16_t durationMs, uint16_t pauseMs) {
  towerProClockwise();  delay(durationMs);
  towerProHalt();       delay(pauseMs);
  towerProCounterwise();delay(durationMs);
  towerProHalt();
}