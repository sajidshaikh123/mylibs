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
#include <Preferences.h>
#include "EEPROM.h"
#include "dwin_function.h"
#include "delayTimer.h"
#include "dwindisplay.h"
#include "EthernetManager.h"
#include "RTCManager.h"
// #include "RTC_operation.h"


RTCManager rtc(0);

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
#include <ESP32Ping.h>
#include "serial_manager.h"
#include "shift_timing.h"
#include "mqtt_publisher.h"
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
bool mqtt_connected = 0;

bool wifi_connected = false;

uint8_t mac[6];
String mac_str = "";


EthernetClient ethClient;
WiFiClient wifiClient;
EthernetManager ethManager;

Preferences subtopicsPref;
Preferences wifiPref;
Preferences ethernetPref;
// Declare MQTT preferences
Preferences mqttPref;
Preferences hmiPref;

// Subtopic JSON document (used by MQTT)
DynamicJsonDocument subtopic(512);


#define FILE_SYSTEM FFat
FilesystemManager fsManagerFFat(FilesystemType::FFAT);

MQTT_Lib mqtt_obj;


// Explicitly call the default constructor
PCF8574_Output outputExpander = PCF8574_Output(OUTPUT_ADDR, SCL_PIN, SDA_PIN);
PCF8574_Input inputExpander = PCF8574_Input(INPUT_ADDR, SCL_PIN, SDA_PIN, INPUT_INTERRUPT);

extern void inputISR();

std::function<void(char*, uint8_t*, unsigned int)> mqtt_callback = nullptr;




void mqttcallbackmain(char *topic, byte *payload, unsigned int length)
{
    // Handle incoming MQTT messages here

    if (mqtt_callback) {
        mqtt_callback(topic, payload, length);
    }else{
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.print("] ");
        for (unsigned int i = 0; i < length; i++) {
            Serial.print((char)payload[i]);
        }
        Serial.println();
    }
}

void mqtt_setcallback(std::function<void(char*, uint8_t*, unsigned int)> callback)
{
    mqtt_callback = callback;
}

void boardinit(){

    Serial.begin(115200);
    Serial.setTimeout(300);
    Serial.println("IIOT Gateway Board ");

    // Initialize HMI if enabled in preferences
    hmiPref.begin("hmi", true);
    if(hmiPref.getBool("enabled", true)) {
        Serial.println("Initializing HMI...");
        HMI.begin(115200, SERIAL_8N1, DWIN_RX_PIN, DWIN_TX_PIN);
        HMI.reset();
    } else {
        Serial.println("HMI disabled in preferences.");
    }

    // Initialize pixel LED
    pixelInit();
    setpixel(RED); // Indicate initialization start

    // Initialize and load subtopic configuration
    initSubtopic();

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
    for (int i = 0; i < 6; i++){
        if (mac[i] < 16)
            Serial.print("0");
        Serial.print(mac[i], HEX);
        if (i < 5)
            Serial.print(":");
    }
    Serial.println();
    WiFi.disconnect();

    wifiPref.begin("wifi", true);

    if(wifiPref.getBool("enabled", false)){
        String ssid = wifiPref.getString("ssid", "");
        String password = wifiPref.getString("password", "");

        if(ssid != "" && password != ""){
            Serial.println("Connecting to saved WiFi credentials...");
            Serial.print("SSID: "); Serial.println(ssid);

            WiFi.begin(ssid.c_str(), password.c_str());
            delay(500);
            // Serial.print("Connecting to WiFi");
            
        }else{
            Serial.println("No valid WiFi credentials found in preferences.");
        }
    }else{
        Serial.println("WiFi not enabled in preferences.");
    }

    ethernetPref.begin("ethernet", true);

    if(ethernetPref.getBool("enabled", true) ){
        Serial.println("Initializing Ethernet with saved configuration...");

        bool useDHCP = ethernetPref.getBool("dhcp", true);
        String ip = ethernetPref.getString("ip", "");
        String gateway = ethernetPref.getString("gateway", "");
        String subnet = ethernetPref.getString("subnet", "");
        String dns = ethernetPref.getString("dns", "");
        
        ethManager.setIPSettings(mac, useDHCP, ip.c_str(), subnet.c_str(), gateway.c_str(), dns.c_str());
        ethManager.begin();
        ethManager.startPolling(1, 2000); // Poll every 2 seconds on core 1
    }else{
        Serial.println("Ethernet not enabled in preferences.");
    }

    mqttPref.begin("mqtt", true);

    // Load MQTT configuration
    bool mqttEnabled = mqttPref.getBool("enabled", false);
    String mqttServer = mqttPref.getString("server", "");
    uint16_t mqttPort = mqttPref.getUShort("port", 1883);
    String mqttUsername = mqttPref.getString("username", "");
    String mqttPassword = mqttPref.getString("password", "");
    String mqttTransport = mqttPref.getString("transport", "auto");
    // mqttPref.end();
    
    if(mqttEnabled){
        Serial.println("MQTT is enabled in preferences.");
        if(mqttTransport == "wifi"){
            mqtt_obj.config((const char *)mqttServer.c_str(), (int)mqttPort, (const char *)mqttUsername.c_str(), (const char *)mqttPassword.c_str(), "disconnected", wifiClient);
        }else if(mqttTransport == "ethernet"){
            mqtt_obj.config((const char *)mqttServer.c_str(), (int)mqttPort, (const char *)mqttUsername.c_str(), (const char *)mqttPassword.c_str(), "disconnected", ethClient);
        }else{ // auto
            mqtt_obj.config((const char *)mqttServer.c_str(), (int)mqttPort, (const char *)mqttUsername.c_str(), (const char *)mqttPassword.c_str(), "disconnected", ethClient);
        }
        mqtt_obj.setsubscribeto("#");

        mqtt_obj.setsubtopic(subtopic);
        mqtt_obj.setCallback(mqttcallbackmain);
        mqtt_obj.setMacAddress(mac_str);
    }
    
}


void boardloop(){

    handleSerialCommands();
    if(onesecloop.ontime()){
        // Place 1-second interval tasks here
        if (ethManager.status() == 1){
            if (conn_status == 0){
                conn_status = 1;
                mqtt_obj.setClient(ethClient);
                Serial.println(Ethernet.localIP());

                // HMI.Write_UString(PS_INFO,Ethernet.localIP().toString().c_str()); 
            }
        }else{
            conn_status = 0;
        }
        if(mqtt_connected){
            toggleLED(2);
        }else{
            toggleLED(conn_status);
        }
    }

    if(ms_100loop.ontime()){
        // Place 100-millisecond interval tasks here
        if (conn_status > 0 || wifi_connected){
            mqtt_obj.loop();
            mqtt_connected = (mqtt_obj.connectionStatus() == MQTT_CONNECTED);
        }
        if(wifiPref.getBool("enabled", false) ){
            if(WiFi.status() == WL_CONNECTED ){
                if(wifi_connected == false){
                    wifi_connected = true;
                    mqtt_obj.setClient(wifiClient);
                    Serial.println("[WiFi] ✓ Connected successfully!");
                    Serial.print("[WiFi] IP Address: ");
                    Serial.println(WiFi.localIP());
                }
            }else{
                wifi_connected = false;
            }
        }
    }
}

#endif // IOTBOARD_H
