#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  pinMode(4, INPUT); // D2
  Serial.begin(9600);
  while(!Serial);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(18, LOW);
  delay(1000);
  digitalWrite(18, HIGH);
  delay(1000);
  Serial.println("Hello!");
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}