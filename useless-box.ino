// Platform libraries.
#include <Arduino.h>  // To add IntelliSense for platform constants.
#include "BaseConfig.h"

// Third-party libraries.
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

// My classes.
#include "speed-servo.h"
#include "status-led.h"
#include "proximity-sensor.h"
#include "config.h"  // To store configuration and secrets.


int timeInterval = 250;
unsigned long elapsedTime;
#define MAX_ACTIONS 25

int switchStart = 0;
int switchHalf = 50;
int switchEnd = 118;
int switchFast = 5;
int switchSlow = 15;
int lidStart = 90;
int lidEnd = 40;
int lidFast = 5;
int lidSlow = 1;
int sensorTriggerThreshold = 100;
int mode = 0;
int actionCount= 0;
String actions[MAX_ACTIONS];

SpeedServo lidServo;
SpeedServo switchServo;
StatusLed led;
ProximitySensor sensor;

int lastSwitchState = 0;
long actionIndex = 0;
bool isLidOpen = false;
int monitorSensor = 0;

#define START_AUDIO "/start.mp3"
#define SENSOR_AUDIO "/sensor"

AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;
AudioFileSourceID3 *id3;

#define MAX_FILES 10
char filename[8] = "/00.mp3";
int fileIndex = 0;
float audioGain = 3.9;

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
	(void)cbData;
	Serial.printf("ID3 callback for: %s = '", type);

	if (isUnicode) {
		string += 2;
	}
  
	while (*string) {
		char a = *(string++);
		if (isUnicode) {
			string++;
		}
		Serial.printf("%c", a);
				}
	Serial.println();
	Serial.flush();
}

void handleAudio() {
	String filename = server.arg("audio");
	if(filename.length()) {
		playAudio("/" + filename + ".mp3");
	}
	server.send(200, "text/json", "audio was requested");
}

void handleLid() {
	String pos = server.arg("pos");
	if(pos.length()) {
		lidServo.moveTo(pos.toInt(), lidSlow);
	}
	server.send(200, "text/json", "lid servo to " + pos);
}

void handleSwitch() {
	String pos = server.arg("pos");
	if(pos.length()) {
		switchServo.moveTo(pos.toInt(), switchSlow);
	}
	server.send(200, "text/json", "switch servo to " + pos);
}

void handleProximity() {
	String pos = server.arg("pos");
	if(pos.length()) {
		switchServo.moveTo(pos.toInt(), switchSlow);
	}
	server.send(200, "text/json", "Proximity = " + String(sensor.getProximity()));
}

/*
  load config
*/
void loadConfig() {
	String line = "";
	int config = 0;
	
	actionCount = 0;
	File f = SPIFFS.open(CONFIG_FILE, "r");
	if(f) {
		while(f.available()) {
			line =f.readStringUntil('\n');
			line.replace("\r","");
			if(line.length() > 0 && line.charAt(0) != '#') {
				switch(config) {
					case 0: host = line; break;
					case 1: switchStart = line.toInt(); break;
					case 2: switchHalf = line.toInt(); break;
					case 3: switchEnd = line.toInt(); break;
					case 4: switchFast = line.toInt(); break;
					case 5: switchSlow = line.toInt(); break;
					case 6: lidStart = line.toInt(); break;
					case 7: lidEnd = line.toInt(); break;
					case 8: lidFast = line.toInt(); break;
					case 9: lidSlow = line.toInt(); break;
					case 10: sensorTriggerThreshold = line.toInt();break;
					case 11: mode = line.toInt(); break;
					case 12: audioGain = line.toInt()/10; break;
					default :
						if(actionCount < MAX_ACTIONS) {
							actions[actionCount] = line;
							actionCount++;
						}
						break;
				}
				config++;
			}
		}
		f.close();
		Serial.println(F("Config loaded"));
		Serial.print(F("host:"));Serial.println(host);
		Serial.print(F("switchStart:"));Serial.println(String(switchStart));
		Serial.print(F("switchHalf:"));Serial.println(String(switchHalf));
		Serial.print(F("switchEnd:"));Serial.println(String(switchEnd));
		Serial.print(F("switchFast:"));Serial.println(String(switchFast));
		Serial.print(F("switchSlow:"));Serial.println(String(switchSlow));
		Serial.print(F("lidStart:"));Serial.println(String(lidStart));
		Serial.print(F("lidEnd:"));Serial.println(String(lidEnd));
		Serial.print(F("lidFast:"));Serial.println(String(lidFast));
		Serial.print(F("lidSlow:"));Serial.println(String(lidSlow));
		Serial.print(F("sensorTriggerThreshold:"));Serial.println(String(sensorTriggerThreshold));
		Serial.print(F("mode:"));Serial.println(String(mode));
		Serial.print(F("audioGain:"));Serial.println(String(audioGain));
		Serial.print(String(actionCount));Serial.println(F(" Actions loaded"));
	} else {
		Serial.println(String(CONFIG_FILE) + " not found");
	}
}

void updateactionIndex() {
	if(mode == 0) {
		actionIndex++;
		actionIndex = actionIndex % actionCount;
	} else {
		actionIndex = random(actionCount);
	}
}

void playAudio(String filename) {
	Serial.println("Play " + filename);
	if(SPIFFS.exists(filename)) {
		file = new AudioFileSourceSPIFFS(filename.c_str());
		id3 = new AudioFileSourceID3(file);
		id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
		out = new AudioOutputI2SNoDAC();
		mp3 = new AudioGeneratorMP3();
		delay(100);
		out->SetGain(audioGain);
		mp3->begin(id3, out);
		while(mp3->isRunning()) {
			if (!mp3->loop()) {delay(500);mp3->stop();}
		}
		delete mp3;
		delete out;
		delete id3;
		delete file;
	}
}

void runEx() {
	int i = 0;
	int j = 0;
	int servo;
	int position;
	int speed;
	String action;
	unsigned long t = millis();
	String msg;
	char actionChar;
	char actionChar1;
	
	Serial.println("run " + actions[actionIndex]);
	while(j < actions[actionIndex].length()) {
		msg = String((millis() -t)) + " ";
		j = actions[actionIndex].indexOf(',', i);
		if(j < 0) j = actions[actionIndex].length();
		servo = 0;
		action = actions[actionIndex].substring(i,j);
		actionChar = action.charAt(0);
		action = action.substring(1);
		if(actionChar == 'D') {
			//delay
			delay(action.toInt());
			msg += "Delay " + action;
		} else if(actionChar == 'A') {
			//Audio
			msg += "AudioPlay \/" + action + ".mp3";
			Serial.println(msg);
			playAudio("/" + action + ".mp3");
			msg = "";
		} else if(actionChar == 'X' || actionChar == 'Y') {
			speed = action.substring(0,2).toInt();
			position = action.substring(3).toInt();
			if(actionChar == 'X') {
				servo = 1;
				position = switchStart + (switchEnd - switchStart) * position / 100;
				
			} else {
				position = lidStart + (lidEnd - lidStart) * position / 100;
				servo = 2;
			}
		} else if(actionChar == 'S') {
			position = switchStart;
			speed = (action == "S") ? switchSlow : switchFast;
			servo = 1;
		} else if(actionChar == 'H') {
			position = switchHalf;
			speed = (action == "S") ? switchSlow : switchFast;
			servo = 1;
		} else if(actionChar == 'E') {
			position = switchEnd;
			speed = (action == "S") ? switchSlow : switchFast;
			servo = 1;
		} else  if(actionChar == 'C') {
			position = lidStart;
			speed = (action == "S") ? lidSlow : lidFast;
			servo = 2;
		} else if(actionChar == 'O') {
			position = lidEnd;
			speed = (action == "S") ? lidSlow : lidFast;
			servo = 2;
		} else if(actionChar == 'M') {
			monitorSensor = action.toInt();
			msg+= "monitorSensor " + action;
		} else if(actionChar == 'P') {
			mode = action.toInt();
			msg+= "mode " + action;
		}
		if(servo == 1) {
			switchServo.moveTo(position, speed);
			msg += "switch to " + String(position) + " speed " + String(speed);
		} else if (servo == 2) {
			lidServo.moveTo(position, speed);
			msg += "lid to " + String(position) + " speed " + String(speed);
		}
		if(msg != "") Serial.println(msg);
		i = j + 1;
	}
}

void initServos() {
	lidServo.attach(PIN_LID_SERVO);
	lidServo.moveTo(lidStart, 0);

	switchServo.attach(PIN_SWITCH_SERVO);
	switchServo.moveTo(switchStart, 0);
}

void initLed() {
	led.setPin(LED_BUILTIN);
	led.turnOff();
}

void initSensor() {
	sensor.attach(PIN_SENSOR_SDA, PIN_SENSOR_SCL, sensorTriggerThreshold);
}

void setupStart() {
	audioLogger = &Serial;
}

void extraHandlers() {
	server.on("/audio", HTTP_GET, handleAudio);
	server.on("/lid", HTTP_GET, handleLid);
	server.on("/switch", HTTP_GET, handleSwitch);
	server.on("/proximity", HTTP_GET, handleProximity);
}

void setupEnd() {
	initServos();
	initLed();
	initSensor();
	randomSeed(analogRead(0));
	pinMode(PIN_SWITCH, INPUT_PULLUP);
	playAudio(START_AUDIO);
}

void loop() {
	int switchState = digitalRead(PIN_SWITCH);
	boolean isSwitchTurnedOn = (switchState != lastSwitchState) && (switchState == LOW);

	if (isSwitchTurnedOn) {
		led.turnOn();
		runEx();
		updateactionIndex();
		isLidOpen = false;
		led.turnOff();
	} else {
		// Check the proximity sensor.
		if (sensorTriggerThreshold && sensor.isInRange()) {
			if (!isLidOpen && monitorSensor) {
				lidServo.moveTo(lidEnd, lidFast);
				playAudio(SENSOR_AUDIO + String(monitorSensor) + ".mp3");
				isLidOpen = true;
			}
		} else {
			if (isLidOpen) {
				lidServo.moveTo(lidStart, lidFast);
				isLidOpen = false;
			}
		}
	}

	lastSwitchState = switchState;

	// Wait 250 ms before next reading (required for the sensor).
	delay(timeInterval);
	elapsedTime++;
	server.handleClient();
}

