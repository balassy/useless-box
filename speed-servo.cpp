#include "speed-servo.h"

void SpeedServo::attach(uint8_t pin) {
	_servo.attach(pin);
}

// Valid position: 0-180.
void SpeedServo::moveTo(int newPosition, unsigned long stepDelay) {
	if(stepDelay == 0) {
		_lastPosition = newPosition;
		_servo.write(newPosition);
	} else if(newPosition > _lastPosition) {
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
