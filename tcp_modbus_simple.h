#ifndef TCP_MODBUS_SIMPLE_H
#define TCP_MODBUS_SIMPLE_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Ethernet.h>
#include "esp_task_wdt.h"

// External reference to preferences
extern Preferences tcpModbusPref;

bool debugTCPModbus = false;

std::function<void(uint16_t, uint16_t)> single_write_callback = nullptr;



// Transport type enum
enum TCPModbusTransport {
    TRANSPORT_WIFI = 0,
    TRANSPORT_ETHERNET = 1,
    TRANSPORT_AUTO = 2
};

// ==================== MODBUS REGISTER DEFINITIONS ====================
#define MB_REG_HOLDING_COUNT        100
#define MB_REG_INPUT_COUNT          100
#define MB_REG_COIL_COUNT           64
#define MB_REG_DISCRETE_COUNT       64

// Register storage
static uint16_t mb_holding_registers[MB_REG_HOLDING_COUNT] = {0};
static uint16_t mb_input_registers[MB_REG_INPUT_COUNT] = {0};
static uint8_t mb_coil_registers[(MB_REG_COIL_COUNT + 7) / 8] = {0};
static uint8_t mb_discrete_registers[(MB_REG_DISCRETE_COUNT + 7) / 8] = {0};

// Server state
static EthernetServer* ethModbusServer = nullptr;
static WiFiServer* wifiModbusServer = nullptr;
static EthernetClient mbEthClient;
static WiFiClient mbWifiClient;
static bool mb_initialized = false;
static bool mb_running = false;
static bool mb_use_ethernet = false;

// ==================== HELPER FUNCTIONS ====================

void setsingleWriteCallback(std::function<void(uint16_t, uint16_t)> callback) {
    single_write_callback = callback;
}

uint16_t modbusSwap16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

uint16_t modbusCRC16(uint8_t* buffer, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)buffer[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// ==================== REGISTER ACCESS FUNCTIONS ====================

bool tcpModbusSetHoldingRegister(uint16_t address, uint16_t value) {
    if (address >= MB_REG_HOLDING_COUNT) return false;
    mb_holding_registers[address] = value;
    return true;
}

uint16_t tcpModbusGetHoldingRegister(uint16_t address) {
    if (address >= MB_REG_HOLDING_COUNT) return 0;
    return mb_holding_registers[address];
}

bool tcpModbusSetHoldingRegisters(uint16_t startAddress, uint16_t* values, uint16_t count) {
    if (startAddress + count > MB_REG_HOLDING_COUNT) return false;
    memcpy(&mb_holding_registers[startAddress], values, count * sizeof(uint16_t));
    return true;
}

bool tcpModbusGetHoldingRegisters(uint16_t startAddress, uint16_t* values, uint16_t count) {
    if (startAddress + count > MB_REG_HOLDING_COUNT) return false;
    memcpy(values, &mb_holding_registers[startAddress], count * sizeof(uint16_t));
    return true;
}

bool tcpModbusSetInputRegister(uint16_t address, uint16_t value) {
    if (address >= MB_REG_INPUT_COUNT) return false;
    mb_input_registers[address] = value;
    return true;
}

uint16_t tcpModbusGetInputRegister(uint16_t address) {
    if (address >= MB_REG_INPUT_COUNT) return 0;
    return mb_input_registers[address];
}

bool tcpModbusSetCoil(uint16_t address, bool value) {
    if (address >= MB_REG_COIL_COUNT) return false;
    uint16_t byteIndex = address / 8;
    uint8_t bitIndex = address % 8;
    if (value) {
        mb_coil_registers[byteIndex] |= (1 << bitIndex);
    } else {
        mb_coil_registers[byteIndex] &= ~(1 << bitIndex);
    }
    return true;
}

bool tcpModbusGetCoil(uint16_t address) {
    if (address >= MB_REG_COIL_COUNT) return false;
    uint16_t byteIndex = address / 8;
    uint8_t bitIndex = address % 8;
    return (mb_coil_registers[byteIndex] >> bitIndex) & 0x01;
}

bool tcpModbusSetDiscreteInput(uint16_t address, bool value) {
    if (address >= MB_REG_DISCRETE_COUNT) return false;
    uint16_t byteIndex = address / 8;
    uint8_t bitIndex = address % 8;
    if (value) {
        mb_discrete_registers[byteIndex] |= (1 << bitIndex);
    } else {
        mb_discrete_registers[byteIndex] &= ~(1 << bitIndex);
    }
    return true;
}

bool tcpModbusGetDiscreteInput(uint16_t address) {
    if (address >= MB_REG_DISCRETE_COUNT) return false;
    uint16_t byteIndex = address / 8;
    uint8_t bitIndex = address % 8;
    return (mb_discrete_registers[byteIndex] >> bitIndex) & 0x01;
}

// Float helpers
bool tcpModbusSetHoldingFloat(uint16_t address, float value) {
    if (address + 1 >= MB_REG_HOLDING_COUNT) return false;
    uint16_t* ptr = (uint16_t*)&value;
    mb_holding_registers[address] = ptr[1];
    mb_holding_registers[address + 1] = ptr[0];
    return true;
}

float tcpModbusGetHoldingFloat(uint16_t address) {
    if (address + 1 >= MB_REG_HOLDING_COUNT) return 0.0f;
    float value;
    uint16_t* ptr = (uint16_t*)&value;
    ptr[1] = mb_holding_registers[address];
    ptr[0] = mb_holding_registers[address + 1];
    return value;
}

// ==================== MODBUS TCP FRAME PROCESSING ====================

void processModbusFrame(uint8_t* frame, uint16_t len, Client& client) {
    if (len < 8) return; // Too short for Modbus TCP
    
    yield(); // Feed watchdog at start
    
    // Modbus TCP frame: [TxID(2)][ProtID(2)][Len(2)][UnitID(1)][FC(1)][Data...]
    uint16_t txId = (frame[0] << 8) | frame[1];
    uint8_t unitId = frame[6];
    uint8_t funcCode = frame[7];
    
    uint8_t response[260];
    uint16_t responseLen = 0;
    
    // Copy header
    response[0] = frame[0]; // Transaction ID
    response[1] = frame[1];
    response[2] = 0x00; // Protocol ID
    response[3] = 0x00;
    response[6] = unitId;
    response[7] = funcCode;
    
    if (funcCode == 0x03) { // Read Holding Registers
        yield(); // Feed watchdog before processing
        uint16_t startAddr = (frame[8] << 8) | frame[9];
        uint16_t numRegs = (frame[10] << 8) | frame[11];
        
        if (startAddr + numRegs <= MB_REG_HOLDING_COUNT) {
            response[8] = numRegs * 2; // Byte count
            responseLen = 9;
            
            for (uint16_t i = 0; i < numRegs; i++) {
                if (i % 20 == 0) yield(); // Feed watchdog every 20 registers
                uint16_t val = mb_holding_registers[startAddr + i];
                response[responseLen++] = (val >> 8) & 0xFF;
                response[responseLen++] = val & 0xFF;
            }
            
            // Set length
            uint16_t pduLen = responseLen - 6;
            response[4] = (pduLen >> 8) & 0xFF;
            response[5] = pduLen & 0xFF;
            
            client.write(response, responseLen);
            if (debugTCPModbus){
                Serial.printf("[Modbus] Read Holding Regs: addr=%d, count=%d\n", startAddr, numRegs);
            }
        }
    }
    else if (funcCode == 0x04) { // Read Input Registers
        yield(); // Feed watchdog before processing
        uint16_t startAddr = (frame[8] << 8) | frame[9];
        uint16_t numRegs = (frame[10] << 8) | frame[11];
        
        if (startAddr + numRegs <= MB_REG_INPUT_COUNT) {
            response[8] = numRegs * 2;
            responseLen = 9;
            
            for (uint16_t i = 0; i < numRegs; i++) {
                if (i % 20 == 0) yield(); // Feed watchdog every 20 registers
                uint16_t val = mb_input_registers[startAddr + i];
                response[responseLen++] = (val >> 8) & 0xFF;
                response[responseLen++] = val & 0xFF;
            }
            
            uint16_t pduLen = responseLen - 6;
            response[4] = (pduLen >> 8) & 0xFF;
            response[5] = pduLen & 0xFF;
            
            client.write(response, responseLen);
            if (debugTCPModbus){
                Serial.printf("[Modbus] Read Input Regs: addr=%d, count=%d\n", startAddr, numRegs);
            }
        }
    }
    else if (funcCode == 0x06) { // Write Single Register
        yield(); // Feed watchdog before processing
        uint16_t addr = (frame[8] << 8) | frame[9];
        uint16_t value = (frame[10] << 8) | frame[11];
        
        if (addr < MB_REG_HOLDING_COUNT) {
            mb_holding_registers[addr] = value;
            client.write(frame, 12); // Echo request
            if (debugTCPModbus){
                Serial.printf("[Modbus] Write Reg: addr=%d, value=%d\n", addr, value);
            }
            if(single_write_callback){
                single_write_callback(addr, value);
            }
        }
    }
    else if (funcCode == 0x10) { // Write Multiple Registers
        yield(); // Feed watchdog before processing
        uint16_t startAddr = (frame[8] << 8) | frame[9];
        uint16_t numRegs = (frame[10] << 8) | frame[11];
        uint8_t byteCount = frame[12];
        
        if (startAddr + numRegs <= MB_REG_HOLDING_COUNT && byteCount == numRegs * 2) {
            for (uint16_t i = 0; i < numRegs; i++) {
                if (i % 20 == 0) yield(); // Feed watchdog every 20 registers
                uint16_t val = (frame[13 + i * 2] << 8) | frame[14 + i * 2];
                mb_holding_registers[startAddr + i] = val;
            }
            
            // Send response (echo first 12 bytes)
            responseLen = 12;
            memcpy(response, frame, 12);
            response[4] = 0x00;
            response[5] = 0x06;
            
            client.write(response, responseLen);
            if (debugTCPModbus){
                Serial.printf("[Modbus] Write Multiple Regs: addr=%d, count=%d\n", startAddr, numRegs);
            }
        }
    }
    
    yield(); // Feed watchdog at end of processing
}

// ==================== INITIALIZATION ====================

bool tcpModbusInit() {
    if (mb_initialized) {
        Serial.println("[TCPModbus] Already initialized");
        return true;
    }

    bool enabled = tcpModbusPref.getBool("enabled", false);
    if (!enabled) {
        Serial.println("[TCPModbus] Disabled");
        return false;
    }

    uint16_t port = tcpModbusPref.getUShort("port", 502);
    uint8_t transport = tcpModbusPref.getUChar("transport", TRANSPORT_AUTO);

    bool useWiFi = false;
    bool useEthernet = false;

    if (transport == TRANSPORT_WIFI) {
        useWiFi = true;
    } else if (transport == TRANSPORT_ETHERNET) {
        useEthernet = true;
    } else { // AUTO
        if (Ethernet.linkStatus() == LinkON) {
            useEthernet = true;
        } else if (WiFi.status() == WL_CONNECTED) {
            useWiFi = true;
        } else {
            Serial.println("[TCPModbus] No network");
            return false;
        }
    }

    if (useEthernet) {
        ethModbusServer = new EthernetServer(port);
        ethModbusServer->begin();
        mb_use_ethernet = true;
        Serial.printf("[TCPModbus] Started on Ethernet %s:%d\n", Ethernet.localIP().toString().c_str(), port);
    } else {
        wifiModbusServer = new WiFiServer(port);
        wifiModbusServer->begin();
        mb_use_ethernet = false;
        Serial.printf("[TCPModbus] Started on WiFi %s:%d\n", WiFi.localIP().toString().c_str(), port);
    }

    mb_initialized = true;
    mb_running = true;
    return true;
}

// ==================== LOOP ====================

void tcpModbusLoop() {
    if (!mb_running) return;
    
    yield(); // Feed watchdog at start of loop
    
    // Aggressive throttling - check every 50ms to reduce blocking frequency
    static unsigned long lastCheck = 0;
    unsigned long now = millis();
    if (now - lastCheck < 50) { // Increased from 10ms to 50ms
        return;
    }
    lastCheck = now;
    
    // Watchdog managed by Arduino framework via yield() and delay()
    yield();

    if (mb_use_ethernet && ethModbusServer) {
        yield(); // Before checking for new clients
        EthernetClient newClient = ethModbusServer->available();
        yield(); // After checking for new clients
        
        if (newClient) {
            if (!mbEthClient || !mbEthClient.connected()) {
                mbEthClient = newClient;
                Serial.println("[TCPModbus] Client connected");
            }
        }

        yield(); // Before client operations
        if (mbEthClient && mbEthClient.connected()) {
            yield(); // Before available check
            if (mbEthClient.available()) {
                uint8_t frame[260];
                uint16_t len = 0;
                unsigned long startTime = millis();
                
                // Read with strict timeout - max 50ms
                while (mbEthClient.available() && len < 260 && (millis() - startTime < 50)) {
                    frame[len++] = mbEthClient.read();
                    delayMicroseconds(100); // Small delay between bytes
                    
                    // Feed watchdog every 10 bytes
                    if (len % 10 == 0) {
                        yield(); // Feed watchdog
                    }
                }
                
                if (len >= 8) {
                    processModbusFrame(frame, len, mbEthClient);
                }
            }
        }
    } else if (!mb_use_ethernet && wifiModbusServer) {
        yield(); // Before checking for new clients
        WiFiClient newClient = wifiModbusServer->available();
        yield(); // After checking for new clients
        
        if (newClient) {
            if (!mbWifiClient || !mbWifiClient.connected()) {
                mbWifiClient = newClient;
                Serial.println("[TCPModbus] Client connected");
            }
        }

        yield(); // Before client operations
        if (mbWifiClient && mbWifiClient.connected()) {
            yield(); // Before available check
            if (mbWifiClient.available()) {
                uint8_t frame[260];
                uint16_t len = 0;
                unsigned long startTime = millis();
                
                // Read with strict timeout - max 50ms
                while (mbWifiClient.available() && len < 260 && (millis() - startTime < 50)) {
                    frame[len++] = mbWifiClient.read();
                    delayMicroseconds(100);
                    
                    // Feed watchdog every 10 bytes
                    if (len % 10 == 0) {
                        yield(); // Feed watchdog
                    }
                }
                
                if (len >= 8) {
                    processModbusFrame(frame, len, mbWifiClient);
                }
            }
        }
    }
    
    // Feed watchdog at end of loop
    yield();
}

void tcpModbusStop() {
    if (ethModbusServer) delete ethModbusServer;
    if (wifiModbusServer) delete wifiModbusServer;
    ethModbusServer = nullptr;
    wifiModbusServer = nullptr;
    mb_initialized = false;
    mb_running = false;
}

bool tcpModbusIsRunning() {
    return mb_running;
}

// ==================== COMMAND HANDLERS ====================
// (Include all the command handler functions from previous version here)
void printTCPModbusHelp() {
    Serial.println("========= TCP Modbus Commands =========");
    Serial.println("  tcpmodbus enable         - Enable TCP Modbus");
    Serial.println("  tcpmodbus disable        - Disable TCP Modbus");
    Serial.println("  tcpmodbus slaveid <id>   - Set Slave ID");
    Serial.println("  tcpmodbus port <p>       - Set port");
    Serial.println("  tcpmodbus transport <t>  - wifi/ethernet/auto");
    Serial.println("  tcpmodbus status         - Show status");
    Serial.println("  tcpmodbus debug          - Toggle debug mode");
    Serial.println("========================================");
}

void handleTCPModbusCommand(String args) {
    args.trim();
    int spaceIndex = args.indexOf(' ');
    String subCmd = (spaceIndex > 0) ? args.substring(0, spaceIndex) : args;
    String subArgs = (spaceIndex > 0) ? args.substring(spaceIndex + 1) : "";
    subCmd.toLowerCase();
    subArgs.trim();
    
    if (subCmd == "enable") {
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putBool("enabled", true);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        Serial.println("[TCPModbus] Enabled");
    }
    else if (subCmd == "disable") {
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putBool("enabled", false);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        Serial.println("[TCPModbus] Disabled");
    }
    else if (subCmd == "transport") {
        uint8_t transport = TRANSPORT_AUTO;
        if (subArgs == "wifi") transport = TRANSPORT_WIFI;
        else if (subArgs == "ethernet" || subArgs == "eth") transport = TRANSPORT_ETHERNET;
        else if (subArgs == "auto") transport = TRANSPORT_AUTO;
        
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putUChar("transport", transport);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        Serial.printf("[TCPModbus] Transport: %s\n", subArgs.c_str());
    }
    else if (subCmd == "port") {
        int port = subArgs.toInt();
        if (port > 0 && port < 65536) {
            tcpModbusPref.end();
            tcpModbusPref.begin("tcpmodbus", false);
            tcpModbusPref.putUShort("port", port);
            tcpModbusPref.end();
            tcpModbusPref.begin("tcpmodbus", true);
            Serial.printf("[TCPModbus] Port: %d\n", port);
        }
    }
    else if (subCmd == "status") {
        Serial.println("=== TCP Modbus Status ===");
        Serial.printf("Enabled: %s\n", tcpModbusPref.getBool("enabled", false) ? "Yes" : "No");
        Serial.printf("Running: %s\n", mb_running ? "Yes" : "No");
        Serial.printf("Port: %d\n", tcpModbusPref.getUShort("port", 502));
        Serial.println("=========================");
    }
    else if (subCmd == "debug") {
        debugTCPModbus = !debugTCPModbus;
        Serial.printf("[TCPModbus] Debug mode: %s\n", debugTCPModbus ? "ON" : "OFF");
    }
    else {
        printTCPModbusHelp();
    }
}

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
        case TRANSPORT_ETHERNET: return "Ethernet";
        case TRANSPORT_AUTO: return "Auto";
        default: return "Unknown";
    }
}

#endif // TCP_MODBUS_SIMPLE_H
