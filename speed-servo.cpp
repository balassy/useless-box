#include "speed-servo.h"

const int SLOW_MOVE_DELAY = 15;
const int Fast_MOVE_DELAY = 2;

void SpeedServo::attach(uint8_t pin) {
  _servo.attach(pin);
}

// Valid position: 0-180.
void SpeedServo::moveNowTo(int newPosition) {
  _lastPosition = newPosition;
  _servo.write(newPosition);
}

// Valid position: 0-180.
void SpeedServo::moveFastTo(int newPosition) {
  _moveTo(newPosition, Fast_MOVE_DELAY);
}

// Valid position: 0-180.
void SpeedServo::moveSlowTo(int newPosition) {
  _moveTo(newPosition, SLOW_MOVE_DELAY);
}

// Valid position: 0-180.
void SpeedServo::_moveTo(int newPosition, unsigned long stepDelay) {
  if(newPosition > _lastPosition) {
    for (int pos = _lastPosition; pos <= newPosition; pos++) {
      _servo.write(pos);
      delay(stepDelay);
    }
  } else {
    for (int pos = _lastPosition; pos >= newPosition; pos--) {
      _servo.write(pos);
      delay(stepDelay);
    }
  }

  _lastPosition = newPosition;
}
