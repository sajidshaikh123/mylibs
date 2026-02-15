#ifndef TCP_MODBUS_H
#define TCP_MODBUS_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Ethernet.h>
#include "esp_netif.h"

// esp-modbus includes
#include "mbcontroller.h"

// ==================== IMPORTANT NOTICE ====================
// NOTE: ESP-IDF Modbus library requires esp_netif interfaces
// W5500 Ethernet (SPI) is NOT compatible with esp_netif
// This library ONLY works with:
//   - ESP32 WiFi (WIFI_STA_DEF)
//   - ESP32 native Ethernet (LAN8720, TLK110) via RMII
//
// For W5500 Ethernet, you need to use a different Modbus TCP library
// such as ArduinoModbus or implement a custom TCP Modbus server
// ==========================================================

// External reference to preferences (should be initialized in main code)
extern Preferences tcpModbusPref;

// Transport type enum
enum TCPModbusTransport {
    TRANSPORT_WIFI = 0,
    TRANSPORT_ETHERNET = 1,
    TRANSPORT_AUTO = 2
};

// Mode enum
enum TCPModbusMode {
    MODE_SLAVE = 0,
    MODE_MASTER = 1
};

// ==================== MODBUS REGISTER DEFINITIONS ====================
// Define the number of registers for each type
#define MB_REG_HOLDING_START        0
#define MB_REG_HOLDING_COUNT        100  // Number of holding registers
#define MB_REG_INPUT_START          0
#define MB_REG_INPUT_COUNT          100  // Number of input registers
#define MB_REG_COIL_START           0
#define MB_REG_COIL_COUNT           64   // Number of coils
#define MB_REG_DISCRETE_START       0
#define MB_REG_DISCRETE_COUNT       64   // Number of discrete inputs

// Modbus register storage - use DRAM_ATTR for proper memory placement
static DRAM_ATTR uint16_t mb_holding_registers[MB_REG_HOLDING_COUNT] = {0};
static DRAM_ATTR uint16_t mb_input_registers[MB_REG_INPUT_COUNT] = {0};
static DRAM_ATTR uint8_t mb_coil_registers[(MB_REG_COIL_COUNT + 7) / 8] = {0};
static DRAM_ATTR uint8_t mb_discrete_registers[(MB_REG_DISCRETE_COUNT + 7) / 8] = {0};

// Modbus controller state
static bool mb_initialized = false;
static bool mb_running = false;
static uint8_t mb_mode_cached = MODE_SLAVE;  // Cached mode to avoid reading preferences in loop

// Modbus controller handle (stored for reference)
static void* mb_controller_handle = NULL;

// Callback function pointer for register access events
typedef void (*ModbusEventCallback)(mb_event_group_t event, uint16_t address, uint16_t size);
static ModbusEventCallback mb_event_callback = NULL;

// ==================== TCP MODBUS HELP ====================
void printTCPModbusHelp() {
    Serial.println("========= TCP Modbus Commands =========");
    Serial.println("  tcpmodbus enable         - Enable TCP Modbus");
    Serial.println("  tcpmodbus disable        - Disable TCP Modbus");
    Serial.println("  tcpmodbus master         - Set as Modbus Master (Client)");
    Serial.println("  tcpmodbus slave          - Set as Modbus Slave (Server)");
    Serial.println("  tcpmodbus slaveid <id>   - Set Slave/Unit ID (1-247)");
    Serial.println("  tcpmodbus remoteip <ip>  - Set remote IP address");
    Serial.println("  tcpmodbus remoteport <p> - Set remote port (default: 502)");
    Serial.println("  tcpmodbus sourceport <p> - Set local source port");
    Serial.println("  tcpmodbus transport <t>  - Set transport: wifi/ethernet/auto");
    Serial.println("  tcpmodbus status         - Show TCP Modbus status");
    Serial.println("  tcpmodbus show           - Show saved configuration");
    Serial.println("  tcpmodbus clear          - Clear all TCP Modbus config");
    Serial.println("========================================");
}

// ==================== TCP MODBUS COMMAND HANDLER ====================
void handleTCPModbusCommand(String args) {
    args.trim();
    
    // Parse subcommand
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
    else if (subCmd == "master") {
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putUChar("mode", MODE_MASTER);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        
        Serial.println("[TCPModbus] ✓ Mode set to: Master (Client)");
        Serial.println("[TCPModbus] Configure remoteip and remoteport for target slave");
    }
    else if (subCmd == "slave") {
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putUChar("mode", MODE_SLAVE);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        
        Serial.println("[TCPModbus] ✓ Mode set to: Slave (Server)");
        Serial.println("[TCPModbus] Configure slaveid and sourceport for listening");
    }
    else if (subCmd == "slaveid" || subCmd == "unitid" || subCmd == "id") {
        if (subArgs.length() == 0) {
            Serial.println("[TCPModbus] ✗ Error: Slave ID required");
            Serial.println("Usage: tcpmodbus slaveid <id>");
            Serial.println("Example: tcpmodbus slaveid 1");
            return;
        }
        
        int slaveId = subArgs.toInt();
        
        // Validate Modbus slave ID (1-247 for standard, 0 for broadcast)
        if (slaveId < 0 || slaveId > 247) {
            Serial.println("[TCPModbus] ✗ Error: Slave ID must be 0-247");
            Serial.println("Note: 0 is broadcast address, 1-247 are valid slave addresses");
            return;
        }
        
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putUChar("slaveid", (uint8_t)slaveId);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        
        Serial.printf("[TCPModbus] ✓ Slave ID set to: %d\n", slaveId);
    }
    else if (subCmd == "remoteip" || subCmd == "ip" || subCmd == "serverip") {
        if (subArgs.length() == 0) {
            Serial.println("[TCPModbus] ✗ Error: Remote IP address required");
            Serial.println("Usage: tcpmodbus remoteip <ip_address>");
            Serial.println("Example: tcpmodbus remoteip 192.168.1.100");
            return;
        }
        
        // Validate IP address format
        IPAddress ip;
        if (!ip.fromString(subArgs)) {
            Serial.println("[TCPModbus] ✗ Error: Invalid IP address format");
            return;
        }
        
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putString("remoteip", subArgs);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        
        Serial.printf("[TCPModbus] ✓ Remote IP set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "remoteport" || subCmd == "serverport" || subCmd == "dstport") {
        if (subArgs.length() == 0) {
            Serial.println("[TCPModbus] ✗ Error: Remote port required");
            Serial.println("Usage: tcpmodbus remoteport <port>");
            Serial.println("Example: tcpmodbus remoteport 502");
            return;
        }
        
        int port = subArgs.toInt();
        
        // Validate port range
        if (port < 1 || port > 65535) {
            Serial.println("[TCPModbus] ✗ Error: Port must be 1-65535");
            return;
        }
        
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putUShort("remoteport", (uint16_t)port);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        
        Serial.printf("[TCPModbus] ✓ Remote port set to: %d\n", port);
    }
    else if (subCmd == "sourceport" || subCmd == "localport" || subCmd == "srcport" || subCmd == "listenport") {
        if (subArgs.length() == 0) {
            Serial.println("[TCPModbus] ✗ Error: Source port required");
            Serial.println("Usage: tcpmodbus sourceport <port>");
            Serial.println("Example: tcpmodbus sourceport 502");
            return;
        }
        
        int port = subArgs.toInt();
        
        // Validate port range
        if (port < 1 || port > 65535) {
            Serial.println("[TCPModbus] ✗ Error: Port must be 1-65535");
            return;
        }
        
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", false);
        tcpModbusPref.putUShort("sourceport", (uint16_t)port);
        tcpModbusPref.end();
        tcpModbusPref.begin("tcpmodbus", true);
        
        Serial.printf("[TCPModbus] ✓ Source port set to: %d\n", port);
    }
    else if (subCmd == "transport" || subCmd == "interface") {
        if (subArgs.length() == 0) {
            Serial.println("[TCPModbus] ✗ Error: Transport type required");
            Serial.println("Usage: tcpmodbus transport <wifi|ethernet|auto>");
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
            transportName = "Ethernet";
        }
        else if (subArgs == "auto" || subArgs == "any") {
            transport = TRANSPORT_AUTO;
            transportName = "Auto (prefer Ethernet)";
        }
        else {
            Serial.println("[TCPModbus] ✗ Error: Invalid transport type");
            Serial.println("Valid options: wifi, ethernet, auto");
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
        uint8_t mode = tcpModbusPref.getUChar("mode", MODE_SLAVE);
        uint8_t slaveId = tcpModbusPref.getUChar("slaveid", 1);
        String remoteIp = tcpModbusPref.getString("remoteip", "");
        uint16_t remotePort = tcpModbusPref.getUShort("remoteport", 502);
        uint16_t sourcePort = tcpModbusPref.getUShort("sourceport", 502);
        uint8_t transport = tcpModbusPref.getUChar("transport", TRANSPORT_AUTO);
        
        Serial.printf("Enabled: %s\n", enabled ? "Yes" : "No");
        Serial.printf("Mode: %s\n", mode == MODE_MASTER ? "Master (Client)" : "Slave (Server)");
        Serial.printf("Slave/Unit ID: %d\n", slaveId);
        Serial.printf("Remote IP: %s\n", remoteIp.length() > 0 ? remoteIp.c_str() : "(not set)");
        Serial.printf("Remote Port: %d\n", remotePort);
        Serial.printf("Source Port: %d\n", sourcePort);
        
        String transportStr;
        switch (transport) {
            case TRANSPORT_WIFI: transportStr = "WiFi"; break;
            case TRANSPORT_ETHERNET: transportStr = "Ethernet"; break;
            case TRANSPORT_AUTO: transportStr = "Auto"; break;
            default: transportStr = "Unknown";
        }
        Serial.printf("Transport: %s\n", transportStr.c_str());
        
        Serial.println("=========================");
    }
    else if (subCmd == "show") {
        Serial.println("=== Saved TCP Modbus Configuration ===");
        
        bool enabled = tcpModbusPref.getBool("enabled", false);
        uint8_t mode = tcpModbusPref.getUChar("mode", MODE_SLAVE);
        uint8_t slaveId = tcpModbusPref.getUChar("slaveid", 1);
        String remoteIp = tcpModbusPref.getString("remoteip", "");
        uint16_t remotePort = tcpModbusPref.getUShort("remoteport", 502);
        uint16_t sourcePort = tcpModbusPref.getUShort("sourceport", 502);
        uint8_t transport = tcpModbusPref.getUChar("transport", TRANSPORT_AUTO);
        
        Serial.printf("Enabled: %s\n", enabled ? "Yes" : "No");
        Serial.printf("Mode: %s\n", mode == MODE_MASTER ? "Master (Client)" : "Slave (Server)");
        Serial.printf("Slave/Unit ID: %d\n", slaveId);
        Serial.printf("Remote IP: %s\n", remoteIp.length() > 0 ? remoteIp.c_str() : "(not set)");
        Serial.printf("Remote Port: %d\n", remotePort);
        Serial.printf("Source Port: %d\n", sourcePort);
        
        String transportStr;
        switch (transport) {
            case TRANSPORT_WIFI: transportStr = "WiFi"; break;
            case TRANSPORT_ETHERNET: transportStr = "Ethernet"; break;
            case TRANSPORT_AUTO: transportStr = "Auto"; break;
            default: transportStr = "Unknown";
        }
        Serial.printf("Transport: %s\n", transportStr.c_str());
        
        Serial.println("=======================================");
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
        Serial.println("[TCPModbus] Type 'tcpmodbus help' for available commands");
    }
}

// ==================== HELPER FUNCTIONS ====================

// Get current configuration as a struct (useful for initializing Modbus library)
struct TCPModbusConfig {
    bool enabled;
    uint8_t mode;           // MODE_MASTER or MODE_SLAVE
    uint8_t slaveId;        // 1-247
    String remoteIp;
    uint16_t remotePort;    // Default 502
    uint16_t sourcePort;    // Default 502
    uint8_t transport;      // TRANSPORT_WIFI, TRANSPORT_ETHERNET, TRANSPORT_AUTO
};

TCPModbusConfig getTCPModbusConfig() {
    TCPModbusConfig config;
    
    config.enabled = tcpModbusPref.getBool("enabled", false);
    config.mode = tcpModbusPref.getUChar("mode", MODE_SLAVE);
    config.slaveId = tcpModbusPref.getUChar("slaveid", 1);
    config.remoteIp = tcpModbusPref.getString("remoteip", "");
    config.remotePort = tcpModbusPref.getUShort("remoteport", 502);
    config.sourcePort = tcpModbusPref.getUShort("sourceport", 502);
    config.transport = tcpModbusPref.getUChar("transport", TRANSPORT_AUTO);
    
    return config;
}

// Check if TCP Modbus is enabled
bool isTCPModbusEnabled() {
    return tcpModbusPref.getBool("enabled", false);
}

// Check if configured as Master
bool isTCPModbusMaster() {
    return tcpModbusPref.getUChar("mode", MODE_SLAVE) == MODE_MASTER;
}

// Get transport type string
String getTCPModbusTransportString() {
    uint8_t transport = tcpModbusPref.getUChar("transport", TRANSPORT_AUTO);
    switch (transport) {
        case TRANSPORT_WIFI: return "WiFi";
        case TRANSPORT_ETHERNET: return "Ethernet";
        case TRANSPORT_AUTO: return "Auto";
        default: return "Unknown";
    }
}

// ==================== REGISTER ACCESS FUNCTIONS ====================

// Set a callback for register access events
void tcpModbusSetEventCallback(ModbusEventCallback callback) {
    mb_event_callback = callback;
}

// Holding Register functions
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

// Input Register functions
bool tcpModbusSetInputRegister(uint16_t address, uint16_t value) {
    if (address >= MB_REG_INPUT_COUNT) return false;
    mb_input_registers[address] = value;
    return true;
}

uint16_t tcpModbusGetInputRegister(uint16_t address) {
    if (address >= MB_REG_INPUT_COUNT) return 0;
    return mb_input_registers[address];
}

bool tcpModbusSetInputRegisters(uint16_t startAddress, uint16_t* values, uint16_t count) {
    if (startAddress + count > MB_REG_INPUT_COUNT) return false;
    memcpy(&mb_input_registers[startAddress], values, count * sizeof(uint16_t));
    return true;
}

// Coil functions
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

// Discrete Input functions
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

// Float helper functions for holding registers (2 registers per float)
bool tcpModbusSetHoldingFloat(uint16_t address, float value) {
    if (address + 1 >= MB_REG_HOLDING_COUNT) return false;
    uint16_t* ptr = (uint16_t*)&value;
    mb_holding_registers[address] = ptr[1];     // High word
    mb_holding_registers[address + 1] = ptr[0]; // Low word
    return true;
}

float tcpModbusGetHoldingFloat(uint16_t address) {
    if (address + 1 >= MB_REG_HOLDING_COUNT) return 0.0f;
    float value;
    uint16_t* ptr = (uint16_t*)&value;
    ptr[1] = mb_holding_registers[address];     // High word
    ptr[0] = mb_holding_registers[address + 1]; // Low word
    return value;
}

// ==================== MODBUS SLAVE INITIALIZATION ====================

// Helper function to get active network interface
static esp_netif_t* getActiveNetif(TCPModbusTransport transport) {
    esp_netif_t* netif = NULL;
    esp_netif_ip_info_t ip_info;
    
    switch (transport) {
        case TRANSPORT_WIFI:
            netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif) {
                esp_netif_get_ip_info(netif, &ip_info);
                Serial.printf("[TCPModbus] Using WiFi interface (IP: " IPSTR ")\n", IP2STR(&ip_info.ip));
            }
            break;
        case TRANSPORT_ETHERNET:
            // Try common Ethernet interface keys
            netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
            if (!netif) {
                Serial.println("[TCPModbus] ETH_DEF not found, trying eth0...");
                netif = esp_netif_get_handle_from_ifkey("eth0");
            }
            if (!netif) {
                Serial.println("[TCPModbus] eth0 not found, enumerating all interfaces...");
                // Enumerate all network interfaces and collect candidates
                esp_netif_t* candidate = NULL;
                esp_netif_t* temp_netif = esp_netif_next(NULL);
                int interface_count = 0;
                
                while (temp_netif != NULL) {
                    interface_count++;
                    if (esp_netif_get_ip_info(temp_netif, &ip_info) == ESP_OK) {
                        const char* desc = esp_netif_get_desc(temp_netif);
                        if (desc == NULL) desc = "unknown";
                        
                        if (ip_info.ip.addr != 0) {
                            Serial.printf("[TCPModbus] Interface %d: %s (IP: " IPSTR ")\n", 
                                        interface_count, desc, IP2STR(&ip_info.ip));
                            
                            uint32_t ip = ip_info.ip.addr;
                            // Skip only explicitly bad interfaces
                            bool is_loopback = ((ip & 0xFF) == 127);
                            bool is_wifi_ap = (ip == 0x0104A8C0); // 192.168.4.1
                            bool is_ap_interface = (strcmp(desc, "ap") == 0);
                            
                            // Accept first valid interface that's not loopback or AP
                            if (!is_loopback && !is_wifi_ap && !is_ap_interface && candidate == NULL) {
                                candidate = temp_netif;
                                Serial.printf("[TCPModbus]   -> Candidate for Ethernet\n");
                            } else {
                                Serial.printf("[TCPModbus]   -> Skipped (loopback=%d, wifi_ap=%d, ap=%d)\n",
                                            is_loopback, is_wifi_ap, is_ap_interface);
                            }
                        } else {
                            Serial.printf("[TCPModbus] Interface %d: %s (No IP)\n", interface_count, desc);
                        }
                    }
                    temp_netif = esp_netif_next(temp_netif);
                }
                
                netif = candidate;
                if (netif) {
                    Serial.println("[TCPModbus] Selected candidate as Ethernet interface");
                }
            }
            if (netif) {
                esp_netif_get_ip_info(netif, &ip_info);
                Serial.printf("[TCPModbus] Using Ethernet interface (IP: " IPSTR ")\n", IP2STR(&ip_info.ip));
            }
            break;
        case TRANSPORT_AUTO:
        default:
            // Try Ethernet first with multiple keys
            netif = esp_netif_get_handle_from_ifkey("ETH_DEF");
            if (!netif) netif = esp_netif_get_handle_from_ifkey("eth0");
            
            if (netif) {
                esp_netif_get_ip_info(netif, &ip_info);
                if (ip_info.ip.addr != 0) {
                    Serial.printf("[TCPModbus] Auto: Using Ethernet interface (IP: " IPSTR ")\n", IP2STR(&ip_info.ip));
                    break;
                }
                Serial.println("[TCPModbus] Auto: Ethernet found but no IP, trying WiFi...");
                netif = NULL;
            }
            
            // Fall back to WiFi if Ethernet not available or has no IP
            netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif) {
                esp_netif_get_ip_info(netif, &ip_info);
                Serial.printf("[TCPModbus] Auto: Using WiFi interface (IP: " IPSTR ")\n", IP2STR(&ip_info.ip));
            }
            break;
    }
    
    return netif;
}

esp_err_t tcpModbusSlaveInit(uint8_t slaveId, uint16_t port) {
    if (mb_initialized) {
        Serial.println("[TCPModbus] Already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    Serial.printf("[TCPModbus] Initializing Slave (ID: %d, Port: %d)...\n", slaveId, port);

    // Get the configured transport and network interface
    TCPModbusConfig config = getTCPModbusConfig();
    
    // Special warning for Ethernet mode
    if (config.transport == TRANSPORT_ETHERNET || config.transport == TRANSPORT_AUTO) {
        Serial.println("[TCPModbus] ⚠ WARNING: ESP-IDF Modbus requires esp_netif interfaces");
        Serial.println("[TCPModbus] ⚠ W5500 (SPI Ethernet) is NOT compatible!");
        Serial.println("[TCPModbus] ⚠ Only ESP32 native Ethernet (LAN8720/TLK110) or WiFi supported");
        Serial.println("[TCPModbus] ⚠ For W5500, use 'tcpmodbus transport wifi' or a different Modbus library");
    }
    
    esp_netif_t* netif = getActiveNetif((TCPModbusTransport)config.transport);
    
    if (netif == NULL) {
        Serial.println("[TCPModbus] ✗ No compatible network interface available!");
        Serial.println("[TCPModbus] Make sure WiFi is connected, or use a native Ethernet controller.");
        Serial.println("[TCPModbus] Current limitation: W5500 SPI Ethernet is not supported by esp-modbus.");
        return ESP_ERR_INVALID_STATE;
    }

    // Get IP address from the selected network interface
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        Serial.println("[TCPModbus] ✗ Failed to get IP info from interface!");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Convert IP to string
    static char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
    
    // Check if interface has valid IP
    if (ip_info.ip.addr == 0) {
        Serial.println("[TCPModbus] ✗ Network interface has no IP address!");
        Serial.println("[TCPModbus] Make sure the interface is connected and has obtained an IP.");
        return ESP_ERR_INVALID_STATE;
    }
    
    Serial.printf("[TCPModbus] Binding to IP: %s\n", ip_str);

    // Initialize Modbus slave controller for TCP
    esp_err_t err = mbc_slave_init_tcp(&mb_controller_handle);
    if (err != ESP_OK) {
        Serial.printf("[TCPModbus] ✗ Failed to init slave controller: 0x%x\n", err);
        return err;
    }

    // Configure TCP slave communication
    mb_communication_info_t comm_info = {};
    comm_info.ip_port = port;
    comm_info.ip_addr_type = MB_IPV4;
    comm_info.ip_mode = MB_MODE_TCP;
    comm_info.slave_uid = slaveId;
    comm_info.ip_addr = NULL;  // NULL to bind to all addresses on the interface
    comm_info.ip_netif_ptr = (void*)netif;  // Required: actual network interface

    // Setup communication parameters
    err = mbc_slave_setup((void*)&comm_info);
    if (err != ESP_OK) {
        Serial.printf("[TCPModbus] ✗ Failed to setup slave controller: 0x%x\n", err);
        mbc_slave_destroy();
        return err;
    }

    // Setup register area descriptors
    mb_register_area_descriptor_t reg_area;

    // Holding Registers
    reg_area.type = MB_PARAM_HOLDING;
    reg_area.start_offset = MB_REG_HOLDING_START;
    reg_area.address = (void*)mb_holding_registers;
    reg_area.size = MB_REG_HOLDING_COUNT * sizeof(uint16_t);
    err = mbc_slave_set_descriptor(reg_area);
    if (err != ESP_OK) {
        Serial.printf("[TCPModbus] ✗ Failed to set holding register descriptor: 0x%x\n", err);
        mbc_slave_destroy();
        return err;
    }

    // Input Registers
    reg_area.type = MB_PARAM_INPUT;
    reg_area.start_offset = MB_REG_INPUT_START;
    reg_area.address = (void*)mb_input_registers;
    reg_area.size = MB_REG_INPUT_COUNT * sizeof(uint16_t);
    err = mbc_slave_set_descriptor(reg_area);
    if (err != ESP_OK) {
        Serial.printf("[TCPModbus] ✗ Failed to set input register descriptor: 0x%x\n", err);
        mbc_slave_destroy();
        return err;
    }

    // Coils
    reg_area.type = MB_PARAM_COIL;
    reg_area.start_offset = MB_REG_COIL_START;
    reg_area.address = (void*)mb_coil_registers;
    reg_area.size = sizeof(mb_coil_registers);
    err = mbc_slave_set_descriptor(reg_area);
    if (err != ESP_OK) {
        Serial.printf("[TCPModbus] ✗ Failed to set coil descriptor: 0x%x\n", err);
        mbc_slave_destroy();
        return err;
    }

    // Discrete Inputs
    reg_area.type = MB_PARAM_DISCRETE;
    reg_area.start_offset = MB_REG_DISCRETE_START;
    reg_area.address = (void*)mb_discrete_registers;
    reg_area.size = sizeof(mb_discrete_registers);
    err = mbc_slave_set_descriptor(reg_area);
    if (err != ESP_OK) {
        Serial.printf("[TCPModbus] ✗ Failed to set discrete input descriptor: 0x%x\n", err);
        mbc_slave_destroy();
        return err;
    }

    // Start Modbus controller
    err = mbc_slave_start();
    if (err != ESP_OK) {
        Serial.printf("[TCPModbus] ✗ Failed to start slave controller: 0x%x\n", err);
        mbc_slave_destroy();
        return err;
    }

    mb_initialized = true;
    mb_running = true;
    mb_mode_cached = MODE_SLAVE;  // Cache the mode
    Serial.println("[TCPModbus] ✓ Slave initialized and running");
    return ESP_OK;
}

// ==================== MODBUS MASTER INITIALIZATION ====================

esp_err_t tcpModbusMasterInit(const char* slaveIp, uint8_t slaveId, uint16_t port) {
    if (mb_initialized) {
        Serial.println("[TCPModbus] Already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    Serial.printf("[TCPModbus] Initializing Master (Target: %s:%d)...\n", slaveIp, port);

    // Get the configured transport and network interface
    TCPModbusConfig config = getTCPModbusConfig();
    esp_netif_t* netif = getActiveNetif((TCPModbusTransport)config.transport);
    
    if (netif == NULL) {
        Serial.println("[TCPModbus] ✗ No network interface available!");
        Serial.println("[TCPModbus] Make sure WiFi or Ethernet is connected first.");
        return ESP_ERR_INVALID_STATE;
    }

    // Initialize Modbus master controller for TCP
    esp_err_t err = mbc_master_init_tcp(&mb_controller_handle);
    if (err != ESP_OK) {
        Serial.printf("[TCPModbus] ✗ Failed to init master controller: 0x%x\n", err);
        return err;
    }

    // Create IP address table for master (required for TCP master)
    // Format: "IP_ADDRESS" for each slave, null-terminated
    static char* slave_ip_table[2] = {NULL, NULL};
    static char ip_buffer[20];
    strncpy(ip_buffer, slaveIp, sizeof(ip_buffer) - 1);
    ip_buffer[sizeof(ip_buffer) - 1] = '\0';
    slave_ip_table[0] = ip_buffer;
    slave_ip_table[1] = NULL;

    // Configure TCP master communication
    mb_communication_info_t comm_info = {};
    comm_info.ip_port = port;
    comm_info.ip_addr_type = MB_IPV4;
    comm_info.ip_mode = MB_MODE_TCP;
    comm_info.ip_addr = (void*)slave_ip_table;  // Pointer to IP address table
    comm_info.ip_netif_ptr = (void*)netif;  // Required: actual network interface

    // Setup communication parameters
    err = mbc_master_setup((void*)&comm_info);
    if (err != ESP_OK) {
        Serial.printf("[TCPModbus] ✗ Failed to setup master controller: 0x%x\n", err);
        mbc_master_destroy();
        return err;
    }

    // Start Modbus controller
    err = mbc_master_start();
    if (err != ESP_OK) {
        Serial.printf("[TCPModbus] ✗ Failed to start master controller: 0x%x\n", err);
        mbc_master_destroy();
        return err;
    }

    mb_initialized = true;
    mb_running = true;
    mb_mode_cached = MODE_MASTER;  // Cache the mode
    Serial.println("[TCPModbus] ✓ Master initialized and running");
    return ESP_OK;
}

// ==================== MODBUS AUTO INITIALIZATION ====================

// Initialize TCP Modbus based on saved configuration
bool tcpModbusInit() {
    TCPModbusConfig config = getTCPModbusConfig();
    
    if (!config.enabled) {
        Serial.println("[TCPModbus] Disabled in configuration");
        return false;
    }

    esp_err_t err;
    
    if (config.mode == MODE_SLAVE) {
        err = tcpModbusSlaveInit(config.slaveId, config.sourcePort);
    } else {
        // For master mode
        if (config.remoteIp.length() > 0) {
            err = tcpModbusMasterInit(config.remoteIp.c_str(), config.slaveId, config.remotePort);
        } else {
            Serial.println("[TCPModbus] ✗ Master mode requires remote IP");
            return false;
        }
    }

    return (err == ESP_OK);
}

// ==================== MODBUS LOOP AND EVENT HANDLING ====================

// Slave loop - check for register access events
// Only checks every 100ms to prevent blocking, and only if callback is set
static unsigned long mb_last_event_check = 0;

void tcpModbusSlaveLoop() {
    if (!mb_initialized || !mb_running) {
        return;
    }
    
    // Skip event checking if no callback registered
    if (mb_event_callback == NULL) {
        return;
    }
    
    // Rate limit event checking to every 100ms
    unsigned long now = millis();
    if (now - mb_last_event_check < 100) {
        return;
    }
    mb_last_event_check = now;

    // Check for events (non-blocking with 0 timeout)
    mb_event_group_t event = mbc_slave_check_event(
        (mb_event_group_t)(MB_EVENT_HOLDING_REG_WR | MB_EVENT_HOLDING_REG_RD | 
                          MB_EVENT_INPUT_REG_RD | MB_EVENT_COILS_WR | 
                          MB_EVENT_COILS_RD | MB_EVENT_DISCRETE_RD));

    if (event != MB_EVENT_NO_EVENTS) {
        mb_param_info_t reg_info = {0};
        // Use 0 timeout for non-blocking operation
        esp_err_t err = mbc_slave_get_param_info(&reg_info, 0);
        if (err == ESP_OK) {
            // Pass the actual event that was triggered
            mb_event_callback(event, reg_info.mb_offset, reg_info.size);
        }
    }
}

// Master read holding registers from slave
esp_err_t tcpModbusMasterReadHolding(uint8_t slaveAddr, uint16_t regAddr, uint16_t regCount, uint16_t* destBuffer) {
    if (!mb_initialized || !mb_running) {
        return ESP_ERR_INVALID_STATE;
    }

    mb_param_request_t request = {
        .slave_addr = slaveAddr,
        .command = 0x03,  // Read Holding Registers
        .reg_start = regAddr,
        .reg_size = regCount
    };

    return mbc_master_send_request(&request, (void*)destBuffer);
}

// Master write holding registers to slave
esp_err_t tcpModbusMasterWriteHolding(uint8_t slaveAddr, uint16_t regAddr, uint16_t regCount, uint16_t* srcBuffer) {
    if (!mb_initialized || !mb_running) {
        return ESP_ERR_INVALID_STATE;
    }

    mb_param_request_t request = {
        .slave_addr = slaveAddr,
        .command = 0x10,  // Write Multiple Holding Registers
        .reg_start = regAddr,
        .reg_size = regCount
    };

    return mbc_master_send_request(&request, (void*)srcBuffer);
}

// Master read input registers from slave
esp_err_t tcpModbusMasterReadInput(uint8_t slaveAddr, uint16_t regAddr, uint16_t regCount, uint16_t* destBuffer) {
    if (!mb_initialized || !mb_running) {
        return ESP_ERR_INVALID_STATE;
    }

    mb_param_request_t request = {
        .slave_addr = slaveAddr,
        .command = 0x04,  // Read Input Registers
        .reg_start = regAddr,
        .reg_size = regCount
    };

    return mbc_master_send_request(&request, (void*)destBuffer);
}

// Master read coils from slave
esp_err_t tcpModbusMasterReadCoils(uint8_t slaveAddr, uint16_t coilAddr, uint16_t coilCount, uint8_t* destBuffer) {
    if (!mb_initialized || !mb_running) {
        return ESP_ERR_INVALID_STATE;
    }

    mb_param_request_t request = {
        .slave_addr = slaveAddr,
        .command = 0x01,  // Read Coils
        .reg_start = coilAddr,
        .reg_size = coilCount
    };

    return mbc_master_send_request(&request, (void*)destBuffer);
}

// Master write single coil to slave
esp_err_t tcpModbusMasterWriteCoil(uint8_t slaveAddr, uint16_t coilAddr, bool value) {
    if (!mb_initialized || !mb_running) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t coilValue = value ? 0xFF00 : 0x0000;
    
    mb_param_request_t request = {
        .slave_addr = slaveAddr,
        .command = 0x05,  // Write Single Coil
        .reg_start = coilAddr,
        .reg_size = 1
    };

    return mbc_master_send_request(&request, (void*)&coilValue);
}

// ==================== MODBUS STOP AND CLEANUP ====================

void tcpModbusStop() {
    if (!mb_initialized) {
        return;
    }

    if (mb_mode_cached == MODE_SLAVE) {
        mbc_slave_destroy();
    } else {
        mbc_master_destroy();
    }

    mb_initialized = false;
    mb_running = false;
    
    Serial.println("[TCPModbus] ✓ Stopped and cleaned up");
}

// Check if TCP Modbus is running
bool tcpModbusIsRunning() {
    return mb_running && mb_initialized;
}

// ==================== MAIN LOOP FUNCTION ====================

// Call this in the main loop
void tcpModbusLoop() {
    if (!mb_initialized || !mb_running) {
        return;
    }

    if (mb_mode_cached == MODE_SLAVE) {
        tcpModbusSlaveLoop();
    }
    // Master mode: user calls read/write functions as needed
}

#endif // TCP_MODBUS_H
