// Standalone sketch

#include <Arduino.h>

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
// Feature flags
// -----------------------------
#define DEBUG                 (0U)

// -----------------------------
// Pins
// -----------------------------
constexpr uint8_t LED_PIN  = 2;
constexpr uint8_t OUT1_PIN = 13;   // Primary servo output
constexpr uint8_t OUT2_PIN = 14;   // Secondary (DS dual-servo use)

// -----------------------------
// Servo timing constants
// -----------------------------
constexpr uint16_t SERVO_FRAME_MS   = 20;    // Typical RC servo frame
constexpr uint16_t SERVO_NEUTRAL_US = 1500;  // Neutral for continuous
constexpr uint16_t SERVO_MIN_US     = 500;   // Positional min pulse
constexpr uint16_t SERVO_MAX_US     = 2500;  // Positional max pulse

// -----------------------------
// DS positional presets (per pin)
// -----------------------------
#if DS_MOTOR
constexpr uint8_t TOP_BUN_CLOSED     = 170;  // OUT1_PIN
constexpr uint8_t TOP_BUN_OPEN       = 80;   // OUT1_PIN
constexpr uint8_t BOTTOM_BUN_CLOSED  = 90;   // OUT2_PIN
constexpr uint8_t BOTTOM_BUN_OPEN    = 0;    // OUT2_PIN
constexpr uint16_t DS_BOUNCE_MOVE_MS = 500;  // time to hold each target
#endif

// -----------------------------
// TowerPro MG995 (Servo lib)
// -----------------------------
#if TOWER_PRO_MOTOR
#include <Servo.h>
Servo MG995_Servo;
constexpr uint8_t TOWER_PRO_FORWARD   = 0;
constexpr uint8_t TOWER_PRO_BACKWARD  = 180;
constexpr uint8_t TOWER_PRO_HALT      = 90;
#endif

// -----------------------------
// Serial helpers
// -----------------------------
static void initSerial() {
  Serial.begin(9600);
  delay(1000);
  while (!Serial) {}
}

static void waitForUser() {
  Serial.println("Type 'y' then press Enter to start:");
  String input;
  while (true) {
    if (Serial.available()) {
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.equalsIgnoreCase("y")) {
        Serial.println("Starting main loop...");
        return;
      } else {
        Serial.println("Waiting for 'y'...");
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
// Generic single-frame pulse (manual PWM)
// -----------------------------
static inline void servoOneFrameWriteUs(uint8_t pin, uint16_t highUs) {
  if (highUs < 400)  highUs = 400;
  if (highUs > 2700) highUs = 2700;
  delay(SERVO_FRAME_MS);
  digitalWrite(pin, HIGH);
  delayMicroseconds(highUs);
  digitalWrite(pin, LOW);
}

// -----------------------------
// DS positional (manual pulse)
// -----------------------------
#if DS_MOTOR
static inline uint16_t angleToUs(uint8_t deg) {
  return static_cast<uint16_t>(SERVO_MIN_US + ((static_cast<uint32_t>(deg) * 2000U) / 180U));
}

static void driveDsServoAngle(uint8_t pin, int angleDeg) {
  angleDeg = constrain(angleDeg, 0, 180);
  uint16_t us = angleToUs(static_cast<uint8_t>(angleDeg));
  if (DEBUG) { Serial.print("DS angle="); Serial.print(angleDeg); Serial.print(" us="); Serial.println(us); }
  servoOneFrameWriteUs(pin, us);
}

// Hold a position for holdMs by refreshing every frame
static void dsHoldAngle(uint8_t pin, int angleDeg, uint16_t holdMs) {
  uint32_t start = millis();
  while (millis() - start < holdMs) {
    driveDsServoAngle(pin, angleDeg);
  }
}

static inline uint8_t dsClosedAngleForPin(uint8_t pin) {
  return (pin == OUT2_PIN) ? BOTTOM_BUN_CLOSED : TOP_BUN_CLOSED;
}
static inline uint8_t dsOpenAngleForPin(uint8_t pin) {
  return (pin == OUT2_PIN) ? BOTTOM_BUN_OPEN : TOP_BUN_OPEN;
}

// CLOSED->OPEN, pause, OPEN->CLOSED
static void dsBounce(uint8_t pin, uint16_t pauseMs) {
  uint8_t openA   = dsOpenAngleForPin(pin);
  uint8_t closedA = dsClosedAngleForPin(pin);
  dsHoldAngle(pin, openA,   DS_BOUNCE_MOVE_MS);
  delay(pauseMs);
  dsHoldAngle(pin, closedA, DS_BOUNCE_MOVE_MS);
}
#endif

// -----------------------------
// Continuous (manual pulse)
// -----------------------------
#if DS_CONTINUOUS_MOTOR || PARALLAX_MOTOR
static void contServoHalt(uint8_t pin)  { servoOneFrameWriteUs(pin, SERVO_NEUTRAL_US); }
static void contServoRight(uint8_t pin) { servoOneFrameWriteUs(pin, 1300); }
static void contServoLeft(uint8_t pin)  { servoOneFrameWriteUs(pin, 1700); }
static void contServoBounce(uint8_t pin, int cycles) {
  if (cycles < 0) cycles = 0;
  for (int i = 0; i < cycles; ++i) contServoRight(pin);
  for (int i = 0; i < cycles; ++i) contServoLeft(pin);
}
#endif

// -----------------------------
// TowerPro MG995 (Servo lib)
// -----------------------------
#if TOWER_PRO_MOTOR
static void towerProClockwise()    { MG995_Servo.write(TOWER_PRO_FORWARD); }
static void towerProCounterwise()  { MG995_Servo.write(TOWER_PRO_BACKWARD); }
static void towerProHalt()         { MG995_Servo.write(TOWER_PRO_HALT); }
static void towerProBounce(uint16_t durationMs, uint16_t pauseMs) {
  towerProClockwise();  delay(durationMs);
  towerProHalt();       delay(pauseMs);
  towerProCounterwise();delay(durationMs);
  towerProHalt();
}
#endif

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
  MG995_Servo.attach(OUT1_PIN);
#endif

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
  Serial.println("DS: enter 'open [1|2]', 'close [1|2]', 'b <pause_ms> [1|2]', or '<angle> [1|2]':");
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
  Serial.println("TP: enter '<angle 0..180>' or 'b <duration_ms> <pause_ms>':");
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

#elif DS_CONTINUOUS_MOTOR || PARALLAX_MOTOR
  Serial.println("CR: enter 'r' (right), 'l' (left), 's' (stop), or a number for bounce cycles:");
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
        case 'r': Serial.println("Right"); dir = 1;  bounceFlag = false; break;
        case 'l': Serial.println("Left");  dir = -1; bounceFlag = false; break;
        case 's': Serial.println("Stop");  dir = 0;  bounceFlag = false; break;
        default:
          if (isDigit(mode)) { num = MAX(0, (int)(input.toInt())); bounceFlag = true; }
          break;
      }
      break;
    }
  }
#endif
}
