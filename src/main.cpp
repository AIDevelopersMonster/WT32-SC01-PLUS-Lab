#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("WT32-SC01-PLUS-Lab bootstrap");
  Serial.println("Stage: HW-01 identity probe pending");
}

void loop() {
  delay(1000);
}
