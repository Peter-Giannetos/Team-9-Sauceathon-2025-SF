// tower_pro_motor.cpp
#include <Arduino.h>
#include "tower_pro_motor.h"

void TowerProMotor::attach(int pin) {
    servo.attach(pin);
}

void TowerProMotor::bounce(uint16_t durationMs, uint16_t pauseMs) {
    clockwise();
    delay(durationMs);
    halt();
    delay(pauseMs);
    counterwise();
    delay(durationMs);
    halt();
}