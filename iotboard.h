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
#include "esp_task_wdt.h"
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
#include "serialport.h"
#include "serial_scanner.h"
#include "string_functions.h"

#if defined(ESP32)
    #if CONFIG_IDF_TARGET_ESP32C6
        // #include "usb_scanner_Lib.h"
    #elif CONFIG_IDF_TARGET_ESP32S3
        #include "usb_scanner_Lib.h"
    #endif

#endif


#include "WeighingScale.h"
#include "WiFi_manager.h"
#include <ESP32Ping.h>
#include "serial_manager.h"
#include "shift_timing.h"
#include "shift_details.h"
#include "mqtt_publisher.h"

// Forward-declare globals needed by firmware_update.h and mac_process.h
// (actual usage is below after all includes)
DynamicJsonDocument subtopic(512);
FilesystemManager fsManagerFFat(FilesystemType::FFAT);
MQTT_Lib mqtt_obj;

#include "firmware_update.h"
#include "mac_process.h"

// #include "opcua_server.h"


// Web configuration MUST be included BEFORE WiFi_manager to avoid HTTP method conflicts
#include "web_configuration.h"

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
bool mqttEnabled = false;
bool ethernetEnabled = false;  // Cache ethernet enabled status
bool wifiEnabled = false;      // Cache WiFi enabled status
bool tcpModbusEnabled = false; // Cache TCP Modbus enabled status
bool filesystemReady = false;  // Track filesystem status
bool rs485ModbusEnabled = false; // Cache RS485 Modbus enabled status
bool usbScannerEnabled = false;  // Cache USB scanner enabled status


// Cache MQTT subtopic components to avoid NVS reads in high-frequency loops
String cached_company = "";
String cached_location = "";
String cached_department = "";
String cached_line = "";
String cached_machine = "";
bool subtopicCacheReady = false;

uint8_t mac[6];
String mac_str = "";


EthernetClient ethClient;
WiFiClient wifiClient;
EthernetManager ethManager;

Preferences wifiPref;

// Set to true by wifiApplyCredentials() to restart the WiFi init state machine
static volatile bool _wifiReconnectRequested = false;

// Save new WiFi credentials to NVS and trigger a reconnect in boardloop.
// Safe to call from any context (loop, callback, etc.).
inline void wifiApplyCredentials(const String& ssid, const String& password) {
    { Preferences p;
      p.begin("wifi", false);
      p.putBool("enabled",  true);
      p.putBool("dhcp",     true);
      p.putString("ssid",     ssid);
      p.putString("password", password);
      p.end(); }
    wifiEnabled = true;
    _wifiReconnectRequested = true;
    Serial.printf("[WiFi] Credentials updated — SSID: %s — reconnecting...\n", ssid.c_str());
}
Preferences ethernetPref;
// Declare MQTT preferences
Preferences mqttPref;
Preferences hmiPref;
Preferences tcpModbusPref;
Preferences settingsPref;
Preferences rs485ModbusPref;
Preferences serialportPref;

bool serialportEnabled = false;  // Cache serial port enabled status

String mqttTransport = "auto"; // Cache MQTT transport type
String lastResetReason = "Unknown"; // Last boot reset reason (set in boardinit)

// subtopic, fsManagerFFat, mqtt_obj declared above (before firmware_update.h / mac_process.h)
#define FILE_SYSTEM FFat

unsigned long execution_timer = 0;


// Explicitly call the default constructor
PCF8574_Output outputExpander = PCF8574_Output(OUTPUT_ADDR, SCL_PIN, SDA_PIN);
PCF8574_Input inputExpander = PCF8574_Input(INPUT_ADDR, SCL_PIN, SDA_PIN, INPUT_INTERRUPT);

extern void IRAM_ATTR inputISR();

std::function<void(char*, uint8_t*, unsigned int)> mqtt_callback = nullptr;
std::function<void()> wifi_connected_callback = nullptr;

void set_wifi_connected_callback(std::function<void()> cb) { wifi_connected_callback = cb; }




void mqttcallbackmain(char *topic, byte *payload, unsigned int length)
{
    // Handle incoming MQTT messages here
    // Add bounds checking to prevent heap corruption
    if(length > 4096) {
        Serial.printf("⚠ Warning: MQTT message too large (%u bytes), truncating\n", length);
        // length = 4096;
    }

    String _topic_str = String(topic);
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += String((char)payload[i]);
    }

    splitTopic(_topic_str, "/");

    if (split_data[1].equals(mac_str)) {
        Serial.println("IN MAC");
        Serial.print(_topic_str + ":");
        Serial.println(message);

        processMac(split_data[2],message);

    }else if (mqtt_callback) {
        mqtt_callback(topic, payload, length);
    }else{
        Serial.print("Message arrived [");
        Serial.print(topic);
        Serial.print("] ");
        
        // Safely print payload with length limit
        for (unsigned int i = 0; i < length && i < 512; i++) {
            Serial.print((char)payload[i]);
        }
        if(length > 512) {
            Serial.print("... (truncated)");
        }
        Serial.println();
    }
}

void mqtt_setcallback(std::function<void(char*, uint8_t*, unsigned int)> callback)
{
    mqtt_callback = callback;
}



// Add this function to check PSRAM status
void printPSRAMInfo() {
    Serial.println("\n=== PSRAM Information ===");
    
    #ifdef BOARD_HAS_PSRAM
    Serial.println("PSRAM: Enabled in build configuration");
    #else
    Serial.println("PSRAM: Not enabled in build configuration");
    #endif
    
    if(psramFound()) {
        Serial.println("PSRAM: ✓ Detected and available");
        Serial.printf("PSRAM Size: %d bytes (%.2f MB)\n", ESP.getPsramSize(), ESP.getPsramSize() / (1024.0 * 1024.0));
        Serial.printf("Free PSRAM: %d bytes (%.2f MB)\n", ESP.getFreePsram(), ESP.getFreePsram() / (1024.0 * 1024.0));
        Serial.printf("PSRAM Used: %d bytes (%.2f MB)\n", 
                      ESP.getPsramSize() - ESP.getFreePsram(), 
                      (ESP.getPsramSize() - ESP.getFreePsram()) / (1024.0 * 1024.0));
    } else {
        Serial.println("PSRAM: ✗ Not found or not enabled");
    }
    
    Serial.printf("Heap Size: %d bytes (%.2f KB)\n", ESP.getHeapSize(), ESP.getHeapSize() / 1024.0);
    Serial.printf("Free Heap: %d bytes (%.2f KB)\n", ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    Serial.printf("Min Free Heap: %d bytes (%.2f KB)\n", ESP.getMinFreeHeap(), ESP.getMinFreeHeap() / 1024.0);
    Serial.println("========================\n");
}

void printPartitionTable() {
    Serial.println("\n=== Partition Table ===");
    
    #ifdef ESP32
    #include "esp_partition.h"
    
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    
    if (it == NULL) {
        Serial.println("No partitions found!");
        return;
    }
    
    Serial.println("Label            Type     SubType      Address      Size         ");
    Serial.println("----------------------------------------------------------------");
    
    while (it != NULL) {
        const esp_partition_t *part = esp_partition_get(it);
        
        // Format label (max 16 chars, pad with spaces)
        char label[17];
        snprintf(label, sizeof(label), "%-16s", part->label);
        
        // Get type string
        const char* type_str;
        switch(part->type) {
            case ESP_PARTITION_TYPE_APP:  type_str = "app"; break;
            case ESP_PARTITION_TYPE_DATA: type_str = "data"; break;
            default: type_str = "unknown"; break;
        }
        
        // Get subtype string
        char subtype_str[12];
        if (part->type == ESP_PARTITION_TYPE_APP) {
            switch(part->subtype) {
                case ESP_PARTITION_SUBTYPE_APP_FACTORY: snprintf(subtype_str, sizeof(subtype_str), "factory"); break;
                case ESP_PARTITION_SUBTYPE_APP_OTA_0: snprintf(subtype_str, sizeof(subtype_str), "ota_0"); break;
                case ESP_PARTITION_SUBTYPE_APP_OTA_1: snprintf(subtype_str, sizeof(subtype_str), "ota_1"); break;
                default: snprintf(subtype_str, sizeof(subtype_str), "0x%02x", part->subtype); break;
            }
        } else if (part->type == ESP_PARTITION_TYPE_DATA) {
            switch(part->subtype) {
                case ESP_PARTITION_SUBTYPE_DATA_OTA: snprintf(subtype_str, sizeof(subtype_str), "ota"); break;
                case ESP_PARTITION_SUBTYPE_DATA_NVS: snprintf(subtype_str, sizeof(subtype_str), "nvs"); break;
                case ESP_PARTITION_SUBTYPE_DATA_SPIFFS: snprintf(subtype_str, sizeof(subtype_str), "spiffs"); break;
                case ESP_PARTITION_SUBTYPE_DATA_FAT: snprintf(subtype_str, sizeof(subtype_str), "fat"); break;
                default: snprintf(subtype_str, sizeof(subtype_str), "0x%02x", part->subtype); break;
            }
        } else {
            snprintf(subtype_str, sizeof(subtype_str), "0x%02x", part->subtype);
        }
        
        // Print partition info
        Serial.printf("%s %-8s %-12s 0x%06X   %7d KB\n", 
                     label, 
                     type_str, 
                     subtype_str,
                     part->address, 
                     part->size / 1024);
        
        it = esp_partition_next(it);
    }
    
    esp_partition_iterator_release(it);
    
    // Print flash chip info
    Serial.println("----------------------------------------------------------------");
    Serial.printf("Flash Chip Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("Flash Chip Speed: %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
    #endif
    
    Serial.println("=======================\n");
}

// ==================== PERIPHERAL STATUS PUBLISHER ====================
void publishPeripheralStatus() {
    if (mqtt_obj.connectionStatus() != MQTT_CONNECTED) return;

    DynamicJsonDocument doc(1024);

    // Board info
    doc["firmware"] = FIRMWARE_VERSION;
    doc["board"] = BOARD_MODEL;
    doc["hw_version"] = BOARD_HW_VERSION;
    doc["chip"] = ESP.getChipModel();
    doc["cores"] = ESP.getChipCores();
    doc["cpu_mhz"] = ESP.getCpuFreqMHz();
    doc["flash_mb"] = ESP.getFlashChipSize() / (1024 * 1024);
    doc["mac"] = mac_str;

    // PSRAM
    JsonObject psram = doc.createNestedObject("psram");
    psram["available"] = psramFound();
    if (psramFound()) {
        psram["size_mb"] = (float)(ESP.getPsramSize() / (1024.0 * 1024.0));
        psram["free_mb"] = (float)(ESP.getFreePsram() / (1024.0 * 1024.0));
    }

    // Memory
    doc["free_heap"] = ESP.getFreeHeap();
    doc["min_free_heap"] = ESP.getMinFreeHeap();

    // Peripheral enable states
    JsonObject peripherals = doc.createNestedObject("peripherals");
    peripherals["wifi"] = wifiEnabled;
    peripherals["ethernet"] = ethernetEnabled;
    peripherals["mqtt"] = mqttEnabled;
    peripherals["filesystem"] = settingsPref.getBool("fs_enabled", false);
    peripherals["input_expander"] = settingsPref.getBool("input_enabled", false);
    peripherals["output_expander"] = settingsPref.getBool("output_enabled", false);
    peripherals["hmi"] = hmiPref.getBool("enabled", false);
    peripherals["tcp_modbus"] = tcpModbusEnabled;

    // Live connection states
    JsonObject connections = doc.createNestedObject("connections");
    connections["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
    if (WiFi.status() == WL_CONNECTED) {
        connections["wifi_ip"] = WiFi.localIP().toString();
        connections["wifi_rssi"] = WiFi.RSSI();
    }
    connections["ethernet_linked"] = (Ethernet.linkStatus() == LinkON);
    if (Ethernet.linkStatus() == LinkON) {
        connections["ethernet_ip"] = Ethernet.localIP().toString();
    }
    connections["mqtt_transport"] = mqttTransport;
    connections["filesystem_ready"] = filesystemReady;

    // Storage (if filesystem enabled)
    if (filesystemReady) {
        JsonObject storage = doc.createNestedObject("storage");
        storage["type"] = "FFat";
        storage["total_kb"] = FFat.totalBytes() / 1024;
        storage["used_kb"] = FFat.usedBytes() / 1024;
        storage["free_kb"] = (FFat.totalBytes() - FFat.usedBytes()) / 1024;
        storage["used_pct"] = (FFat.totalBytes() > 0) ? (float)(FFat.usedBytes() * 100.0 / FFat.totalBytes()) : 0;
    }

    // RTC
    JsonObject rtcInfo = doc.createNestedObject("rtc");
    rtcInfo["type"] = rtc.isExternalRTCAvailable() ? "external" : "internal";
    rtcInfo["datetime"] = String(rtc.getDateTime());

    // Uptime
    doc["uptime_sec"] = millis() / 1000;
    doc["timestamp"] = String(rtc.getDateTime());

    // Publish as retained message
    String topic = mqtt_obj.getTopic("metadata/status");
    String payload;
    serializeJson(doc, payload);
    mqtt_obj.publish(topic.c_str(), payload.c_str(), true);  // retained = true
    Serial.println("[MQTT] Published peripheral status to metadata/status");
}

void boardinit(){

    Serial.begin(115200);
    Serial.setTimeout(50);
    Serial.println("IIOT Gateway Board");
    settingsPref.begin("settings", true); 

    #if defined(ESP32)
        #if CONFIG_IDF_TARGET_ESP32C6
        #elif CONFIG_IDF_TARGET_ESP32S3
            usbScannerEnabled = settingsPref.getBool("usb_enabled", false);
            if (usbScannerEnabled) {
                usb_scannerInit();
                Serial.println("USB Scanner Init");
            } else {
                Serial.println("USB Scanner disabled in preferences.");
            }
        #endif
    #endif
    
    // Configure watchdog timeout - increase to 10 seconds for network operations
    // #ifdef ESP32
    // esp_task_wdt_config_t wdt_config = {
    //     .timeout_ms = 10000,      // 10 second timeout
    //     .idle_core_mask = 0,      // Don't watch idle tasks
    //     .trigger_panic = true     // Trigger panic on timeout
    // };
    // esp_task_wdt_init(&wdt_config);
    // esp_task_wdt_add(NULL); // Add current task
    // esp_task_wdt_reset();
    // #endif
    
    // Feed watchdog early
    delay(5);

      // Print reset reason and store counters in NVS
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.print("Reset reason: ");
    
    const char* reasonKey = "unknown";
    switch(reason) {
        case ESP_RST_UNKNOWN:    Serial.println("Unknown");           reasonKey = "unknown";   lastResetReason = "Unknown";        break;
        case ESP_RST_POWERON:    Serial.println("Power-on");          reasonKey = "poweron";   lastResetReason = "Power-on";       break;
        case ESP_RST_EXT:        Serial.println("External pin");      reasonKey = "ext";       lastResetReason = "External pin";   break;
        case ESP_RST_SW:         Serial.println("Software reset");    reasonKey = "sw";        lastResetReason = "Software reset"; break;
        case ESP_RST_PANIC:      Serial.println("Exception/panic");   reasonKey = "panic";     lastResetReason = "Panic";          break;
        case ESP_RST_INT_WDT:    Serial.println("Interrupt watchdog");reasonKey = "int_wdt";   lastResetReason = "Int WDT";        break;
        case ESP_RST_TASK_WDT:   Serial.println("Task watchdog");     reasonKey = "task_wdt";  lastResetReason = "Task WDT";       break;
        case ESP_RST_WDT:        Serial.println("Other watchdog");    reasonKey = "wdt";       lastResetReason = "Other WDT";      break;
        case ESP_RST_DEEPSLEEP:  Serial.println("Deep sleep reset");  reasonKey = "deepsleep"; lastResetReason = "Deep Sleep";     break;
        case ESP_RST_BROWNOUT:   Serial.println("Brownout");          reasonKey = "brownout";  lastResetReason = "Brownout";       break;
        case ESP_RST_SDIO:       Serial.println("SDIO");              reasonKey = "sdio";      lastResetReason = "SDIO";           break;
        default:                 Serial.println("Unknown");           reasonKey = "unknown";   lastResetReason = "Unknown";        break;
    }

    // Increment reset reason counter in Preferences (NVS)
    {
        Preferences resetPref;
        resetPref.begin("reset_cnt", false);  // read-write
        uint32_t count = resetPref.getUInt(reasonKey, 0) + 1;
        resetPref.putUInt(reasonKey, count);
        uint32_t totalBoots = resetPref.getUInt("total", 0) + 1;
        resetPref.putUInt("total", totalBoots);

        Serial.println("\n=== Reset Reason Counters ===");
        Serial.printf("Total boots:       %u\n", totalBoots);
        const char* keys[]   = {"poweron","sw","panic","int_wdt","task_wdt","wdt","brownout","ext","deepsleep","sdio","unknown"};
        const char* labels[] = {"Power-on","Software","Panic","Int WDT","Task WDT","Other WDT","Brownout","External","DeepSleep","SDIO","Unknown"};
        for (int i = 0; i < 11; i++) {
            uint32_t val = resetPref.getUInt(keys[i], 0);
            if (val > 0) {
                Serial.printf("  %-12s: %u\n", labels[i], val);
            }
        }
        Serial.println("=============================\n");
        resetPref.end();
    }

    printPSRAMInfo();
    printPartitionTable();

    if(rtc.begin()){
        Serial.println("External RTC found and initialized.");
    } else {
        Serial.println("External RTC not found. Using internal RTC.");
    }
    yield();

    


    
    // Initialize TCP Modbus preferences
    tcpModbusPref.begin("tcpmodbus", true);
    tcpModbusEnabled = tcpModbusPref.getBool("enabled", false);  // Cache the value
    tcpModbusPref.end();
    
    // Initialize HMI if enabled in preferences
    hmiPref.begin("hmi", true);
    if(hmiPref.getBool("enabled", false) ) {  // Temporarily disabled (was true)
        Serial.println("Initializing HMI...");
        yield();

        HMI.begin(115200, SERIAL_8N1, DWIN_RX_PIN, DWIN_TX_PIN);
        delay(100);
        Serial.println("HMI Serial begun at 115200 baud.");

        
        Serial.println("HMI initialized successfully (reset skipped).");

        // Sync RTC time to HMI display
        HMI.Time_Stamp(
            rtc.getDay(),
            rtc.getMonth(),
            (uint8_t)(rtc.getYear() % 100),  // 2-digit year
            rtc.getHour(),
            rtc.getMinute(),
            rtc.getSecond()
        );
        Serial.printf("[HMI] RTC time synced: %02d/%02d/%02d %02d:%02d:%02d\n",
            rtc.getDay(), rtc.getMonth(), (uint8_t)(rtc.getYear() % 100),
            rtc.getHour(), rtc.getMinute(), rtc.getSecond());

        delay(100);
        yield();
        HMI.reset();  // Commented out - reset manually after full boot if needed
        delay(100);
        yield();
        
    } else {
        Serial.println("HMI disabled in preferences.");
    }
    yield();

    Serial.println("Initializing pixel LED...");
    pixelInit();
    setpixel(RED); // Indicate initialization start
    yield();

    

    Serial.println("Initializing EEPROM...");
    yield();
    if(EEPROM.begin(512)){
        Serial.println("EEPROM initialized.");
    } else {
        Serial.println("Failed to initialize EEPROM.");
    }
    yield();

    
    // ethManager = EthernetManager();
    // ethManager.begin();

    if(settingsPref.getBool("output_enabled", false)){
        if(outputExpander.begin()){
            Serial.println("PCF8574 Output Expander initialized.");
        } else {
            Serial.println("Failed to initialize PCF8574 Output Expander.");
        }
        yield();
    } else {
        Serial.println("Output expander disabled in preferences.");
    }

    if(settingsPref.getBool("input_enabled", false)){
        if(inputExpander.begin()){
            Serial.println("PCF8574 Input Expander initialized.");
            inputExpander.attachInt(inputISR, CHANGE);
            Serial.println("Input ISR attached.");
            yield();
        } else {
            Serial.println("Failed to initialize PCF8574 Input Expander.");
        }
        yield();
    } else {
        Serial.println("Input expander disabled in preferences.");
    }

    
    if(settingsPref.getBool("fs_enabled",false)){
        Serial.println("Initializing filesystem ...");
        yield();
        delay(100);
        yield();
        if (fsManagerFFat.init()) {
            Serial.println("[FS] FFat initialized successfully.");
            filesystemReady = true;
        } else {
            Serial.println("[FS] FFat initialization failed - continuing without filesystem");
            filesystemReady = false;
        }
        yield();
    }else{
        Serial.println("Filesystem disabled in preferences.");
    }


    WiFi.mode(WIFI_STA);
    delay(200); // Delay for 200 milliseconds
    WiFi.macAddress(mac);
    mac_str = WiFi.macAddress();
    WiFi.disconnect(); // Ensure we're disconnected before starting MQTT

    
    // Just cache preferences, don't initialize WiFi yet
    wifiPref.begin("wifi", true);
    wifiEnabled = wifiPref.getBool("enabled", false);  // Cache the value
    
    if(wifiEnabled){
        Serial.println("WiFi is enabled in preferences - will initialize in main loop.");
    } else {
        Serial.println("WiFi not enabled in preferences.");
    }
    yield();

    ethernetPref.begin("ethernet", true);
    ethernetEnabled = ethernetPref.getBool("enabled", false);  // Cache the value

    if(ethernetEnabled){
        Serial.println("Initializing Ethernet with saved configuration...");
        yield();

        bool useDHCP = ethernetPref.getBool("dhcp", true);
        String ip = ethernetPref.getString("ip", "");
        String gateway = ethernetPref.getString("gateway", "");
        String subnet = ethernetPref.getString("subnet", "");
        String dns = ethernetPref.getString("dns", "");
        Serial.printf("DHCP: %s\n", useDHCP ? "Yes" : "No");
        if (!useDHCP) {
            Serial.printf("IP: %s\n", ip.c_str());
            Serial.printf("Gateway: %s\n", gateway.c_str());
            Serial.printf("Subnet: %s\n", subnet.c_str());
            Serial.printf("DNS: %s\n", dns.c_str());
            Serial.printf("MAC: %s\n", mac_str.c_str());
        }
        mac[5] = 0xEF;
        ethManager.setIPSettings(mac, useDHCP, ip.c_str(), subnet.c_str(), gateway.c_str(), dns.c_str());
        yield();
        ethManager.begin();
        yield();
        ethManager.startPolling(1, 500); // Poll every 100 milliseconds on core 1
    }else{
        Serial.println("Ethernet not enabled in preferences.");
    }
    ethernetPref.end();
    yield();

    Serial.println("Initializing MQTT...");
    yield();
    mqttPref.begin("mqtt", true);

    // Load MQTT configuration
    mqttEnabled = mqttPref.getBool("enabled", false);
    
    yield();
    if(mqttEnabled){
        Serial.println("MQTT is enabled in preferences.");
        String mqttServer = mqttPref.getString("server", "");
        uint16_t mqttPort = mqttPref.getUShort("port", 1883);
        String mqttUsername = mqttPref.getString("username", "");
        String mqttPassword = mqttPref.getString("password", "");
        mqttTransport = mqttPref.getString("transport", "auto");
        yield();
        // Reduce WiFiClient TCP connect timeout from the 3-second ESP32 default to 1 second.
        // PubSubClient::setSocketTimeout() only controls the MQTT protocol read timeout
        // (waiting for CONNACK) — it does NOT affect the TCP connect() call.
        // Without this, every failed MQTT connect attempt blocks the main loop for ~3 s.
        wifiClient.setTimeout(1);

        if(mqttTransport == "wifi"){
            mqtt_obj.config((const char *)mqttServer.c_str(), (int)mqttPort, (const char *)mqttUsername.c_str(), (const char *)mqttPassword.c_str(), "disconnected", wifiClient);
        }else if(mqttTransport == "ethernet"){
            mqtt_obj.config((const char *)mqttServer.c_str(), (int)mqttPort, (const char *)mqttUsername.c_str(), (const char *)mqttPassword.c_str(), "disconnected", ethClient);
        }else{ // auto
            mqtt_obj.config((const char *)mqttServer.c_str(), (int)mqttPort, (const char *)mqttUsername.c_str(), (const char *)mqttPassword.c_str(), "disconnected", ethClient);
        }
        yield();

        // Initialize and load subtopic configuration
        Serial.println("Initializing subtopic...");
        yield();
        // initSubtopic();
        
        mqtt_obj.setsubscribeto("#");

        // mqtt_obj.setsubtopic(subtopic);
        mqtt_obj.setCallback(mqttcallbackmain);
        mqtt_obj.setMacAddress(mac_str);
    }
    mqttPref.end();
    yield();

    rs485ModbusPref.begin("rs485modbus", true);
    rs485ModbusEnabled = rs485ModbusPref.getBool("modbus_enabled", false);  // Cache the value
    if(rs485ModbusEnabled){
        Serial.println("Initializing Modbus RTU over RS485...");
        modbusInit();
    } else {
        Serial.println("Modbus RTU over RS485 is disabled in preferences.");
    }
    rs485ModbusPref.end();
    yield(); // Feed watchdog

    // Initialize Serial Port (UART2) if enabled
    serialportPref.begin(SERIALPORT_PREF_NS, true);
    serialportEnabled = serialportPref.getBool("enabled", false);
    serialportPref.end();
    if (serialportEnabled) {
        Serial.println("Initializing Serial Port (UART2)...");
        serialPortInit();
    } else {
        Serial.println("Serial Port (UART2) disabled in preferences.");
    }
    yield(); // Feed watchdog

    // Initialize Shift Details
    shiftDetailsInit();
    yield();

    // Don't start web servers here - start them after network is ready
    // setupWebServer(); 
    // setupSyncWebServer();

    // Sync RTC time to HMI display
    HMI.Time_Stamp(
        rtc.getDay(),
        rtc.getMonth(),
        (uint8_t)(rtc.getYear() % 100),  // 2-digit year
        rtc.getHour(),
        rtc.getMinute(),
        rtc.getSecond()
    );
    
    Serial.println("Board initialization complete!");
    yield();
}




void boardloop(){

    static unsigned long boardloop_count = 0;
    static unsigned long last_boardloop_print = 0;
    // WiFi init state machine: 0=pending, 1=mode_set, 2=get_mac, 3=disconnect, 4=connect, 5=done
    static uint8_t wifi_init_state = 0;
    static unsigned long wifi_init_timer = 0;
    boardloop_count++;

    // External reconnect request (e.g. from wifiApplyCredentials)
    if (_wifiReconnectRequested) {
        _wifiReconnectRequested = false;
        wifi_connected = false;
        WiFi.disconnect(false);  // keep radio on — avoids 3+ second radio shutdown
        wifi_init_state = 0;     // restart init state machine with new credentials
    }
    
    
    unsigned long boardloop_start = millis();

    yield(); // Feed watchdog at start
    
    // if (ethManager.spiMutex) xSemaphoreTake(ethManager.spiMutex, portMAX_DELAY);


    if(ethWebServerStarted) {
        handleEthWeb();
    }

    modbusLoop(); // Disabled - ModbusRTU causes FreeRTOS assert failed: xQueueSemaphoreTake queue.c:1713
    
    // Non-blocking WiFi initialization state machine
    if (wifiEnabled && wifi_init_state < 5) {
        switch (wifi_init_state) {
            case 0: // Set WiFi mode
                Serial.println("\n[WiFi] Starting deferred WiFi initialization...");
                WiFi.mode(WIFI_STA);
                WiFi.setSleep(false);  // Disable modem sleep — prevents 100-400ms ping latency
                wifi_init_timer = millis();
                wifi_init_state = 1;
                break;

            case 1: // Wait for mode change to settle, then get MAC
                if (millis() - wifi_init_timer >= 1000) {
                    WiFi.macAddress(mac);
                    mac_str = WiFi.macAddress();
                    Serial.print("[WiFi] MAC: ");
                    for (int i = 0; i < 6; i++) {
                        if (mac[i] < 16) Serial.print("0");
                        Serial.print(mac[i], HEX);
                        if (i < 5) Serial.print(":");
                    }
                    Serial.println();
                    wifi_init_timer = millis();
                    wifi_init_state = 2;
                }
                break;

            case 2: // Disconnect any stale connection
                if (millis() - wifi_init_timer >= 200) {
                    WiFi.disconnect();
                    wifi_init_timer = millis();
                    wifi_init_state = 3;
                }
                break;

            case 3: // Wait after disconnect, then connect
                if (millis() - wifi_init_timer >= 500) {
                    String ssid = wifiPref.getString("ssid", "");
                    String password = wifiPref.getString("password", "");
                    bool useDHCP = wifiPref.getBool("dhcp", true);
                    
                    if (ssid.length() == 0) {
                        Serial.println("[WiFi] ✗ No SSID configured - skipping connection.");
                        wifi_init_state = 5; // Skip to done
                        break;
                    }
                    
                    Serial.print("[WiFi] Connecting to: "); Serial.println(ssid);
                    Serial.printf("[WiFi] Mode: %s\n", useDHCP ? "DHCP" : "Static IP");
                    
                    // Configure static IP if DHCP is disabled
                    if (!useDHCP) {
                        String ipStr = wifiPref.getString("ip", "");
                        String gatewayStr = wifiPref.getString("gateway", "");
                        String subnetStr = wifiPref.getString("subnet", "");
                        String dnsStr = wifiPref.getString("dns", "");
                        
                        // Validate that IP address is configured
                        if (ipStr.length() == 0) {
                            Serial.println("[WiFi] ✗ Error: Static IP mode enabled but no IP address configured!");
                            Serial.println("[WiFi] ✗ Use 'wifi ip <address>' to set IP or 'wifi dhcp' to enable DHCP.");
                            Serial.println("[WiFi] ✗ Connection aborted.");
                            wifi_init_state = 5; // Skip to done without connecting
                            break;
                        }
                        
                        IPAddress ip, gateway, subnet, dns;
                        
                        // Parse and validate IP address
                        if (!ip.fromString(ipStr)) {
                            Serial.printf("[WiFi] ✗ Error: Invalid IP address format: %s\n", ipStr.c_str());
                            Serial.println("[WiFi] ✗ Connection aborted.");
                            wifi_init_state = 5;
                            break;
                        }
                        
                        // Use defaults if not set, otherwise validate
                        if (gatewayStr.length() > 0) {
                            if (!gateway.fromString(gatewayStr)) {
                                Serial.printf("[WiFi] ✗ Error: Invalid gateway format: %s\n", gatewayStr.c_str());
                                Serial.println("[WiFi] ✗ Connection aborted.");
                                wifi_init_state = 5;
                                break;
                            }
                        } else {
                            gateway = IPAddress(192, 168, 1, 1); // Default gateway
                        }
                        
                        if (subnetStr.length() > 0) {
                            if (!subnet.fromString(subnetStr)) {
                                Serial.printf("[WiFi] ✗ Error: Invalid subnet mask format: %s\n", subnetStr.c_str());
                                Serial.println("[WiFi] ✗ Connection aborted.");
                                wifi_init_state = 5;
                                break;
                            }
                        } else {
                            subnet = IPAddress(255, 255, 255, 0); // Default subnet
                        }
                        
                        if (dnsStr.length() > 0) {
                            if (!dns.fromString(dnsStr)) {
                                Serial.printf("[WiFi] ✗ Error: Invalid DNS format: %s\n", dnsStr.c_str());
                                Serial.println("[WiFi] ✗ Connection aborted.");
                                wifi_init_state = 5;
                                break;
                            }
                        } else {
                            dns = gateway; // Use gateway as DNS by default
                        }
                        
                        // Apply static IP configuration
                        if (!WiFi.config(ip, dns, gateway, subnet)) {
                            Serial.println("[WiFi] ✗ Failed to configure static IP settings!");
                            Serial.println("[WiFi] ✗ Connection aborted.");
                            wifi_init_state = 5;
                            break;
                        }
                        
                        Serial.printf("[WiFi] Static IP configured:\n");
                        Serial.printf("  IP:      %s\n", ip.toString().c_str());
                        Serial.printf("  Gateway: %s\n", gateway.toString().c_str());
                        Serial.printf("  Subnet:  %s\n", subnet.toString().c_str());
                        Serial.printf("  DNS:     %s\n", dns.toString().c_str());
                    }
                    
                    // Proceed with connection
                    WiFi.begin(ssid.c_str(), password.c_str());
                    wifi_init_state = 4;
                    wifi_init_timer = millis();
                }
                break;

            case 4: // Done
                if (millis() - wifi_init_timer >= 100) {
                    Serial.println("[WiFi] Initialization complete.");
                    wifi_init_state = 5;
                }
                break;
        }
    }
    
    handleSerialCommands();
    yield(); // Feed after serial
    yield(); // Feed watchdog between operations
    
    // TCP Modbus loop
    tcpModbusLoop();
    yield(); // Feed after Modbus
    yield(); // Feed watchdog after Modbus
    
    // Web server handling - only use sync server for Ethernet
    if(ethernetEnabled){  // Use cached value instead of reading from NVS
        // Ethernet HTTP is handled by handleEthWebClients() in web_configuration.h
    }
    
    
    yield(); // Feed watchdog after web server
    
    if(onesecloop.ontime()){
        // Place 1-second interval tasks here
        if(ethernetEnabled){  // Use cached value instead of reading from NVS
            if (ethManager.status() == 1){
                if (conn_status == 0){
                    conn_status = 1;
                    
                    Serial.println(Ethernet.localIP());
                    setupEthWebServer();
                    if(mqttEnabled){
                        if(mqttTransport == "ethernet" || mqttTransport == "auto"){
                            mqtt_obj.setClient(ethClient);
                        }
                    }

                    if(tcpModbusEnabled) {
                        // W5500-compatible Modbus implementation
                        if (!tcpModbusIsRunning()) {
                            Serial.println("Initializing TCP Modbus on Ethernet...");
                            
                            if(tcpModbusInit()) {
                                Serial.println("TCP Modbus initialized successfully on Ethernet.");
                            } else {
                                Serial.println("TCP Modbus initialization failed.");
                            }
                            delay(10); // Feed watchdog
                        } else {
                            Serial.println("TCP Modbus already running.");
                        }
                    }

                    // HMI.Write_UString(PS_INFO,Ethernet.localIP().toString().c_str()); 
                }
            }else{
                conn_status = 0;
            }
        }
        if(mqtt_connected){
            toggleLED(2);
        }else if(wifi_connected || conn_status == 1){
            toggleLED(1);
        }else{
            toggleLED(0);
        }
        // Release heap held by disconnected WebSocket clients every second.
        // Omitting this call causes every browser tab/reload to permanently
        // consume heap until the device crashes with OOM inside AsyncTCP.
        if(webServerStarted) ws.cleanupClients();
        yield(); // Feed watchdog after LED toggle
    }
    yield(); // Feed after onesecloop

    // handleWebServer();  // Don't use AsyncWebServer with Ethernet
    // syncServer.handleClient();  // Moved to top

    if(ms_100loop.ontime()){
        yield(); // Feed before mqtt operations
        // Place 100-millisecond interval tasks here
        if (conn_status > 0 || wifi_connected){
            execution_timer = millis();
            if(mqttEnabled){
                static bool prev_mqtt_connected = false;
                if(mqttTransport == "wifi" || mqttTransport == "auto"){ // wifi or auto
                    // For "wifi" transport, only proceed when WiFi link is actually up.
                    // Prevents repeated TCP connect attempts (blocking ~1 s each) when WiFi
                    // is down but Ethernet or a stale wifi_connected flag passes the outer guard.
                    if (mqttTransport == "wifi" && WiFi.status() != WL_CONNECTED) {
                        mqtt_connected = false;
                        prev_mqtt_connected = false;  // re-arm "just connected" event for next connect
                    } else {
                    mqtt_obj.loop();
                    mqtt_connected = (mqtt_obj.connectionStatus() == MQTT_CONNECTED);
                    // Publish peripheral status on fresh MQTT connection
                    if (mqtt_connected && !prev_mqtt_connected) {
                        Serial.println("[MQTT] Connected - publishing peripheral status...");
                        publishPeripheralStatus();

                        DynamicJsonDocument status(100);
    
                        status["status"] = "connected";
                        status["timestamp"] = rtc.getDateTime();
                        char buffer[200];
                        serializeJsonPretty(status, buffer);
                        Serial.println(buffer);
                        mqtt_obj.publish(mqtt_obj.getTopic("events/connection_status").c_str() , buffer,true) ;
                        mqtt_obj.publish(mqtt_obj.getTopic("status").c_str() , buffer) ;
                    }
                    prev_mqtt_connected = mqtt_connected;
                    } // end WiFi-status guard
                }else if(mqttTransport == "ethernet"){
                    // With Ethernet transport, only loop MQTT if Ethernet is connected
                    if (conn_status == 1) {
                        mqtt_obj.loop();
                        mqtt_connected = (mqtt_obj.connectionStatus() == MQTT_CONNECTED);
                        // Publish peripheral status on fresh MQTT connection
                        if (mqtt_connected && !prev_mqtt_connected) {
                            Serial.println("[MQTT] Connected - publishing peripheral status...");
                            publishPeripheralStatus();

                            DynamicJsonDocument status(100);
    
                            status["status"] = "connected";
                            status["timestamp"] = rtc.getDateTime();
                            char buffer[200];
                            serializeJsonPretty(status, buffer);
                            Serial.println(buffer);
                            mqtt_obj.publish(mqtt_obj.getTopic("events/connection_status").c_str() , buffer,true) ;
                            mqtt_obj.publish(mqtt_obj.getTopic("status").c_str() , buffer) ;
                        }
                        prev_mqtt_connected = mqtt_connected;
                    } else {
                        mqtt_connected = false; // Ensure we update status if Ethernet disconnects
                    }
                }
            }
            unsigned long mqtt_time = millis() - execution_timer;
            if(mqtt_time > 500){
                Serial.print("⚠ Warning: MQTT loop execution time exceeded 500ms: ");
                Serial.print(mqtt_time);
                Serial.println(" ms");
            }
        }
            
        if(wifiEnabled){  // Use cached value instead of reading from NVS
            execution_timer = millis();
            if(WiFi.status() == WL_CONNECTED ){
                if(wifi_connected == false){
                    wifi_connected = true;

                    // Re-apply no-sleep every time the link comes up.
                    // WiFi.begin() can re-enable modem sleep internally.
                    WiFi.setSleep(false);
                    WiFi.setTxPower(WIFI_POWER_19_5dBm);

                    Serial.println("[WiFi] ✓ Connected successfully!");
                    Serial.print("[WiFi] IP Address: ");
                    Serial.println(WiFi.localIP());

                    setupWebServer();
                    Serial.println("[Web] Async web server started for WiFi");
                    if (wifi_connected_callback) wifi_connected_callback();

                    if(mqttEnabled){
                        if(mqttTransport == "wifi" || mqttTransport == "auto"){
                            mqtt_obj.setClient(wifiClient);
                        }
                    } 

                    if(tcpModbusEnabled) {
                        // Only initialize if not already initialized (from Ethernet section)
                        if (!tcpModbusIsRunning()) {
                            Serial.println("Initializing TCP Modbus on WiFi...");
                            
                            if(tcpModbusInit()) {
                                Serial.println("TCP Modbus initialized successfully on WiFi.");
                            } else {
                                Serial.println("TCP Modbus initialization failed.");
                            }
                            delay(10); // Feed watchdog
                        } else {
                            Serial.println("TCP Modbus already running.");
                        }
                    } 
                }

                // ── WiFi connectivity watchdog ─────────────────────────────────
                // Ping the gateway every 60 s in a background FreeRTOS task so
                // the main loop is never blocked by the ~1 s ICMP timeout.
                static unsigned long    wifi_ping_timer    = 0;
                static uint8_t          wifi_ping_failures = 0;
                static volatile int8_t  _pingResult        = -1;  // -1=idle 0=fail 1=ok
                static TaskHandle_t     _pingTaskHandle    = NULL;
                static IPAddress        _pingGW;

                // Launch ping task when interval expires and no task is running.
                // After a failure, retry every 10 s instead of 60 s so we
                // detect recovery quickly without waiting a full minute.
                unsigned long _pingInterval = (wifi_ping_failures > 0) ? 10000UL : 60000UL;
                if (millis() - wifi_ping_timer >= _pingInterval &&
                    _pingResult == -1 && _pingTaskHandle == NULL)
                {
                    wifi_ping_timer = millis();
                    IPAddress gw = WiFi.gatewayIP();
                    if (gw[0] != 0) {
                        _pingGW = gw;
                        xTaskCreate([](void*) {
                            bool ok = Ping.ping(_pingGW, 1);
                            _pingResult = ok ? 1 : 0;
                            _pingTaskHandle = NULL;
                            vTaskDelete(NULL);
                        }, "wifi_ping", 4096, NULL, 1, &_pingTaskHandle);
                    }
                }

                // Consume result when the background task has finished.
                if (_pingResult >= 0) {
                    bool pingOk = (_pingResult == 1);
                    _pingResult = -1;

                    if (pingOk) {
                        if (wifi_ping_failures > 0)
                            Serial.println("[WiFi] Gateway ping OK — connection healthy");
                        wifi_ping_failures = 0;
                    } else {
                        wifi_ping_failures++;
                        Serial.printf("[WiFi] \u26a0 Gateway ping failed (%u/3) \u2014 IP:%s GW:%s\n",
                            wifi_ping_failures,
                            WiFi.localIP().toString().c_str(),
                            WiFi.gatewayIP().toString().c_str());
                        if (wifi_ping_failures >= 3) {
                            Serial.println("[WiFi] \u26a0 Gateway unreachable \u2014 forcing WiFi reconnect...");
                            wifi_ping_failures = 0;
                            wifi_ping_timer    = millis(); // wait full interval before next ping
                            wifi_connected     = false;
                            WiFi.disconnect(false);        // deassociate only — keep radio on
                            wifi_init_state    = 3;        // skip mode/MAC delays, go straight to connect
                            // wifi_init_timer intentionally not reset: state 3 checks
                            // (millis() - wifi_init_timer >= 500); the old stale value
                            // guarantees the connect fires on the very next loop iteration.
                        }
                    }
                }
                // ──────────────────────────────────────────────────────────────

            }else{
                if (wifi_connected) {
                    // WiFi just dropped — schedule an automatic reconnect.
                    Serial.println("[WiFi] \u2718 Disconnected \u2014 scheduling reconnect...");
                    WiFi.disconnect(false);  // ensure radio is cleanly reset before reconnect
                    wifi_init_timer = 0;     // makes state 3 fire immediately on next boardloop()
                    wifi_init_state = 3;
                }
                wifi_connected = false;
            }
            if(millis() - execution_timer > 500){
                Serial.print("⚠ Warning: WiFi loop execution time exceeded 500ms: ");
                Serial.print(millis() - execution_timer);
                Serial.println(" ms");
            }
        }
        yield(); // Feed after WiFi check
    }
    yield(); // Feed after ms_100loop
    

    
    // Only print completion for iterations that take unusually long
    unsigned long boardloop_time = millis() - boardloop_start;
    if (boardloop_time > 500) {
        Serial.printf("[BOARDLOOP] #%lu took %lu ms\n", boardloop_count, boardloop_time);
    }
}

#endif // IOTBOARD_H
