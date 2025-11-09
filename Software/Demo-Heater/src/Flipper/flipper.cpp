#include <Arduino.h>
#include <Servo.h>
#include "flipper.h"
#include "slave_config.h"

#define servoPin1 1// 10
#define servoPin2 22// 11
#define toastServoPin 18
#define buttonPin 23

#define WIGGLES 5
#define WIGGLE_SPEED_MS 200
#define IDLE_ANGLE 180
#define THROW_ANGLE 200
flipperState_E state = IDLE;
bool prevButton = false;
bool currButton = false;
Servo toastServo;
Servo flipServo1;
Servo flipServo2;

bool toastServoClosed = true;

// 0 is starting angle 200 is end angle
void goToAngle(int angle)
{
  flipServo1.write(200 - angle);
  flipServo2.write(0 + angle);
}

void setupFlipperTask(void)
{
  flipServo1.attach(servoPin1);
  flipServo2.attach(servoPin2);
  toastServo.attach(toastServoPin);
  pinMode(buttonPin, INPUT_PULLUP);

  delay(1000);
  toastServo.write(0);
  delay(1000);
  toastServo.write(90);
  delay(1000);
  for(int i = 90; i < IDLE_ANGLE; i++)
  {
    goToAngle(i);
    delay(10);
  }
  // enqueuePrint("INIT FLIP: ");
}

void FlipperTask(void* parameter) {  
  while(true) {
    // enqueuePrint("State: %d\n", (int)state);
    // currButton = digitalRead(buttonPin);
    if(currButton && prevButton == false)
    {
      switch(state) {
        case IDLE:
        {
          // Go to load state
          for(int i = IDLE_ANGLE; i > 0; i--)
          {
            goToAngle(i);
            vTaskDelay(10 / portTICK_PERIOD_MS);
          }
          state = LOAD;
          break;
        }
        case LOAD:
        {
          // wiggle
          for(int i = 0; i < WIGGLES; i++)
          {
            goToAngle(15);
            vTaskDelay(WIGGLE_SPEED_MS / portTICK_PERIOD_MS);

            // swing arm back
            goToAngle(0);
            vTaskDelay(WIGGLE_SPEED_MS / portTICK_PERIOD_MS);
          }
          vTaskDelay(1000 / portTICK_PERIOD_MS);

          // throw
          for (int i = 0; i < THROW_ANGLE; i+=5)
          {
            goToAngle(i);
            vTaskDelay(5 / portTICK_PERIOD_MS);
          }
          vTaskDelay(1000 / portTICK_PERIOD_MS);

          // swing arm back to midpoint
          for (int i = THROW_ANGLE; i > IDLE_ANGLE; i-= 1)
          {
            goToAngle(i);
            vTaskDelay(10 / portTICK_PERIOD_MS);
          }
          state = IDLE;
          break;
        }
      }
    }

    if(toastServoClosed)
    {
      toastServo.write(90);
    }
    else {
      toastServo.write(0);
    }

    prevButton = currButton;
    if(currButton)
    {
      currButton = false; // unlatch trigger
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

flipperState_E getFlipperState(void)
{
  return state;
}

void triggerFlipper(void)
{
  currButton = true;
}

void closeToastServo(bool closed)
{
  toastServoClosed = closed;
}
