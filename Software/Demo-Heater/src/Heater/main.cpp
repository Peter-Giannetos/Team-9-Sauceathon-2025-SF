#include <Arduino.h>

#define LED_PIN (2)
#define OUT_PIN (19)
#define PWM_PIN (12)

bool ready = false;

unsigned long previousMillisLED = 0;
unsigned long previousMillisHello = 0;
const long ledInterval = 1000;      // 1 second
const long helloInterval = 10000;   // 10 seconds

bool ledState = LOW;

const int pwmFreq = 1000;    // 1 kHz
const int pwmChannel = 0;
const int pwmResolution = 8; // 8-bit: 0-255
int pwmDutyCycle = 0;      // default 50%

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  Serial.begin(115200);

  // Setup PWM
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(PWM_PIN, pwmChannel);
  ledcWrite(pwmChannel, pwmDutyCycle);

  Serial.println("Type 'GO' then press Enter to start:");

  String input;
  while (!ready) {
    if (Serial.available()) {
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.equalsIgnoreCase("GO")) {
        ready = true;
        Serial.println("Starting main loop...");
        Serial.println("You can enter 'ON', 'OFF', or a PWM value (0–100).");
      } else {
        Serial.println("Waiting for 'GO'...");
      }
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Non-blocking LED blink every 1 second
  if (currentMillis - previousMillisLED >= ledInterval) {
    previousMillisLED = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }

  // Print "Hello" every 10 seconds
  if (currentMillis - previousMillisHello >= helloInterval) {
    previousMillisHello = currentMillis;
    Serial.print("Hello! Time since boot: ");
    Serial.print(currentMillis);
    Serial.println(" ms");
  }

  // Serial input for commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("ON")) {
      digitalWrite(OUT_PIN, HIGH);
      Serial.println("OUT_PIN turned ON");
    } else if (cmd.equalsIgnoreCase("OFF")) {
      digitalWrite(OUT_PIN, LOW);
      Serial.println("OUT_PIN turned OFF");


    } else if (cmd.toInt() >= 0 && cmd.toInt() <= 100) {
      int userValue = cmd.toInt();
      pwmDutyCycle = map(userValue, 0, 100, 0, 255);
      ledcWrite(pwmChannel, pwmDutyCycle);
      Serial.print("PWM duty cycle set to ");
      Serial.print(userValue);
      Serial.println("%");
    } else {
      Serial.println("Unknown command. Use ON, OFF, or a number (0–100).");
    }
  }
}
