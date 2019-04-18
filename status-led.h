#include <Arduino.h>

#ifndef status_led_h
#define status_led_h

class StatusLed {
 public:
  void setPin(uint8_t pin);
  void turnOn();
  void turnOff();

 private:
  uint8_t _pin;
};

#endif /* status_led_h */
