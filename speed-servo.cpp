#include "speed-servo.h"

const int SLOW_MOVE_DELAY = 15;

void SpeedServo::attach(uint8_t pin) {
  _servo.attach(pin);
}

// Valid position: 0-180.
void SpeedServo::moveFastTo(int newPosition) {
  _lastPosition = newPosition;
  _servo.write(newPosition);
}

// Valid position: 0-180.
void SpeedServo::moveSlowTo(int newPosition) {
  if(newPosition > _lastPosition) {
    for (int pos = _lastPosition; pos <= newPosition; pos++) {
      _servo.write(pos);
      delay(SLOW_MOVE_DELAY);
    }
  } else {
    for (int pos = _lastPosition; pos >= newPosition; pos--) {
      _servo.write(pos);
      delay(SLOW_MOVE_DELAY);
    }
  }

  _lastPosition = newPosition;
}
