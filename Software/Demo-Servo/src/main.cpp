#include <Arduino.h>

#define LED_PIN (2)
#define OUT_PIN (13)

void init_serial() {
  Serial.begin(9600);
  delay(1000);
  while(!Serial);
}

void toggle_led(int pin) {
  digitalWrite(pin, LOW);
  delay(1000);
  digitalWrite(pin, HIGH);
  delay(1000);
}

void drive_parallax_servo_clockwise(int pin) {
  // assume pin is LOW at start of function
  delay(20);
  digitalWrite(pin, HIGH);
  delayMicroseconds(1300);
  digitalWrite(pin, LOW);
}

void drive_parallax_servo_counterwise(int pin) {
  // assume pin is LOW at start of function
  delay(20);
  digitalWrite(pin, HIGH);
  delayMicroseconds(1700);
  digitalWrite(pin, LOW);
}

void setup() {
  // put your setup code here, to run once:

  // Setup Serial
  init_serial();

  // Setup Pins
  pinMode(LED_PIN, OUTPUT); // D2
  pinMode(OUT_PIN, OUTPUT); // D13
}

void loop() {
  // put your main code here, to run repeatedly:
  toggle_led(LED_PIN);
  drive_parallax_servo_clockwise(OUT_PIN);
  delay(1000);
  drive_parallax_servo_counterwise(OUT_PIN);
  delay(1000);

  Serial.println("Hello!");
}
