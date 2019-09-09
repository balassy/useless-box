const char* APP_VERSION = "0.0.1";

#ifndef LED_BUILTIN
#define LED_BUILTIN D4
#endif

const int PIN_LID_SERVO = D8;     // The number of the GPIO pin where the lid opener servo is connected.
const int PIN_SWITCH_SERVO = D0;  // The number of the GPIO pin where the switch manipulator servo is connected.
const int PIN_SWITCH = D5;        // The number of the GPIO pin where the switch is connected.
const int PIN_SENSOR_SDA = D2;
const int PIN_SENSOR_SCL = D1;

const int LID_START_POSITION = 90;
const int LID_END_POSITION = 40;
const int SWITCH_START_POSITION = 0;
const int SWITCH_END_POSITION = 145;

const int SWITCH_HALF_POSITION = 90;
