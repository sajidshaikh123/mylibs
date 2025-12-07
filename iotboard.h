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

#include "delayTimer.h"
#include "dwindisplay.h"
#include "EthernetManager.h"
#include "RTCManager.h"
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
RTCManager rtc(0);



#endif // IOTBOARD_H
