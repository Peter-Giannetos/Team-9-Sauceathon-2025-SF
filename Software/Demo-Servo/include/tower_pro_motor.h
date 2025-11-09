// tower_pro_motor.h
#ifndef TOWER_PRO_MOTOR_H
#define TOWER_PRO_MOTOR_H

#include <Servo.h>

class TowerProMotor {
public:
    // Constants (same for all instances)
    static constexpr uint8_t FORWARD = 0;
    static constexpr uint8_t BACKWARD = 180;
    static constexpr uint8_t HALT = 90;

    TowerProMotor() = default; // default constructor

    // Initialize servo on given pin
    void attach(int pin);

    // Basic controls
    void clockwise() { servo.write(FORWARD); }
    void counterwise() { servo.write(BACKWARD); }
    void halt() { servo.write(HALT); }

    // Bounce with duration and pause (blocking)
    void bounce(uint16_t durationMs, uint16_t pauseMs);

    // each instance owns its Servo object
    Servo servo; 

private:
};

#endif // TOWER_PRO_MOTOR_H
