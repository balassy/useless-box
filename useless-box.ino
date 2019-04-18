// Platform libraries.
#include <Arduino.h>           // To add IntelliSense for platform constants.

// Third-party libraries.

// My classes.
#include "speed-servo.h"

#include "config.h"  // To store configuration and secrets.

SpeedServo lidServo;
SpeedServo switchServo;

void setup() {
  initSerial();
  initServos();

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.printf("Application version: %s\n", APP_VERSION);
  Serial.println("Setup completed.");
}

void initSerial() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Initializing serial connection DONE.");
}

void initServos() {
  lidServo.attach(PIN_LID_SERVO);
  switchServo.attach(PIN_SWITCH_SERVO);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);

  lidServo.moveSlowTo(0);
  switchServo.moveSlowTo(0);

  digitalWrite(LED_BUILTIN, LOW);
  delay(2000);

  lidServo.moveSlowTo(180);
  switchServo.moveSlowTo(180);

  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);

  lidServo.moveSlowTo(90);
  switchServo.moveSlowTo(90);
  digitalWrite(LED_BUILTIN, LOW);
  delay(2000);
}

