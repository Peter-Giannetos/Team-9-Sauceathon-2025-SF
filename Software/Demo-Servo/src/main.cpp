#include <Arduino.h>

#define LED_PIN           (2)
#define OUT_PIN           (13)
#define DEBUG             (0U)
#define DS_MOTOR          (0U)
#define PARALLAX_MOTOR    (1U)
#define POSITIONAL_MOTOR  (DS_MOTOR)
#define CONTINUOUS_MOTOR  (PARALLAX_MOTOR)

void init_serial() {
  Serial.begin(9600);
  delay(1000);
  while(!Serial);
}

void wait_for_user() {
  Serial.println("Type 'y' then press Enter to start:");

  String input;
  bool ready = false;
  while (!ready) {
    if (Serial.available()) {
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.equalsIgnoreCase("y")) {
        ready = true;
        Serial.println("Starting main loop...");
      } else {
        Serial.println("Waiting for 'y'...");
      }
    }
  }
}

void toggle_led(int pin) {
  digitalWrite(pin, LOW);
  delay(1000);
  digitalWrite(pin, HIGH);
  delay(1000);
}

#if PARALLAX_MOTOR
void drive_parallax_servo_halt(int pin) {
  // assume pin is LOW at start of function
  delay(20);
  digitalWrite(pin, HIGH);
  delayMicroseconds(1500);  // 1.5ms UPTIME per 20ms DOWNTIME
  digitalWrite(pin, LOW);
}

void drive_parallax_servo_clockwise(int pin) {
  // assume pin is LOW at start of function
  delay(20);
  digitalWrite(pin, HIGH);
  delayMicroseconds(1300);  // 1.3ms UPTIME per 20ms DOWNTIME
  digitalWrite(pin, LOW);
}

void drive_parallax_servo_counterwise(int pin) {
  // assume pin is LOW at start of function
  delay(20);
  digitalWrite(pin, HIGH);
  delayMicroseconds(1700);  // 1.7ms UPTIME per 20ms DOWNTIME
  digitalWrite(pin, LOW);
}
#endif // PARALLAX_MOTOR

#if DS_MOTOR
void drive_ds_servo(int pin, int degrees) {
  int target_us;

  if ( degrees < 0 || degrees > 180 )
  {
    // fail
    return;
  }
  // 0 degrees --> 500 us
  // 90 degrees --> 1500 us
  // 180 degrees --> 2500 us
  target_us = ((degrees * 2000) / 180) + 500;
  if (DEBUG)
    Serial.printf("DEBUG: degrees = %d\n", degrees);
  if (DEBUG)
    Serial.printf("DEBUG: target_us = %d\n", target_us);

  delay(20);
  digitalWrite(pin, HIGH);
  delayMicroseconds(target_us);
  digitalWrite(pin, LOW);
}
#endif // DS_MOTOR

void setup() {
  // put your setup code here, to run once:

  // Setup Serial
  init_serial();

  // Setup Pins
  pinMode(LED_PIN, OUTPUT); // D2
  toggle_led(LED_PIN);
  pinMode(OUT_PIN, OUTPUT); // D13

  // Serial Interface
  wait_for_user();
}

void loop() {
  static int num = 90;
  static int dir = 0;
  String input;

#if POSITIONAL_MOTOR
  // drive_parallax_servo_halt(OUT_PIN);
  Serial.println("Please enter value from 0 to 180...: ");

  while(true)
  {
    // continuously drive motor to position
    drive_ds_servo(OUT_PIN, num);

    // update position
    if (Serial.available())
    {
      input = Serial.readStringUntil('\n');
      input.trim();
      num = input.toInt();

      break;
    }
  }

  Serial.printf("Driving motor to %d degrees...\n", num);

#elif CONTINUOUS_MOTOR

  // drive_parallax_servo_halt(OUT_PIN);
  Serial.println("Please enter value 'r', 'l', or 's' for right, left, stop...: ");

  while(true)
  {
    // continuously drive motor to position
    if (dir < 0)
    {
      drive_parallax_servo_counterwise(OUT_PIN);
    }
    else if (dir > 0)
    {
      drive_parallax_servo_clockwise(OUT_PIN);
    }
    else 
    {
      drive_parallax_servo_halt(OUT_PIN);
    }

    // update position
    if (Serial.available())
    {
      input = Serial.readStringUntil('\n');
      input.trim();
      char mode = input.charAt(0);
      switch (mode)
      {
        case 'r':
          Serial.println("Drive motor to RIGHT");
          dir = 1;
          break;
        case 'l':
          Serial.println("Drive motor to LEFT");
          dir = -1;
          break;
        case 's':
        default:
          Serial.println("Drive motor to HALT");
          dir = 0;
          break;
      }

      break;
    }
  }

#endif // MOTOR SELECTION
}
