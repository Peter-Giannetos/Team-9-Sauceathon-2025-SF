#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN         2
#define OUT_PIN         19
#define NEOPIXEL_PIN    14
#define NUMPIXELS       16
#define SOUND_PIN       34  // Analog input pin for sound sensor

Adafruit_NeoPixel strip(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

bool ready = false;
bool ledState = LOW;

unsigned long previousMillisLED = 0;
unsigned long previousMillisHello = 0;
unsigned long previousMillisSound = 0;

const long ledInterval = 1000;
const long helloInterval = 10000;
const long soundInterval = 20;  // More frequent updates for faster reaction

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
  pinMode(SOUND_PIN, INPUT);

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
  float progress = (float)step / (NUMPIXELS - 1);
  float globalBrightness = pow(progress, 2.2) * 0.9 + 0.1;

  for (int i = 0; i < NUMPIXELS; i++) {
    float ratio = (float)i / (NUMPIXELS - 1);
    uint32_t color;

    if (ratio < 0.15) {
      float t = ratio / 0.15;
      color = interpolateColor(0, 255, 0, 255, 255, 0, t);
    } else if (ratio < 0.35) {
      float t = (ratio - 0.15) / 0.20;
      color = interpolateColor(255, 255, 0, 255, 120, 0, t);
    } else {
      float t = (ratio - 0.35) / 0.65;
      color = interpolateColor(255, 120, 0, 255, 0, 0, t);
    }

    uint8_t r = ((color >> 16) & 0xFF) * globalBrightness;
    uint8_t g = ((color >> 8) & 0xFF) * globalBrightness;
    uint8_t b = (color & 0xFF) * globalBrightness;

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

  if (currentMillis - previousMillisSound >= soundInterval) {
    previousMillisSound = currentMillis;

    int soundValue = analogRead(SOUND_PIN);

    // Faster smoothing for spikes
    static float smoothValue = 0;
    float alpha = 0.15;
    smoothValue = smoothValue * (1 - alpha) + soundValue * alpha;

    // Envelope detector with dynamic decay based on activity
    static float level = 0;
    float decayRate = 0.92; // base decay (faster when quiet)

    if (smoothValue > 1500) {
      decayRate = 0.97; // slow decay for loud sounds
    } else if (smoothValue > 800) {
      decayRate = 0.95; // medium decay
    } // else keep base decay

    if (smoothValue > level) {
      // Fast rise for responsiveness
      level = smoothValue + (smoothValue - level) * 0.5;
    } else {
      // Dynamic decay
      level *= decayRate;
    }

    if (level < 70) level = 70;

    float amplified = (level - 500) * 3.2;
    int amplifiedValue = constrain(amplified, 0, 4095);
    int step = map(amplifiedValue, 0, 4095, 0, NUMPIXELS - 1);
    step = constrain(step, 0, NUMPIXELS - 1);

    displayEnhancedBrightnessGradient(step);
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
