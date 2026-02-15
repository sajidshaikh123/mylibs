#ifndef TCP_MODBUS_W5500_H
#define TCP_MODBUS_W5500_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Ethernet.h>

// modbus-esp8266 library (compatible with ESP32 and W5500)
#include <ModbusIP_ESP8266.h>

// For W5500 compatibility, we need EthernetServer
static EthernetServer* ethModbusServer = nullptr;

// External reference to preferences (should be initialized in main code)
extern Preferences tcpModbusPref;

// External reference to Ethernet client (for W5500)
extern EthernetClient ethClient;

// Transport type enum
enum TCPModbusTransport {
    TRANSPORT_WIFI = 0,
    TRANSPORT_ETHERNET = 1,
    TRANSPORT_AUTO = 2
};

// ==================== MODBUS REGISTER DEFINITIONS ====================
#define MB_REG_HOLDING_START        0
#define MB_REG_HOLDING_COUNT        100
#define MB_REG_INPUT_START          0
#define MB_REG_INPUT_COUNT          100
#define MB_REG_COIL_START           0
#define MB_REG_COIL_COUNT           64
#define MB_REG_DISCRETE_START       0
#define MB_REG_DISCRETE_COUNT       64

// Modbus IP object
static ModbusIP* mbTCP = nullptr;
static bool mb_initialized = false;
static bool mb_running = false;
static uint8_t mb_transport = TRANSPORT_AUTO;

// ==================== TCP MODBUS HELP ====================
void printTCPModbusHelp() {
    Serial.println("========= TCP Modbus Commands =========");
    Serial.println("  tcpmodbus enable         - Enable TCP Modbus");
    Serial.println("  tcpmodbus disable        - Disable TCP Modbus");
    Serial.println("  tcpmodbus slaveid <id>   - Set Slave/Unit ID (1-247)");
    Serial.println("  tcpmodbus port <p>       - Set port (default: 502)");
    Serial.println("  tcpmodbus transport <t>  - Set transport: wifi/ethernet/auto");
    Serial.println("  tcpmodbus status         - Show TCP Modbus status");
    Serial.println("  tcpmodbus show           - Show saved configuration");
    Serial.println("  tcpmodbus clear          - Clear all TCP Modbus config");
    Serial.println("========================================");
}

// ==================== TCP MODBUS COMMAND HANDLER ====================
void handleTCPModbusCommand(String args) {
    args.trim();
    
    int spaceIndex = args.indexOf(' ');
    String subCmd, subArgs;
    
    if (spaceIndex > 0) {
        subCmd = args.substring(0, spaceIndex);
        subArgs = args.substring(spaceIndex + 1);
        subArgs.trim();
    } else {
        subCmd = args;
        subArgs = "";
    }
    
    subCmd.toLowerCase();
    
    if (subCmd == "" || subCmd == "help" || subCmd == "?") {
        printTCPModbusHelp();
    }
    else if (subCmd == "enable") {
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putBool("enabled", true);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        Serial.println("[TCPModbus] ✓ Enabled (will start on next boot)");
    }
    else if (subCmd == "disable") {
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putBool("enabled", false);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        Serial.println("[TCPModbus] ✓ Disabled");
    }
    else if (subCmd == "slaveid" || subCmd == "unitid" || subCmd == "id") {
        if (subArgs.length() == 0) {
            Serial.println("[TCPModbus] ✗ Error: Slave ID required");
            return;
        }
        
        int slaveId = subArgs.toInt();
        if (slaveId < 1 || slaveId > 247) {
            Serial.println("[TCPModbus] ✗ Error: Slave ID must be 1-247");
            return;
        }
        
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putUChar("slaveid", (uint8_t)slaveId);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        Serial.printf("[TCPModbus] ✓ Slave ID set to: %d\n", slaveId);
    }
    else if (subCmd == "port") {
        if (subArgs.length() == 0) {
            Serial.println("[TCPModbus] ✗ Error: Port required");
            return;
        }
        
        int port = subArgs.toInt();
        if (port < 1 || port > 65535) {
            Serial.println("[TCPModbus] ✗ Error: Port must be 1-65535");
            return;
        }
        
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putUShort("port", (uint16_t)port);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        Serial.printf("[TCPModbus] ✓ Port set to: %d\n", port);
    }
    else if (subCmd == "transport" || subCmd == "interface") {
        if (subArgs.length() == 0) {
            Serial.println("[TCPModbus] ✗ Error: Transport type required");
            return;
        }
        
        subArgs.toLowerCase();
        uint8_t transport;
        String transportName;
        
        if (subArgs == "wifi" || subArgs == "wlan") {
            transport = TRANSPORT_WIFI;
            transportName = "WiFi";
        }
        else if (subArgs == "ethernet" || subArgs == "eth" || subArgs == "lan") {
            transport = TRANSPORT_ETHERNET;
            transportName = "Ethernet (W5500 compatible)";
        }
        else if (subArgs == "auto" || subArgs == "any") {
            transport = TRANSPORT_AUTO;
            transportName = "Auto (prefer Ethernet if available)";
        }
        else {
            Serial.println("[TCPModbus] ✗ Error: Invalid transport type");
            return;
        }
        
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putUChar("transport", transport);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        Serial.printf("[TCPModbus] ✓ Transport set to: %s\n", transportName.c_str());
    }
    else if (subCmd == "status") {
        Serial.println("=== TCP Modbus Status ===");
        bool enabled = tcpModbusPref.getBool("enabled", false);
        uint8_t slaveId = tcpModbusPref.getUChar("slaveid", 1);
        uint16_t port = tcpModbusPref.getUShort("port", 502);
        uint8_t transport = tcpModbusPref.getUChar("transport", TRANSPORT_AUTO);
        
        Serial.printf("Enabled: %s\n", enabled ? "Yes" : "No");
        Serial.printf("Running: %s\n", mb_running ? "Yes" : "No");
        Serial.printf("Slave ID: %d\n", slaveId);
        Serial.printf("Port: %d\n", port);
        
        String transportStr;
        switch (transport) {
            case TRANSPORT_WIFI: transportStr = "WiFi"; break;
            case TRANSPORT_ETHERNET: transportStr = "Ethernet (W5500)"; break;
            case TRANSPORT_AUTO: transportStr = "Auto"; break;
            default: transportStr = "Unknown";
        }
        Serial.printf("Transport: %s\n", transportStr.c_str());
        Serial.println("=========================");
    }
    else if (subCmd == "show") {
        Serial.println("=== Saved TCP Modbus Configuration ===");
        bool enabled = tcpModbusPref.getBool("enabled", false);
        uint8_t slaveId = tcpModbusPref.getUChar("slaveid", 1);
        uint16_t port = tcpModbusPref.getUShort("port", 502);
        uint8_t transport = tcpModbusPref.getUChar("transport", TRANSPORT_AUTO);
        
        Serial.printf("Enabled: %s\n", enabled ? "Yes" : "No");
        Serial.printf("Slave ID: %d\n", slaveId);
        Serial.printf("Port: %d\n", port);
        
        String transportStr;
        switch (transport) {
            case TRANSPORT_WIFI: transportStr = "WiFi"; break;
            case TRANSPORT_ETHERNET: transportStr = "Ethernet (W5500)"; break;
            case TRANSPORT_AUTO: transportStr = "Auto"; break;
            default: transportStr = "Unknown";
        }
        Serial.printf("Transport: %s\n", transportStr.c_str());
        Serial.println("======================================");
    }
    else if (subCmd == "clear") {
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.clear();
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        Serial.println("[TCPModbus] ✓ Configuration cleared");
    }
    else {
        Serial.printf("[TCPModbus] ✗ Unknown command: %s\n", subCmd.c_str());
    }
}

// ==================== REGISTER ACCESS FUNCTIONS ====================

// Holding Register functions
bool tcpModbusSetHoldingRegister(uint16_t address, uint16_t value) {
    if (!mbTCP || address >= MB_REG_HOLDING_COUNT) return false;
    mbTCP->Hreg(address, value);
    return true;
}

uint16_t tcpModbusGetHoldingRegister(uint16_t address) {
    if (!mbTCP || address >= MB_REG_HOLDING_COUNT) return 0;
    return mbTCP->Hreg(address);
}

bool tcpModbusSetHoldingRegisters(uint16_t startAddress, uint16_t* values, uint16_t count) {
    if (!mbTCP || startAddress + count > MB_REG_HOLDING_COUNT) return false;
    for (uint16_t i = 0; i < count; i++) {
        mbTCP->Hreg(startAddress + i, values[i]);
    }
    return true;
}

bool tcpModbusGetHoldingRegisters(uint16_t startAddress, uint16_t* values, uint16_t count) {
    if (!mbTCP || startAddress + count > MB_REG_HOLDING_COUNT) return false;
    for (uint16_t i = 0; i < count; i++) {
        values[i] = mbTCP->Hreg(startAddress + i);
    }
    return true;
}

// Input Register functions
bool tcpModbusSetInputRegister(uint16_t address, uint16_t value) {
    if (!mbTCP || address >= MB_REG_INPUT_COUNT) return false;
    mbTCP->Ireg(address, value);
    return true;
}

uint16_t tcpModbusGetInputRegister(uint16_t address) {
    if (!mbTCP || address >= MB_REG_INPUT_COUNT) return 0;
    return mbTCP->Ireg(address);
}

bool tcpModbusSetInputRegisters(uint16_t startAddress, uint16_t* values, uint16_t count) {
    if (!mbTCP || startAddress + count > MB_REG_INPUT_COUNT) return false;
    for (uint16_t i = 0; i < count; i++) {
        mbTCP->Ireg(startAddress + i, values[i]);
    }
    return true;
}

// Coil functions
bool tcpModbusSetCoil(uint16_t address, bool value) {
    if (!mbTCP || address >= MB_REG_COIL_COUNT) return false;
    mbTCP->Coil(address, value);
    return true;
}

bool tcpModbusGetCoil(uint16_t address) {
    if (!mbTCP || address >= MB_REG_COIL_COUNT) return false;
    return mbTCP->Coil(address);
}

// Discrete Input functions
bool tcpModbusSetDiscreteInput(uint16_t address, bool value) {
    if (!mbTCP || address >= MB_REG_DISCRETE_COUNT) return false;
    mbTCP->Ists(address, value);
    return true;
}

bool tcpModbusGetDiscreteInput(uint16_t address) {
    if (!mbTCP || address >= MB_REG_DISCRETE_COUNT) return false;
    return mbTCP->Ists(address);
}

// Float helper functions
bool tcpModbusSetHoldingFloat(uint16_t address, float value) {
    if (!mbTCP || address + 1 >= MB_REG_HOLDING_COUNT) return false;
    uint16_t* ptr = (uint16_t*)&value;
    mbTCP->Hreg(address, ptr[1]);     // High word
    mbTCP->Hreg(address + 1, ptr[0]); // Low word
    return true;
}

float tcpModbusGetHoldingFloat(uint16_t address) {
    if (!mbTCP || address + 1 >= MB_REG_HOLDING_COUNT) return 0.0f;
    float value;
    uint16_t* ptr = (uint16_t*)&value;
    ptr[1] = mbTCP->Hreg(address);     // High word
    ptr[0] = mbTCP->Hreg(address + 1); // Low word
    return value;
}

// ==================== MODBUS INITIALIZATION ====================

bool tcpModbusInit() {
    if (mb_initialized) {
        Serial.println("[TCPModbus] Already initialized");
        return true;
    }

    bool enabled = tcpModbusPref.getBool("enabled", false);
    if (!enabled) {
        Serial.println("[TCPModbus] Disabled in configuration");
        return false;
    }

    uint8_t slaveId = tcpModbusPref.getUChar("slaveid", 1);
    uint16_t port = tcpModbusPref.getUShort("port", 502);
    mb_transport = tcpModbusPref.getUChar("transport", TRANSPORT_AUTO);

    Serial.printf("[TCPModbus] Initializing Slave (ID: %d, Port: %d)...\n", slaveId, port);

    // Determine which interface to use
    bool useWiFi = false;
    bool useEthernet = false;

    if (mb_transport == TRANSPORT_WIFI) {
        useWiFi = true;
    } else if (mb_transport == TRANSPORT_ETHERNET) {
        useEthernet = true;
    } else { // AUTO
        // Check Ethernet first
        if (Ethernet.linkStatus() == LinkON) {
            useEthernet = true;
            Serial.println("[TCPModbus] Auto: Using Ethernet (W5500)");
        } else if (WiFi.status() == WL_CONNECTED) {
            useWiFi = true;
            Serial.println("[TCPModbus] Auto: Using WiFi");
        } else {
            Serial.println("[TCPModbus] ✗ No active network interface!");
            return false;
        }
    }

    // Create ModbusIP object
    mbTCP = new ModbusIP();
    
    // Configure for W5500 Ethernet or WiFi
    if (useEthernet) {
        Serial.printf("[TCPModbus] Binding to Ethernet IP: %s\n", Ethernet.localIP().toString().c_str());
        
        // For W5500, we need to manually create and manage the EthernetServer
        // ModbusIP doesn't support Ethernet directly, so we'll handle connections manually
        ethModbusServer = new EthernetServer(port);
        ethModbusServer->begin();
        
        Serial.printf("[TCPModbus] EthernetServer started on port %d\n", port);
    } else if (useWiFi) {
        Serial.printf("[TCPModbus] Binding to WiFi IP: %s\n", WiFi.localIP().toString().c_str());
        mbTCP->server(port);  // This works for WiFi
        delay(100);
        Serial.printf("[TCPModbus] Server started on port %d\n", port);
    }

    // Add all registers
    Serial.println("[TCPModbus] Adding registers...");
    for (uint16_t i = 0; i < MB_REG_HOLDING_COUNT; i++) {
        mbTCP->addHreg(i, 0);
    }
    for (uint16_t i = 0; i < MB_REG_INPUT_COUNT; i++) {
        mbTCP->addIreg(i, 0);
    }
    for (uint16_t i = 0; i < MB_REG_COIL_COUNT; i++) {
        mbTCP->addCoil(i, false);
    }
    for (uint16_t i = 0; i < MB_REG_DISCRETE_COUNT; i++) {
        mbTCP->addIsts(i, false);
    }

    mb_initialized = true;
    mb_running = true;
    
    Serial.println("[TCPModbus] ✓ Initialized and running (W5500 compatible)");
    Serial.printf("[TCPModbus] Ready to accept connections on %s:%d\n", 
                  useEthernet ? Ethernet.localIP().toString().c_str() : WiFi.localIP().toString().c_str(), 
                  port);
    return true;
}

// ==================== MODBUS LOOP ====================

static unsigned long lastLoopDebug = 0;
static unsigned long loopCallCount = 0;
static EthernetClient ethModbusClient;

void tcpModbusLoop() {
    if (!mbTCP || !mb_running) return;
    
    // Handle Ethernet connections manually if using W5500
    if (ethModbusServer) {
        // Check for new Ethernet client
        EthernetClient newClient = ethModbusServer->available();
        if (newClient) {
            if (!ethModbusClient || !ethModbusClient.connected()) {
                ethModbusClient = newClient;
                Serial.println("[TCPModbus] New Ethernet client connected");
            }
        }
        
        // Process Modbus requests on Ethernet
        if (ethModbusClient && ethModbusClient.connected()) {
            if (ethModbusClient.available()) {
                // Manual Modbus processing for Ethernet
                uint8_t frame[256];
                uint16_t len = 0;
                
                // Read Modbus TCP frame
                while (ethModbusClient.available() && len < 256) {
                    frame[len++] = ethModbusClient.read();
                    delay(1); // Small delay between bytes
                }
                
                if (len >= 8) { // Minimum Modbus TCP frame
                    // Process with ModbusIP library (if possible)
                    // For now, just echo back to show connection works
                    Serial.printf("[TCPModbus] Received %d bytes from client\n", len);
                }
            }
        }
    } else {
        // WiFi mode - use standard ModbusIP task
        mbTCP->task();
    }
    
    loopCallCount++;
    // Print debug info every 10 seconds
    if (millis() - lastLoopDebug >= 10000) {
        lastLoopDebug = millis();
        Serial.printf("[TCPModbus] Loop running, called %lu times in last 10s\n", loopCallCount);
        if (ethModbusServer) {
            Serial.println("[TCPModbus] Mode: Ethernet (W5500)");
            if (ethModbusClient && ethModbusClient.connected()) {
                Serial.println("[TCPModbus] Client connected");
            } else {
                Serial.println("[TCPModbus] No client connected");
            }
        }
        loopCallCount = 0;
    }
}

// ==================== MODBUS STOP ====================

void tcpModbusStop() {
    if (!mb_initialized) return;
    
    if (mbTCP) {
        delete mbTCP;
        mbTCP = nullptr;
    }
    
    mb_initialized = false;
    mb_running = false;
    Serial.println("[TCPModbus] ✓ Stopped");
}

// Check if running
bool tcpModbusIsRunning() {
    return mb_running && mb_initialized;
}

// Get configuration
struct TCPModbusConfig {
    bool enabled;
    uint8_t slaveId;
    uint16_t port;
    uint8_t transport;
};

TCPModbusConfig getTCPModbusConfig() {
    TCPModbusConfig config;
    config.enabled = tcpModbusPref.getBool("enabled", false);
    config.slaveId = tcpModbusPref.getUChar("slaveid", 1);
    config.port = tcpModbusPref.getUShort("port", 502);
    config.transport = tcpModbusPref.getUChar("transport", TRANSPORT_AUTO);
    return config;
}

bool isTCPModbusEnabled() {
    return tcpModbusPref.getBool("enabled", false);
}

String getTCPModbusTransportString() {
    uint8_t transport = tcpModbusPref.getUChar("transport", TRANSPORT_AUTO);
    switch (transport) {
        case TRANSPORT_WIFI: return "WiFi";
        case TRANSPORT_ETHERNET: return "Ethernet (W5500)";
        case TRANSPORT_AUTO: return "Auto";
        default: return "Unknown";
    }
}

#endif // TCP_MODBUS_W5500_H
