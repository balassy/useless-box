// Platform libraries.
#include <Arduino.h>           // To add IntelliSense for platform constants.

// Third-party libraries.

// My classes.

#include "config.h"  // To store configuration and secrets.

void setup() {
  initSerial();

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.printf("Application version: %s\n", APP_VERSION);
  Serial.println("Setup completed.");
}

void initSerial() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Initializing serial connection DONE.");
}

void loop() {
  // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, HIGH);

  // wait for a second
  delay(1000);

  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);

   // wait for a second
  delay(1000);
}

