#include <Arduino.h>
#include <slave_config.h>

#include <Servo.h>

typedef enum {
  IDLE,
  LOAD,
  UNLOAD,
} state_E;

#define servoPin1 1// 10
#define servoPin2 22// 11
#define buttonPin 23

#define WIGGLES 5
#define WIGGLE_SPEED_MS 200
#define IDLE_ANGLE 180
#define THROW_ANGLE 200
bool prevButton = false;
Servo flipServo1;
Servo flipServo2;

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
  pinMode(buttonPin, INPUT_PULLUP);

  delay(1000);
  for(int i = 90; i < IDLE_ANGLE; i++)
  {
    goToAngle(i);
    delay(10);
  }
}

void FlipperTask(void* parameter) {
  state_E state = IDLE;
  bool prevButton = false;
  
  while(true) {
    bool currButton = digitalRead(buttonPin);
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

    prevButton = currButton;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
  
void setup() {
  Serial.begin(115200);
  delay(500);

  flipServo1.attach(servoPin1);
  flipServo2.attach(servoPin2);
  pinMode(buttonPin, INPUT_PULLUP);

  delay(1000);
  for(int i = 90; i < IDLE_ANGLE; i++)
  {
    goToAngle(i);
    delay(10);
  }

  startSlave();
}

void loop() {
  Serial.println(state);
  bool currButton = digitalRead(buttonPin);
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

  prevButton = currButton;
  vTaskDelay(10 / portTICK_PERIOD_MS);
}


