// Platform libraries.
#include <Arduino.h>  // To add IntelliSense for platform constants.

// Third-party libraries.

// My classes.
#include "speed-servo.h"
#include "status-led.h"
#include "proximity-sensor.h"

#include "config.h"  // To store configuration and secrets.

SpeedServo lidServo;
SpeedServo switchServo;
StatusLed led;
ProximitySensor sensor;

int lastSwitchState = 0;
long playCount = 0;
bool isLidOpen = false;

void setup() {
  initSerial();
  initServos();
  initLed();
  initSensor();
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

void initSensor() {
  sensor.attach(PIN_SENSOR_SDA, PIN_SENSOR_SCL, SENSOR_TRIGGER_THRESHOLD);
}

void loop() {
  int switchState = digitalRead(PIN_SWITCH);
  boolean isSwitchTurnedOn = (switchState != lastSwitchState) && (switchState == LOW);

  if (isSwitchTurnedOn) {
    led.turnOn();
    run();
    led.turnOff();
  }

  lastSwitchState = switchState;

  // Check the proximity sensor.
  if (sensor.isInRange()) {
    if (!isLidOpen) {
      openLidFast();
      isLidOpen = true;
    }
  } else {
    if (isLidOpen) {
      closeLidFast();
      isLidOpen = false;
    }
  }

  // Wait 250 ms before next reading
  delay(250);
}

void run() {
  switch (playCount % 10) {
    case 0:
    case 1:
      runSlow();
      break;
    case 2:
      runWaitThenFast();
      break;
    case 3:
      runFast();
      break;
    case 4:
      runFastThenClap();
      break;
    case 5:
      runOpenCloseThenFast();
      break;
    case 6:
      runPeekThenFast();
      break;
    case 7:
      runFastWithDelay();
      break;
    case 8:
      runClap();
      break;
    case 9:
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

void runWaitThenFast() {
  delay(5000);
  flipSwitchFast();
}

void runFast() {
  flipSwitchFast();
}

void runFastThenClap() {
  flipSwitchFast();
  clapLid();
}

void runOpenCloseThenFast() {
  openLidSlow();
  delay(2000);
  closeLidSlow();
  delay(2000);
  flipSwitchFast();
}

void runPeekThenFast() {
  switchServo.moveSlowTo(SWITCH_HALF_POSITION);
  delay(3000);
  switchServo.moveFastTo(SWITCH_START_POSITION);
  delay(3000);
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
