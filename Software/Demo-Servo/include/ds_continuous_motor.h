/* DC (Continuous) Motor Header */
#ifndef DC_CONTINUOUS_MOTOR_H
#define DC_CONTINUOUS_MOTOR_H

/* Public Function Definitions */
void contServoHalt(uint8_t pin);
void contServoRight(uint8_t pin);
void contServoLeft(uint8_t pin);
void contServoBounce(uint8_t pin, int cycles);

#endif // DC_CONTINUOUS_MOTOR_H