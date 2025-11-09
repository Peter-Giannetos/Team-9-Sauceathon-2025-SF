#include <Arduino.h>
#include <slave_config.h>
#include "flipper.h"
  
void setup() {
  Serial.begin(115200);
  delay(500);
  // enqueuePrint("TEST");
  setupFlipperTask();
  startSlave();

  xTaskCreatePinnedToCore(FlipperTask, "Flipper Task", 4096, NULL, 1, NULL, 0);
}

void loop() {
  vTaskDelay(10 / portTICK_PERIOD_MS);
}
