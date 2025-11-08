#include <Adafruit_NeoPixel.h>

#define PIN            6  // Pin where NeoPixel is connected
#define NUMPIXELS      8  // Number of NeoPixels

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.show();  // Initialize all pixels to 'off'
}

void loop() {
  for(int i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0)); // Red color
    strip.show();
    delay(50);
    strip.setPixelColor(i, strip.Color(0, 0, 0)); // Turn off the pixel
  }
}