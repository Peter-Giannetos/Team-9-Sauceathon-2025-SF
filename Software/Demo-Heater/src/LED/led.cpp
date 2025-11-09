#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN         2
#define OUT_PIN         19
#define NEOPIXEL_PIN    14
#define NUMPIXELS       16

Adafruit_NeoPixel strip(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

bool ready = false;
bool ledState = LOW;

unsigned long previousMillisLED = 0;
unsigned long previousMillisHello = 0;
unsigned long previousMillisPixel = 0;

const long ledInterval = 1000;
const long helloInterval = 10000;
const long pixelInterval = 100;

int currentPixel = 0;

uint32_t interpolateColor(uint8_t r1, uint8_t g1, uint8_t b1,
                          uint8_t r2, uint8_t g2, uint8_t b2,
                          float t) {
  uint8_t r = r1 + (r2 - r1) * t;
  uint8_t g = g1 + (g2 - g1) * t;
  uint8_t b = b1 + (b2 - b1) * t;
  return strip.Color(r, g, b);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);

  Serial.begin(115200);
  Serial.println("Type 'GO' then press Enter to start:");

  strip.begin();
  strip.show();

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

void displayEnhancedBrightnessGradient(int step) {
  // Steeper brightness curve for higher contrast
  float progress = (float)step / (NUMPIXELS - 1);
  float globalBrightness = pow(progress, 2.2) * 0.9 + 0.1; // steeper exponential curve

  for (int i = 0; i < NUMPIXELS; i++) {
    float ratio = (float)i / (NUMPIXELS - 1);
    uint32_t color;

    // Adjusted color segments for longer red tail
    if (ratio < 0.15) {
      // Green to Yellow
      float t = ratio / 0.15;
      color = interpolateColor(0, 255, 0, 255, 255, 0, t);
    } else if (ratio < 0.35) {
      // Yellow to Orange
      float t = (ratio - 0.15) / 0.20;
      color = interpolateColor(255, 255, 0, 255, 120, 0, t);
    } else {
      // Extend red section much more
      float t = (ratio - 0.35) / 0.65;
      color = interpolateColor(255, 120, 0, 255, 0, 0, t);
    }

    uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * globalBrightness);
    uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * globalBrightness);
    uint8_t b = (uint8_t)((color & 0xFF) * globalBrightness);

    if (i <= step)
      strip.setPixelColor(i, r, g, b);
    else
      strip.setPixelColor(i, 0, 0, 0);
  }

  strip.show();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillisLED >= ledInterval) {
    previousMillisLED = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }

  if (currentMillis - previousMillisHello >= helloInterval) {
    previousMillisHello = currentMillis;
    Serial.print("Hello! Time since boot: ");
    Serial.print(currentMillis);
    Serial.println(" ms");
  }

  if (currentMillis - previousMillisPixel >= pixelInterval) {
    previousMillisPixel = currentMillis;
    displayEnhancedBrightnessGradient(currentPixel);
    currentPixel++;
    if (currentPixel >= NUMPIXELS) currentPixel = 0;
  }

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
