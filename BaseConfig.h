/*
 R. J. Tidey 2019/12/30
 Basic config
*/
#define FILESYSTYPE 1

/*
Wifi Manager Web set up
*/
#define WM_NAME "uselessbox"
#define WM_PASSWORD "password"

//Update service set up
String host = "uselessbox";
const char* update_password = "password";

//define actions during setup
//define any call at start of set up
#define SETUP_START 1
//define config file name if used 
#define CONFIG_FILE "/uselessConfig.txt"
//set to 1 if SPIFFS or LittleFS used
#define SETUP_FILESYS 1
//define to set up server and reference any extra handlers required
#define SETUP_SERVER 1
//call any extra setup at end
#define SETUP_END 1

// comment out this define unless using modified WifiManager with fast connect support
#define FASTCONNECT true

#include "BaseSupport.h"
