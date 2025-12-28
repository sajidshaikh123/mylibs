#ifndef IOTBOARD_H
#define IOTBOARD_H

// IOT Board Library - Master Header File
// Includes all library headers from mylibs folder

// ==================== VERSION MANAGEMENT ====================
// Library Version (Semantic Versioning: MAJOR.MINOR.PATCH)
#define IOTBOARD_VERSION_MAJOR 1
#define IOTBOARD_VERSION_MINOR 0
#define IOTBOARD_VERSION_PATCH 0
#define IOTBOARD_VERSION "1.0.0"

// Build date and time
#define IOTBOARD_BUILD_DATE __DATE__
#define IOTBOARD_BUILD_TIME __TIME__

// Board Hardware Version
#define BOARD_HW_VERSION "1.0"
#define BOARD_MODEL "ESP32-IOT-BOARD"

// Firmware Version
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0
#define FIRMWARE_VERSION "1.0.0"

// Version comparison macros
#define IOTBOARD_VERSION_CHECK(major, minor, patch) \
    ((IOTBOARD_VERSION_MAJOR > (major)) || \
     (IOTBOARD_VERSION_MAJOR == (major) && IOTBOARD_VERSION_MINOR > (minor)) || \
     (IOTBOARD_VERSION_MAJOR == (major) && IOTBOARD_VERSION_MINOR == (minor) && IOTBOARD_VERSION_PATCH >= (patch)))

// ==================== LIBRARY INCLUDES ====================
// NOTE: Libraries must be included BEFORE the namespace functions that use them

#include "Print.h"
#include "EEPROM.h"
#include "delayTimer.h"
#include "dwindisplay.h"
#include "EthernetManager.h"
#include "RTCManager.h"
#include "RTC_operation.h"
// #include "Filesystem.h"
#include "FilesystemManager.h"
#include "jsonoperation.h"
#include "MQTT_Lib.h"
#include "PCF8574_Input.h"
#include "PCF8574_Output.h"
#include "pindefinition.h"
#include "pixel_led.h"
#include "RS485_modbus_RTU.h"
#include "serial_scanner.h"
#include "string_functions.h"
#include "usb_scanner_Lib.h"
#include "WeighingScale.h"
#include "WiFi_manager.h"
#include "shift_timing.h"
// #include "firmware_update.h"  // Moved to end because it depends on other libraries

// Helper function declarations for version info
namespace IOTBoard {
    inline const char* getLibraryVersion() { return IOTBOARD_VERSION; }
    inline const char* getFirmwareVersion() { return FIRMWARE_VERSION; }
    inline const char* getBoardModel() { return BOARD_MODEL; }
    inline const char* getHardwareVersion() { return BOARD_HW_VERSION; }
    inline const char* getBuildDate() { return IOTBOARD_BUILD_DATE; }
    inline const char* getBuildTime() { return IOTBOARD_BUILD_TIME; }
    
    // Print all version info
    inline void printVersionInfo() {
        Serial.println("=== IOT Board Version Info ===");
        Serial.print("Library Version: "); Serial.println(IOTBOARD_VERSION);
        Serial.print("Firmware Version: "); Serial.println(FIRMWARE_VERSION);
        Serial.print("Board Model: "); Serial.println(BOARD_MODEL);
        Serial.print("Hardware Version: "); Serial.println(BOARD_HW_VERSION);
        Serial.print("Build Date: "); Serial.print(IOTBOARD_BUILD_DATE);
        Serial.print(" "); Serial.println(IOTBOARD_BUILD_TIME);
        Serial.println("==============================");
    }
    
    // Print detailed version info including individual library versions
    inline void printDetailedVersionInfo() {
        Serial.println("=== IOT Board Detailed Version Info ===");
        Serial.print("IOT Board Library: "); Serial.println(IOTBOARD_VERSION);
        Serial.print("Firmware Version: "); Serial.println(FIRMWARE_VERSION);
        Serial.print("Board Model: "); Serial.println(BOARD_MODEL);
        Serial.print("Hardware Version: "); Serial.println(BOARD_HW_VERSION);
        Serial.print("Build Date: "); Serial.print(IOTBOARD_BUILD_DATE);
        Serial.print(" "); Serial.println(IOTBOARD_BUILD_TIME);
        Serial.println("\n--- Individual Library Versions ---");
        Serial.print("DelayTimer: "); Serial.println(DelayTimer::getVersion());
        // Add other library versions here as they implement version control
        Serial.println("=======================================");
    }
}


// Create RTCManager instance with timezone offset (0 for UTC)
// For other timezones, use seconds offset: e.g., 19800 for IST (UTC+5:30)

DelayTimer onesecloop(1000);
DelayTimer ms_100loop(100);

uint8_t conn_status = 0;
uint8_t mac[6];
String mac_str = "";

RTCManager rtc(0);

EthernetClient ethClient;
EthernetManager ethManager;

#define FILE_SYSTEM FFat
FilesystemManager fsManagerFFat(FilesystemType::FFAT);

MQTT_Lib mqtt_obj;


// Explicitly call the default constructor
PCF8574_Output outputExpander = PCF8574_Output(OUTPUT_ADDR, SCL_PIN, SDA_PIN);
PCF8574_Input inputExpander = PCF8574_Input(INPUT_ADDR, SCL_PIN, SDA_PIN, INPUT_INTERRUPT);

extern void inputISR();

void boardinit(){
    // Initialize pixel LED
    pixelInit();
    setpixel(RED); // Indicate initialization start

    if(EEPROM.begin(512)){
        Serial.println("EEPROM initialized.");
    } else {
        Serial.println("Failed to initialize EEPROM.");
    }

    // Initialize RTC
    if(rtc.begin()){
        Serial.println("External RTC found and initialized.");
    } else {
        Serial.println("External RTC not found. Using internal RTC.");
    }

    // ethManager = EthernetManager();
    // ethManager.begin();

    // Initialize PCF8574 Expanders
    if(outputExpander.begin()){
        Serial.println("PCF8574 Output Expander initialized.");
    } else {
        Serial.println("Failed to initialize PCF8574 Output Expander.");
    }

    if(inputExpander.begin()){
        Serial.println("PCF8574 Input Expander initialized.");
        inputExpander.attachInt(inputISR, CHANGE);
    } else {
        Serial.println("Failed to initialize PCF8574 Input Expander.");
    }
    
    if (fsManagerFFat.init()) {
      Serial.println("\nFileSystem: FFat initialized successfully.");
        
    } else {
        Serial.println("FFat initialization failed");
    }

    WiFi.mode(WIFI_STA);
    delay(50);

    WiFi.macAddress(mac);
    mac_str = WiFi.macAddress();

    Serial.print("MAC Address: ");
    for (int i = 0; i < 6; i++)
    {
        if (mac[i] < 16)
            Serial.print("0");
        Serial.print(mac[i], HEX);
        if (i < 5)
            Serial.print(":");
    }
    Serial.println();
    WiFi.disconnect();

    
}


void boardloop(){
    if(onesecloop.ontime()){
        // Place 1-second interval tasks here
        if (ethManager.status() == 1)
        {
            if (conn_status == 0)
            {
                conn_status = 1;
                Serial.println(Ethernet.localIP());
                // HMI.Write_UString(PS_INFO,Ethernet.localIP().toString().c_str()); 
            }
        }
        else
        {
            conn_status = 0;
        }
        toggleLED(conn_status);
    }

    if(ms_100loop.ontime()){
        // Place 100-millisecond interval tasks here
        if (conn_status > 0)
        {
            mqtt_obj.loop();
        }
    }
}

#endif // IOTBOARD_H
