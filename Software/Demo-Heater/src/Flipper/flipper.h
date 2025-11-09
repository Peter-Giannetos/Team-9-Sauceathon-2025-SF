#include <Arduino.h>

typedef enum {
    IDLE,
    LOAD,
    UNLOAD,
} flipperState_E;

void setupFlipperTask(void);
void FlipperTask(void* parameter);
void triggerFlipper(void);
flipperState_E getFlipperState(void);
void closeToastServo(bool closed);
