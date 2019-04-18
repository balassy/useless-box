const char* APP_VERSION = "0.0.1";

#ifndef LED_BUILTIN
#define LED_BUILTIN 16
#endif

const int PIN_LID_SERVO = 4;     // The number of the GPIO pin where the lid opener servo is connected. (GPIO4 = D2)
const int PIN_SWITCH_SERVO = 5;  // The number of the GPIO pin where the switch manipulator servo is connected. (GPIO5 = D1)
const int PIN_SWITCH = 14;       // The number of the GPIO pin where the switch is connected. (DPIO14 = D5)

const int LID_START_POSITION = 0;
const int LID_END_POSITION = 90;
const int SWITCH_START_POSITION = 0;
const int SWITCH_END_POSITION = 90;
