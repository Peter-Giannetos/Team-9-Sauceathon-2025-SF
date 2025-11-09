/* Parallax Motor Driver using Servo.h */

/* Includes */
#include <Arduino.h>
#include <Servo.h>
#include "parallax_motor.h"
#include "servo_util.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp32/rom/ets_sys.h"

portMUX_TYPE myMux = portMUX_INITIALIZER_UNLOCKED;

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

    portENTER_CRITICAL(&myMux);  // Correct usage: pass address of portMUX_TYPE

    for (int i = 0; i < cycles; ++i) {
        parallaxServo.writeMicroseconds(SERVO_RIGHT_US);
        delay(20);  // Use delay here inside critical section for smooth pulses
    }

    // Instead of vTaskDelay (which requires yielding), use delay with critical section kept
    // For longer delays, you might break this into smaller chunks to avoid watchdog reset
    for (int i = 0; i < pause; ++i) {
        parallaxServo.writeMicroseconds(SERVO_NEUTRAL_US);
        delay(20);
    }

    for (int i = 0; i < cycles; ++i) {
        parallaxServo.writeMicroseconds(SERVO_LEFT_US);
        delay(20);
    }

    portEXIT_CRITICAL(&myMux);  // Exit critical section (re-enable interrupts)
}
