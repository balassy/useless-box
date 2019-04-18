// Platform libraries.
#include <Arduino.h>  // To add IntelliSense for platform constants.

// Third-party libraries.

// My classes.
#include "speed-servo.h"
#include "status-led.h"

#include "config.h"  // To store configuration and secrets.

SpeedServo lidServo;
SpeedServo switchServo;
StatusLed led;

int lastSwitchState = 0;

void setup() {
  initSerial();
  initServos();
  initLed();
  pinMode(PIN_SWITCH, INPUT);

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
  lidServo.moveFastTo(LID_START_POSITION);

  switchServo.attach(PIN_SWITCH_SERVO);
  switchServo.moveFastTo(SWITCH_START_POSITION);
}

void initLed() {
  led.setPin(LED_BUILTIN);
  led.turnOff();
}

void loop() {
  int switchState = digitalRead(PIN_SWITCH);
  boolean isSwitchTurnedOn = (switchState != lastSwitchState) && (switchState == HIGH);

  if (isSwitchTurnedOn) {
    openLid();
    flipSwitch();
    closeLid();
  }

  lastSwitchState = switchState;
}

void openLid() {
  led.turnOn();
  lidServo.moveSlowTo(LID_END_POSITION);
}

void closeLid() {
  lidServo.moveSlowTo(LID_START_POSITION);
  led.turnOff();
}

void flipSwitch() {
  switchServo.moveSlowTo(SWITCH_END_POSITION);
  switchServo.moveSlowTo(SWITCH_START_POSITION);
}
