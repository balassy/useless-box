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
long playCount = 0;

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
 lidServo.moveNowTo(LID_START_POSITION);

  switchServo.attach(PIN_SWITCH_SERVO);
  switchServo.moveNowTo(SWITCH_START_POSITION);
}

void initLed() {
  led.setPin(LED_BUILTIN);
  led.turnOff();
}

void loop() {
  int switchState = digitalRead(PIN_SWITCH);
  Serial.println(switchState);
  boolean isSwitchTurnedOn = (switchState != lastSwitchState) && (switchState == LOW);

  if (isSwitchTurnedOn) {
    led.turnOn();
    run();
    led.turnOff();
  }

  lastSwitchState = switchState;
}

void run() {
  switch (playCount % 6)
  {
    case 0:
    case 1:
      runSlow();
      break;
    case 2:
      runFast();
      break;
    case 3:
      runFastWithDelay();
      break;
    case 4:
      runClap();
      break;
    case 5:
      runHalf();
      break;
    default:
      break;
  }

  playCount++;
}

void runSlow() {
  openLidSlow();
  flipSwitchSlow();
  closeLidSlow();
}

void runFast() {
  flipSwitchFast();
}

void runFastWithDelay() {
  openLidSlow();
  delay(4000);
  flipSwitchFast();
  closeLidFast();
}

void runClap() {
  clapLid();
  clapLid();
  clapLid();
  clapLid();
  openLidFast();
  flipSwitchFast();
  closeLidFast();
}

void runHalf() {
  switchServo.moveSlowTo(SWITCH_HALF_POSITION);
  delay(3000);
  switchServo.moveFastTo(SWITCH_END_POSITION);
  switchServo.moveFastTo(SWITCH_START_POSITION);
}

void openLidSlow() {
  lidServo.moveSlowTo(LID_END_POSITION);
}

void openLidFast() {
  lidServo.moveFastTo(LID_END_POSITION);
}

void closeLidSlow() {
  lidServo.moveSlowTo(LID_START_POSITION);
}

void closeLidFast() {
  lidServo.moveFastTo(LID_START_POSITION);
}

void clapLid() {
  openLidFast();
  closeLidFast();
}

void flipSwitchSlow() {
  switchServo.moveSlowTo(SWITCH_END_POSITION);
  switchServo.moveSlowTo(SWITCH_START_POSITION);
}

void flipSwitchFast() {
  switchServo.moveFastTo(SWITCH_END_POSITION);
  switchServo.moveFastTo(SWITCH_START_POSITION);
}
