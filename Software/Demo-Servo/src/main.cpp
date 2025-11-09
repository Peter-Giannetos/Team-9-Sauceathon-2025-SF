// Standalone sketch

#include <Arduino.h>
#include "slave_config.h"
#include "servo_util.h"

// -----------------------------
// Utils
// -----------------------------
#define MAX(x,y) ((x > y) ? (x) : (y))

// -----------------------------
// Compile-time target selection
// Set exactly one to 1
// -----------------------------
#define DS_MOTOR              (0U)  // Discrete positional (manual pulse)
#define DS_CONTINUOUS_MOTOR   (0U)  // Continuous (manual pulse)
#define TOWER_PRO_MOTOR       (0U)  // TowerPro MG995 via Servo lib (positional)
#define PARALLAX_MOTOR        (1U)  // Continuous (manual pulse)

// -----------------------------
// Sanity check: exactly one target
// -----------------------------
#if (DS_MOTOR + DS_CONTINUOUS_MOTOR + TOWER_PRO_MOTOR + PARALLAX_MOTOR) != 1
#error "Select exactly one target motor by setting its macro to 1."
#endif

// -----------------------------
// Include Motor Headers
// -----------------------------
#if DS_MOTOR
#include "ds_motor.h"
#endif
#if DS_CONTINUOUS_MOTOR
#include "ds_continuous_motor.h"
#endif
#if PARALLAX_MOTOR
#include "parallax_motor.h"
#endif
#if TOWER_PRO_MOTOR
#include "tower_pro_motor.h"
#endif

// -----------------------------
// Feature flags
// -----------------------------
#define DEBUG                 (0U)

// -----------------------------
// Serial helpers
// -----------------------------
static void initSerial() {
  Serial.begin(9600);
  delay(1000);
  while (!Serial) {}
}

static void waitForUser() {
  enqueuePrint("Type 'y' then press Enter to start:\n");
  String input;
  while (true) {
    if (Serial.available()) {
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.equalsIgnoreCase("y")) {
        enqueuePrint("Starting main loop...\n");
        return;
      } else {
        enqueuePrint("Waiting for 'y'...\n");
      }
    }
  }
}

static void blinkAtBoot(uint8_t pin) {
  digitalWrite(pin, LOW);  delay(250);
  digitalWrite(pin, HIGH); delay(250);
  digitalWrite(pin, LOW);  delay(250);
  digitalWrite(pin, HIGH);
}

// -----------------------------
// Tokenize up to three space-separated tokens
// -----------------------------
static void tokenize3(const String& s, String& t0, String& t1, String& t2) {
  int s1 = s.indexOf(' ');
  if (s1 < 0) { t0 = s; t1=""; t2=""; return; }
  t0 = s.substring(0, s1);
  int s2 = s.indexOf(' ', s1 + 1);
  if (s2 < 0) { t1 = s.substring(s1 + 1); t2=""; return; }
  t1 = s.substring(s1 + 1, s2);
  t2 = s.substring(s2 + 1);
}

// -----------------------------
// Arduino setup/loop
// -----------------------------
void setup() {
  initSerial();
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUT1_PIN, OUTPUT);
#if DS_MOTOR
  pinMode(OUT2_PIN, OUTPUT);
#endif
  blinkAtBoot(LED_PIN);

#if TOWER_PRO_MOTOR
  towerProInit();
#endif

  // side effect: spin up Wifi task (and Print task if DEBUG flag set)
  // note: must be called before while(!Serial)
  startSlave();

  // BLOCKING!!
  waitForUser();
}

void loop() {
  static int num = 90;             // angle or bounce cycles
  static int dir = 0;              // -1 left, 0 stop, +1 right (continuous)
  static bool bounceFlag = false;
  static uint8_t dsActivePin = OUT1_PIN; // default target for DS_MOTOR

#if DS_MOTOR
  // Input formats:
  // - "open [1|2]"
  // - "close [1|2]"
  // - "b <pause_ms> [1|2]"
  // - "<angle 0..180> [1|2]"
  enqueuePrint("DS: enter 'open [1|2]', 'close [1|2]', 'b <pause_ms> [1|2]', or '<angle> [1|2]':\n");
  while (true) {
    // Refresh last commanded angle for active pin if the last cmd was numeric
    driveDsServoAngle(dsActivePin, num);

    if (Serial.available()) {
      String input = Serial.readStringUntil('\n'); input.trim();
      if (input.length() == 0) break;

      String t0, t1, t2; tokenize3(input, t0, t1, t2);

      // Optional per-command servo selector
      auto parseServo = [](const String& tok)->int {
        if (tok == "1") return 1;
        if (tok == "2") return 2;
        return -1;
      };
      int sel = parseServo(t2); if (sel < 0) sel = parseServo(t1);
      uint8_t targetPin = (sel == 2) ? OUT2_PIN : OUT1_PIN;
      dsActivePin = targetPin; // remember for refresh

      // Commands
      if (t0.equalsIgnoreCase("open")) {
        dsHoldAngle(targetPin, dsOpenAngleForPin(targetPin), DS_BOUNCE_MOVE_MS);
      } else if (t0.equalsIgnoreCase("close")) {
        dsHoldAngle(targetPin, dsClosedAngleForPin(targetPin), DS_BOUNCE_MOVE_MS);
      } else if (t0.equalsIgnoreCase("b") || t0.equalsIgnoreCase("bounce")) {
        uint16_t pauseMs = 1000;
        if (t1.length() && isDigit(t1.charAt(0))) {
          pauseMs = (uint16_t)MAX(0, (int)(t1.toInt()));
        }
        dsBounce(targetPin, pauseMs);
      } else {
        // numeric angle in t0
        if (isDigit(t0.charAt(0))) {
          num = constrain(t0.toInt(), 0, 180);
          dsHoldAngle(targetPin, num, DS_BOUNCE_MOVE_MS);
        }
      }
      break;
    }
  }

#elif TOWER_PRO_MOTOR
  // Input formats:
  // - "<angle 0..180>"
  // - "b <duration_ms> <pause_ms>"
  enqueuePrint("TP: enter '<angle 0..180>' or 'b <duration_ms> <pause_ms>':\n");
  while (true) {
    // Keep last angle commanded
    MG995_Servo.write(constrain(num, 0, 180));

    if (Serial.available()) {
      String input = Serial.readStringUntil('\n'); input.trim();
      if (input.length() == 0) break;

      String t0, t1, t2; tokenize3(input, t0, t1, t2);

      if (t0.equalsIgnoreCase("b") || t0.equalsIgnoreCase("bounce")) {
        uint16_t durationMs = 500;
        uint16_t pauseMs    = 1000;
        if (t1.length() && isDigit(t1.charAt(0))) durationMs = (uint16_t)x(0, t1.toInt());
        if (t2.length() && isDigit(t2.charAt(0))) pauseMs    = (uint16_t)MAX(0, (int)(t2.toInt()));
        towerProBounce(durationMs, pauseMs);
      } else if (isDigit(t0.charAt(0))) {
        num = constrain(t0.toInt(), 0, 180);
        MG995_Servo.write(num);
      }
      break;
    }
  }

#elif DS_CONTINUOUS_MOTOR
  enqueuePrint("CR: enter 'r' (right), 'l' (left), 's' (stop), or a number for bounce cycles:\n");
  while (true) {
    if (dir < 0)      contServoLeft(OUT1_PIN);
    else if (dir > 0) contServoRight(OUT1_PIN);
    else if (bounceFlag) {
      contServoBounce(OUT1_PIN, num);
      dir = 0; bounceFlag = false;
    } else           contServoHalt(OUT1_PIN);

    if (Serial.available()) {
      String input = Serial.readStringUntil('\n'); input.trim();
      if (input.length() == 0) break;

      char mode = input.charAt(0);
      switch (mode) {
        case 'r': enqueuePrint("Right\n"); dir = 1;  bounceFlag = false; break;
        case 'l': enqueuePrint("Left\n");  dir = -1; bounceFlag = false; break;
        case 's': enqueuePrint("Stop\n");  dir = 0;  bounceFlag = false; break;
        default:
          if (isDigit(mode)) { num = MAX(0, (int)(input.toInt())); bounceFlag = true; }
          break;
      }
      break;
    }
  }
#endif // DS_MOTOR

#if PARALLAX_MOTOR
  enqueuePrint("CR: enter 'r' (right), 'l' (left), 's' (stop), or a number for bounce cycles:\n");
  while (true) {
    if (dir < 0)      parallaxServoLeft(OUT1_PIN);
    else if (dir > 0) parallaxServoRight(OUT1_PIN);
    else if (bounceFlag) {
      parallaxServoBounce(OUT1_PIN, num);
      dir = 0; bounceFlag = false;
    } else           parallaxServoHalt(OUT1_PIN);

    if (Serial.available()) {
      String input = Serial.readStringUntil('\n'); input.trim();
      if (input.length() == 0) break;

      char mode = input.charAt(0);
      switch (mode) {
        case 'r': enqueuePrint("Right\n"); dir = 1;  bounceFlag = false; break;
        case 'l': enqueuePrint("Left\n");  dir = -1; bounceFlag = false; break;
        case 's': enqueuePrint("Stop\n");  dir = 0;  bounceFlag = false; break;
        default:
          if (isDigit(mode)) { num = MAX(0, (int)(input.toInt())); bounceFlag = true; }
          break;
      }
      break;
    }
  }
#endif // PARALLAX_MOTOR
  // TODO: add vTaskDelay()
}
