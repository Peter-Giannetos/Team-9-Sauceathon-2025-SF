#include <Arduino.h>

#define LED_PIN           (2)
#define OUT1_PIN          (13)
#define OUT2_PIN          (14)  // TODO!
#define DEBUG             (0U)
#define DS_MOTOR          (0U)
#define TOWER_PRO_MOTOR   (0U)
#define PARALLAX_MOTOR    (0U)
#define DS_CONTINUOUS_MOTOR  (1U)
#if DS_MOTOR
#define POSITIONAL_MOTOR  (DS_MOTOR)
#define TOP_BUN_CLOSED    (170U)
#define TOP_BUN_OPEN      (80U)
#define BOTTOM_BUN_CLOSED    (90U)
#define BOTTOM_BUN_OPEN      (0U)
#define CLOSED_STATE      (1U)
#define OPEN_STATE        (0U)
#elif DS_CONTINUOUS_MOTOR
#define CONTINUOUS_MOTOR  (DS_CONTINUOUS_MOTOR)
// TODO: ...
#elif TOWER_PRO_MOTOR
#define POSITIONAL_MOTOR  (TOWER_PRO_MOTOR)
#elif PARALLAX_MOTOR
#define CONTINUOUS_MOTOR  (PARALLAX_MOTOR)
#endif

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

void drive_parallax_servo_bounce(int pin, int duration) {
  // assume pin is LOW at start of function
  int i;
  for (i = 0; i < duration; i++)
  {
    drive_parallax_servo_clockwise(pin);
  }
  for (i = 0; i < duration; i++)
  {
    drive_parallax_servo_counterwise(pin);
  }
}
#endif // PARALLAX_MOTOR

#if TOWER_PRO_MOTOR
void drive_tower_pro_servo(int pin, int degrees) {
  // assume pin is LOW at start of function
  delayMicroseconds(20000 - degrees);
  digitalWrite(pin, HIGH);
  delayMicroseconds(degrees);
  digitalWrite(pin, LOW);
}
#endif // TOWER_PRO_MOTOR

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

void drive_ds_servo_top_bun(bool open) {
  int pin = OUT1_PIN;
  int degrees = (open) ? (TOP_BUN_OPEN) : (TOP_BUN_CLOSED);

  drive_ds_servo(pin, degrees);
}

void drive_ds_servo_bottom_bun(bool open) {
  int pin = OUT2_PIN;
  int degrees = (open) ? (BOTTOM_BUN_OPEN) : (BOTTOM_BUN_CLOSED);

  drive_ds_servo(pin, degrees);
}
#endif // DS_MOTOR

void setup() {
  // put your setup code here, to run once:

  // Setup Serial
  init_serial();

  // Setup Pins
  pinMode(LED_PIN, OUTPUT); // D2
  toggle_led(LED_PIN);
  pinMode(OUT1_PIN, OUTPUT); // D13

  // Serial Interface
  wait_for_user();
}

void loop() {
  static int num = 90;
  static int dir = 0;
  static bool bounce_flag = false;
  static bool stateful_flag = false;
  static bool open_flag = false;
  String input;

#if POSITIONAL_MOTOR
  Serial.println("Please enter value from 0 to 180...: ");

  while(true)
  {
    // continuously drive motor to position
#if DS_MOTOR
    if (stateful_flag)
    {
      drive_ds_servo(OUT1_PIN, num);
    }
    else
    {
      drive_ds_servo_top_bun(open_flag);
      drive_ds_servo_bottom_bun(open_flag);
    }
#elif TOWER_PRO_MOTOR
    drive_tower_pro_servo(OUT1_PIN, num * 2);
#endif

    // update position
    if (Serial.available())
    {
      input = Serial.readStringUntil('\n');
      input.trim();

      // Test string or integer input
      if ( input.charAt(0) >= 'a' && input.charAt(0) <= 'z' )
      {
        stateful_flag = true;
        open_flag = (strcmp(input.c_str(), "open") > 0);
        Serial.printf("DEBUG: open_flag=%d", open_flag);
      }
      else
      {
        stateful_flag = false;
        num = input.toInt();
        Serial.printf("Driving motor to %d degrees...\n", num);
      }

      break;
    }
  }


#elif CONTINUOUS_MOTOR

  // drive_parallax_servo_halt(OUT1_PIN);
  Serial.println("Please enter value 'r', 'l', or 's' for right, left, stop...: ");
  Serial.println("Or, please enter number for BOUNCE");

  while(true)
  {
    // continuously drive motor to position
    if (dir < 0)
    {
#if PARALLAX_MOTOR
      drive_parallax_servo_counterwise(OUT1_PIN);
#endif
    }
    else if (dir > 0)
    {
#if PARALLAX_MOTOR
      drive_parallax_servo_clockwise(OUT1_PIN);
#endif
    }
    else if (bounce_flag)
    {
      // side effect: set dir = 0 when done
#if PARALLAX_MOTOR
      drive_parallax_servo_bounce(OUT1_PIN, num);
#endif
      dir = 0;
      bounce_flag = false;
    }
    else
    {
#if PARALLAX_MOTOR
      drive_parallax_servo_halt(OUT1_PIN);
#endif
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
          bounce_flag = false;
          break;
        case 'l':
          Serial.println("Drive motor to LEFT");
          dir = -1;
          bounce_flag = false;
          break;
        case 's':
          Serial.println("Drive motor to HALT");
          dir = 0;
          bounce_flag = false;
        default:
          if ( mode > '0' && mode <= '9' )
          {
            bounce_flag = true;
            num = input.toInt();
            Serial.printf("Bounce motor by %d cycles\n", num);
          }
          break;
      }

      break;
    }
  }

#endif // MOTOR SELECTION
}
