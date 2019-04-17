#include <Arduino.h>           // To add IntelliSense for platform constants.
#include <Servo.h>             // To control the SG90 servo motors.

#ifndef speed_servo_h
#define speed_servo_h

class SpeedServo {
  public:
    void attach(uint8_t pin);
    void moveFastTo(int position);
    void moveSlowTo(int position);

  private:
    Servo _servo;
    int _lastPosition;
};

#endif /* speed_servo_h */