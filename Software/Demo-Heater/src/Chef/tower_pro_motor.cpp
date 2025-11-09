/* Tower Pro Motor Driver */

/* Includes */
#include <Arduino.h>
#include "tower_pro_motor.h"
#include "servo_util.h"

/* Public Function Definitions */
// -----------------------------
// TowerPro MG995 (Servo lib)
// -----------------------------
void towerProInit() {
  MG995_Servo.attach(OUT1_PIN);
}
void towerProClockwise()    { MG995_Servo.write(TOWER_PRO_FORWARD); }
void towerProCounterwise()  { MG995_Servo.write(TOWER_PRO_BACKWARD); }
void towerProHalt()         { MG995_Servo.write(TOWER_PRO_HALT); }
void towerProBounce(uint16_t durationMs, uint16_t pauseMs) {
  towerProClockwise();  delay(durationMs);
  towerProHalt();       delay(pauseMs);
  towerProCounterwise();delay(durationMs);
  towerProHalt();
}