#include <Arduino.h>

#define DebugPort Serial1

void setup() {
  Serial.begin(115200);
  Serial.println("INFO: Booting JASPER SX1276 Board (Rev2) with PIO Firmware");

  // LEDs in output mode
  pinMode(PC6, OUTPUT);
  pinMode(PA11, OUTPUT);

}

void loop() {
  Serial.println("INFO: Starting loop() XXX;");

  // write PC6 and PA11 to blink
  digitalWrite(PC6, HIGH);
  digitalWrite(PA11, HIGH);

  delay(500);

  digitalWrite(PC6, LOW);
  digitalWrite(PA11, LOW);
  delay(1000);

}
