/* Parallax Motor Header */
#ifndef PARALLAX_MOTOR_H
#define PARALLAX_MOTOR_H

/* Public Function Definitions */
void parallaxServoHalt(uint8_t pin);
void parallaxServoRight(uint8_t pin);
void parallaxServoLeft(uint8_t pin);
void parallaxServoBounce(uint8_t pin, int cycles);

#endif // PARALLAX_MOTOR_H