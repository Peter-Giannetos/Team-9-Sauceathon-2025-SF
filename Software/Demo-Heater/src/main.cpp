#include <Arduino.h>

#define LED_PIN (2)
#define OUT_PIN (19)

bool ready = false;

unsigned long previousMillisLED = 0;
unsigned long previousMillisHello = 0;
const long ledInterval = 1000;      // 1 second
const long helloInterval = 10000;   // 10 seconds

bool ledState = LOW;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Type 'GO' then press Enter to start:");

  String input;
  while (!ready) {
    if (Serial.available()) {
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.equalsIgnoreCase("GO")) {
        ready = true;
        Serial.println("Starting main loop...");
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

  // Check serial commands to control OUT_PIN without blocking
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("ON")) {
      digitalWrite(OUT_PIN, HIGH);
      Serial.println("OUT_PIN turned ON");
    } else if (cmd.equalsIgnoreCase("OFF")) {
      digitalWrite(OUT_PIN, LOW);
      Serial.println("OUT_PIN turned OFF");
    } else {
      Serial.println("Unknown command. Use ON or OFF.");
    }
  }
}
