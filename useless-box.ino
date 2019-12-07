// Platform libraries.
#include <Arduino.h>  // To add IntelliSense for platform constants.
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include "FS.h"

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

/*
Wifi Manager Web set up
If WM_NAME defined then use WebManager
*/
#define WM_NAME "useless"
#define WM_PASSWORD "password"
#ifdef WM_NAME
	WiFiManager wifiManager;
#endif
char wmName[33];
#define WIFI_CHECK_TIMEOUT 30000

//For update service
String host = "esp8266-useless";
const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "password";

//AP definitions only used if not using wifimanager
#define AP_SSID "ssid"
#define AP_PASSWORD "password"
#define AP_MAX_WAIT 10
String macAddr;

#define AP_PORT 80

ESP8266WebServer server(AP_PORT);
ESP8266HTTPUpdateServer httpUpdater;
int timeInterval = 250;
unsigned long elapsedTime;
unsigned long wifiCheckTime;
//holds the current upload
File fsUploadFile;
#define CONFIG_FILE "/uselessConfig.txt"
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


void initFS() {
	if(!SPIFFS.begin()) {
		Serial.println(F("No SIFFS found. Format it"));
		if(SPIFFS.format()) {
			SPIFFS.begin();
		} else {
			Serial.println(F("No SIFFS found. Format it"));
		}
	} else {
		Serial.println(F("SPIFFS file list"));
		Dir dir = SPIFFS.openDir("/");
		while (dir.next()) {
			Serial.print(dir.fileName());
			Serial.print(F(" - "));
			Serial.println(dir.fileSize());
		}
	}
}

String getContentType(String filename){
	if(server.hasArg("download")) return "application/octet-stream";
	else if(filename.endsWith(".htm")) return "text/html";
	else if(filename.endsWith(".html")) return "text/html";
	else if(filename.endsWith(".css")) return "text/css";
	else if(filename.endsWith(".js")) return "application/javascript";
	else if(filename.endsWith(".png")) return "image/png";
	else if(filename.endsWith(".gif")) return "image/gif";
	else if(filename.endsWith(".jpg")) return "image/jpeg";
	else if(filename.endsWith(".ico")) return "image/x-icon";
	else if(filename.endsWith(".xml")) return "text/xml";
	else if(filename.endsWith(".pdf")) return "application/x-pdf";
	else if(filename.endsWith(".zip")) return "application/x-zip";
	else if(filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

bool handleFileRead(String path){
	Serial.printf_P(PSTR("handleFileRead: %s\r\n"), path.c_str());
	if(path.endsWith("/")) path += "index.htm";
	String contentType = getContentType(path);
	String pathWithGz = path + ".gz";
	if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
		if(SPIFFS.exists(pathWithGz))
			path += ".gz";
		File file = SPIFFS.open(path, "r");
		size_t sent = server.streamFile(file, contentType);
		file.close();
		return true;
	}
	return false;
}

void handleFileUpload(){
	if(server.uri() != "/edit") return;
	HTTPUpload& upload = server.upload();
	if(upload.status == UPLOAD_FILE_START){
		String filename = upload.filename;
		if(!filename.startsWith("/")) filename = "/"+filename;
		Serial.printf_P(PSTR("handleFileUpload Name: %s\r\n"), filename.c_str());
		fsUploadFile = SPIFFS.open(filename, "w");
		filename = String();
	} else if(upload.status == UPLOAD_FILE_WRITE){
		Serial.printf_P(PSTR("handleFileUpload Data: %d\r\n"), upload.currentSize);
		if(fsUploadFile)
			fsUploadFile.write(upload.buf, upload.currentSize);
	} else if(upload.status == UPLOAD_FILE_END){
		if(fsUploadFile)
			fsUploadFile.close();
		Serial.printf_P(PSTR("handleFileUpload Size: %d\r\n"), upload.totalSize);
	}
}

void handleFileDelete(){
	if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
	String path = server.arg(0);
	Serial.printf_P(PSTR("handleFileDelete: %s\r\n"),path.c_str());
	if(path == "/")
		return server.send(500, "text/plain", "BAD PATH");
	if(!SPIFFS.exists(path))
		return server.send(404, "text/plain", "FileNotFound");
	SPIFFS.remove(path);
	server.send(200, "text/plain", "");
	path = String();
}

void handleFileCreate(){
	if(server.args() == 0)
		return server.send(500, "text/plain", "BAD ARGS");
	String path = server.arg(0);
	Serial.printf_P(PSTR("handleFileCreate: %s\r\n"),path.c_str());
	if(path == "/")
		return server.send(500, "text/plain", "BAD PATH");
	if(SPIFFS.exists(path))
		return server.send(500, "text/plain", "FILE EXISTS");
	File file = SPIFFS.open(path, "w");
	if(file)
		file.close();
	else
		return server.send(500, "text/plain", "CREATE FAILED");
	server.send(200, "text/plain", "");
	path = String();
}

void handleFileList() {
	if(!server.hasArg("dir")) {server.send(500, "text/plain", "BAD ARGS"); return;}

	String path = server.arg("dir");
	Serial.printf_P(PSTR("handleFileList: %s\r\n"),path.c_str());
	Dir dir = SPIFFS.openDir(path);
	path = String();

	String output = "[";
	while(dir.next()){
		File entry = dir.openFile("r");
		if (output != "[") output += ',';
		bool isDir = false;
		output += "{\"type\":\"";
		output += (isDir)?"dir":"file";
		output += "\",\"name\":\"";
		output += String(entry.name()).substring(1);
		output += "\"}";
		entry.close();
	}
	output += "]";
	server.send(200, "text/json", output);
}

void handleMinimalUpload() {
	char temp[700];

	snprintf ( temp, 700,
    "<!DOCTYPE html>\
	<html>\
		<head>\
			<title>ESP8266 Upload</title>\
			<meta charset=\"utf-8\">\
			<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\
			<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
		</head>\
		<body>\
			<form action=\"/edit\" method=\"post\" enctype=\"multipart/form-data\">\
				<input type=\"file\" name=\"data\">\
				<input type=\"text\" name=\"path\" value=\"/\">\
				<button>Upload</button>\
			</form>\
		</body>\
    </html>"
	);
	server.send ( 200, "text/html", temp );
}

void handleSpiffsFormat() {
	SPIFFS.format();
	server.send(200, "text/json", "format complete");
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
  Get config
*/
void getConfig() {
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


void setup() {
	initSerial();
 	Serial.println(F("Set up filing system"));
	initFS();
	getConfig();
	initWifi();
	initServos();
	initLed();
	initSensor();
	randomSeed(analogRead(0));
	pinMode(PIN_SWITCH, INPUT_PULLUP);
	playAudio(START_AUDIO);
	Serial.println(F("Setup completed."));
}

/*
  Connect to local wifi with retries
  If check is set then test the connection and re-establish if timed out
*/
int wifiConnect(int check) {
	if(check) {
		if((elapsedTime - wifiCheckTime) * timeInterval > WIFI_CHECK_TIMEOUT) {
			if(WiFi.status() != WL_CONNECTED) {
				Serial.println(F("Wifi connection timed out. Try to relink"));
			} else {
				wifiCheckTime = elapsedTime;
				return 1;
			}
		} else {
			return 1;
		}
	}
	wifiCheckTime = elapsedTime;
#ifdef WM_NAME
	Serial.println(F("Set up managed Web"));
#ifdef WM_STATIC_IP
	wifiManager.setSTAStaticIPConfig(IPAddress(WM_STATIC_IP), IPAddress(WM_STATIC_GATEWAY), IPAddress(255,255,255,0));
#endif
	wifiManager.setConfigPortalTimeout(180);
	//Revert to STA if wifimanager times out as otherwise APA is left on.
	strcpy(wmName, WM_NAME);
	strcat(wmName, macAddr.c_str());
	wifiManager.autoConnect(wmName, WM_PASSWORD);
	WiFi.mode(WIFI_STA);
#else
	Serial.println(F("Set up manual Web"));
	int retries = 0;
	Serial.print(F("Connecting to AP"));
	#ifdef AP_IP
		IPAddress addr1(AP_IP);
		IPAddress addr2(AP_DNS);
		IPAddress addr3(AP_GATEWAY);
		IPAddress addr4(AP_SUBNET);
		WiFi.config(addr1, addr2, addr3, addr4);
	#endif
	WiFi.begin(AP_SSID, AP_PASSWORD);
	while (WiFi.status() != WL_CONNECTED && retries < AP_MAX_WAIT) {
		delaymSec(1000);
		Serial.print(".");
		retries++;
	}
	Serial.println("");
	if(retries < AP_MAX_WAIT) {
		Serial.print(F("WiFi connected ip "));
		Serial.print(WiFi.localIP());
		Serial.printf(":%d mac %s\r\n", AP_PORT, WiFi.macAddress().c_str());
		return 1;
	} else {
		Serial.println(F("WiFi connection attempt failed")); 
		return 0;
	} 
#endif
	//wifi_set_sleep_type(LIGHT_SLEEP_T);
}

void initWifi() {
	Serial.println(F("Set up Wifi services"));
	macAddr = WiFi.macAddress();
	macAddr.replace(":","");
	Serial.println(macAddr);
	wifiConnect(0);
	//Update service
	MDNS.begin(host.c_str());
	httpUpdater.setup(&server, update_path, update_username, update_password);
	Serial.println(F("Set up web server"));
	//Simple upload
	server.on("/upload", handleMinimalUpload);
	server.on("/format", handleSpiffsFormat);
	server.on("/list", HTTP_GET, handleFileList);
	//load editor
	server.on("/edit", HTTP_GET, [](){
	if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");});
	//create file
	server.on("/edit", HTTP_PUT, handleFileCreate);
	//delete file
	server.on("/edit", HTTP_DELETE, handleFileDelete);
	//first callback is called after the request has ended with all parsed arguments
	//second callback handles file uploads at that location
	server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);
	//called when the url is not defined here
	//use it to load content from SPIFFS
	server.on("/audio", HTTP_GET, handleAudio);
	server.on("/lid", HTTP_GET, handleLid);
	server.on("/switch", HTTP_GET, handleSwitch);
	server.on("/proximity", HTTP_GET, handleProximity);
	server.onNotFound([](){if(!handleFileRead(server.uri())) server.send(404, "text/plain", "FileNotFound");});
	server.begin();
	MDNS.addService("http", "tcp", 80);
}

void initSerial() {
	Serial.begin(115200);
	Serial.println();
	Serial.println(F("Initializing serial connection DONE."));
	audioLogger = &Serial;
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
			if(actionChar == 'X')
				servo = 1;
			else
				servo = 2;
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
