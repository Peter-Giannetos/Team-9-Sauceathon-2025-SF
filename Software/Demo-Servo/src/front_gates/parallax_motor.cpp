/* Parallax Motor Driver using Servo.h */

/* Includes */
#include <Arduino.h>
#include <Servo.h>
#include "parallax_motor.h"
#include "servo_util.h"

/* Local Servo instance */
static Servo parallaxServo;

/* Servo pulse constants */
int SERVO_NEUTRAL_US = 1500;
int SERVO_RIGHT_US = 1300;
int SERVO_LEFT_US = 1700;


/* Public Functions */

// Attach servo to pin (preserves original API parameter)
void parallaxServoHalt(uint8_t pin) {
    parallaxServo.detach();
    parallaxServo.attach(pin);
    parallaxServo.writeMicroseconds(SERVO_NEUTRAL_US);
}

void parallaxServoRight(uint8_t pin) {
    parallaxServo.detach();
    parallaxServo.attach(pin);
    parallaxServo.writeMicroseconds(SERVO_RIGHT_US);
}

void parallaxServoLeft(uint8_t pin) {
    parallaxServo.detach();
    parallaxServo.attach(pin);
    parallaxServo.writeMicroseconds(SERVO_LEFT_US);
}

void parallaxServoBounce(uint8_t pin, int cycles, int pause) {
    parallaxServo.detach();
    parallaxServo.attach(pin);
    if (cycles < 0) cycles = 0;
    for (int i = 0; i < cycles; ++i) {
        parallaxServoRight(pin);
    }
    // Maintain neutral pulse during pause to keep motor stable
    unsigned long pauseStart = xTaskGetTickCount();
    while ((xTaskGetTickCount() - pauseStart) < pdMS_TO_TICKS(pause)) {
        parallaxServoHalt(pin);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    for (int i = 0; i < cycles; ++i) {
        parallaxServoLeft(pin);
    }
}
