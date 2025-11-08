#include <Arduino.h>

#define LED_PIN (2)
#define OUT_PIN (19)



void setup() {
  // put your setup code here, to run once:
  pinMode(LED_PIN, OUTPUT); // D2
  Serial.begin(9600);
  delay(1000);
  while(!Serial)
  {
    delay(1);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_PIN, LOW);
  digitalWrite(OUT_PIN, LOW);
  delay(1000);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(OUT_PIN, HIGH);

  delay(1000);
  Serial.println("Hello!");





}
