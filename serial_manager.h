#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Ethernet.h>

#define DEBUG_MODE 1
#define DEBUG_SERIAL        Serial  // Serial port for debug output

#if DEBUG_MODE
    #define DEBUG_PRINT(x)      DEBUG_SERIAL.print(x)
    #define DEBUG_PRINTLN(x)    DEBUG_SERIAL.println(x)
    #define DEBUG_PRINTF(...)   DEBUG_SERIAL.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif

// External reference to WiFi preferences (should be initialized in main code)
extern Preferences wifiPref;
extern Preferences ethernetPref;


void printHelp() {
    Serial.println("                  GATEWAY COMMANDS                          ");
    Serial.println("============================================================");
    Serial.println("Available Commands:");
    Serial.println("  help                     - Show this help message");
    Serial.println("  wifi [args]              - WiFi configuration commands");
    Serial.println("  eth [args]               - Ethernet configuration commands");
    Serial.println("  mqtt [args]              - MQTT configuration commands");
    Serial.println("  modbus [args]            - Modbus device management commands");
    Serial.println("  rtc [args]               - RTC (Real-Time Clock) commands");
    Serial.println("  show                     - Show system status");
    Serial.println("  reboot                   - Reboot the device");
    Serial.println("  factory                  - Restore factory settings");
    Serial.println("");
}

void printWiFiHelp() {
    Serial.println("=========== WiFi Commands ============");
    Serial.println("  wifi enable              - Enable WiFi");
    Serial.println("  wifi disable             - Disable WiFi");
    Serial.println("  wifi ssid <ssid>         - Set WiFi SSID");
    Serial.println("  wifi password <pass>     - Set WiFi password");
    Serial.println("  wifi connect             - Connect to WiFi");
    Serial.println("  wifi disconnect          - Disconnect from WiFi");
    Serial.println("  wifi status              - Show WiFi status");
    Serial.println("  wifi scan                - Scan for networks");
    Serial.println("  wifi show                - Show saved config");
    Serial.println("  wifi clear               - Clear WiFi config");
    Serial.println("  wifi ping <host>         - Ping a host (IP or domain)");
}

void printEthernetHelp() {
    Serial.println("========= Ethernet Commands ==========");
    Serial.println("  eth enable               - Enable Ethernet");
    Serial.println("  eth disable              - Disable Ethernet");
    Serial.println("  eth dhcp                 - Use DHCP (auto IP)");
    Serial.println("  eth static               - Use static IP");
    Serial.println("  eth ip <ip>              - Set static IP address");
    Serial.println("  eth gateway <ip>         - Set gateway address");
    Serial.println("  eth subnet <ip>          - Set subnet mask");
    Serial.println("  eth dns <ip>             - Set DNS server");
    Serial.println("  eth status               - Show Ethernet status");
    Serial.println("  eth show                 - Show saved config");
    Serial.println("  eth clear                - Clear Ethernet config");
    Serial.println("  eth reconnect            - Reconnect Ethernet");
}

void printSystemHelp() {
    Serial.println("========== System Commands ===========");
    Serial.println("  show                     - Show system status");
    Serial.println("  reboot                   - Reboot the device");
    Serial.println("  factory confirm          - Restore factory settings");
}

void printRTCHelp() {
    Serial.println("============ RTC Commands ============");
    Serial.println("  rtc show                 - Show current date/time");
    Serial.println("  rtc set <YYYY-MM-DD HH:MM:SS> - Set date and time");
    Serial.println("  rtc date                 - Show only date");
    Serial.println("  rtc time                 - Show only time");
    Serial.println("  rtc status               - Show RTC status (external/internal)");
    Serial.println("Example: rtc set 2025-12-31 23:59:59");
}
    
        
// ==================== WiFi COMMAND HANDLER ====================
void handleWiFiCommand(String args) {
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
        printWiFiHelp();
    }
    else if (subCmd == "enable") {
        wifiPref.end();
        wifiPref.begin("wifi", false);
        wifiPref.putBool("enabled", true);
        wifiPref.end();
        wifiPref.begin("wifi", true);
        
        Serial.println("[WiFi] ✓ Enabled (will connect on next boot)");
    }
    else if (subCmd == "disable") {
        wifiPref.end();
        wifiPref.begin("wifi", false);
        wifiPref.putBool("enabled", false);
        wifiPref.end();
        wifiPref.begin("wifi", true);
        
        WiFi.disconnect(true);
        Serial.println("[WiFi] ✓ Disabled");
    }
    else if (subCmd == "ssid") {
        if (subArgs.length() == 0) {
            Serial.println("[WiFi] ✗ Error: SSID required");
            Serial.println("Usage: wifi ssid <ssid>");
            return;
        }
        
        if (subArgs.length() > 32) {
            Serial.println("[WiFi] ✗ Error: SSID too long (max 32 chars)");
            return;
        }
        
        wifiPref.end();
        wifiPref.begin("wifi", false);
        wifiPref.putString("ssid", subArgs);
        wifiPref.end();
        wifiPref.begin("wifi", true);
        
        Serial.printf("[WiFi] ✓ SSID set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "password" || subCmd == "pass") {
        if (subArgs.length() == 0) {
            Serial.println("[WiFi] ✗ Error: Password required");
            Serial.println("Usage: wifi password <password>");
            return;
        }
        
        if (subArgs.length() < 8 || subArgs.length() > 63) {
            Serial.println("[WiFi] ✗ Error: Password must be 8-63 characters");
            return;
        }
        
        wifiPref.end();
        wifiPref.begin("wifi", false);
        wifiPref.putString("password", subArgs);
        wifiPref.end();
        wifiPref.begin("wifi", true);
        
        Serial.println("[WiFi] ✓ Password saved");
    }
    else if (subCmd == "connect") {
        String ssid = wifiPref.getString("ssid", "");
        String password = wifiPref.getString("password", "");
        
        if (ssid.length() == 0) {
            Serial.println("[WiFi] ✗ Error: No SSID configured");
            Serial.println("Use: wifi ssid <ssid>");
            return;
        }
        
        Serial.printf("[WiFi] Connecting to: %s\n", ssid.c_str());
        
        WiFi.begin(ssid.c_str(), password.c_str());
        
        // Wait for connection (max 10 seconds)
        int timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 40) {
            delay(250);
            Serial.print(".");
            timeout++;
        }
        Serial.println();
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[WiFi] ✓ Connected!");
            Serial.printf("[WiFi] IP Address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] Signal: %d dBm\n", WiFi.RSSI());
            
            // Auto-enable on successful connection
            wifiPref.end();
            wifiPref.begin("wifi", false);
            wifiPref.putBool("enabled", true);
            wifiPref.end();
            wifiPref.begin("wifi", true);
        } else {
            Serial.println("[WiFi] ✗ Connection failed");
            Serial.print("[WiFi] Status: ");
            switch(WiFi.status()) {
                case WL_NO_SSID_AVAIL: Serial.println("SSID not found"); break;
                case WL_CONNECT_FAILED: Serial.println("Connection failed"); break;
                case WL_CONNECTION_LOST: Serial.println("Connection lost"); break;
                case WL_DISCONNECTED: Serial.println("Disconnected"); break;
                default: Serial.println("Unknown error");
            }
        }
    }
    else if (subCmd == "disconnect") {
        WiFi.disconnect(true);
        Serial.println("[WiFi] ✓ Disconnected");
    }
    else if (subCmd == "status") {
        Serial.println("=== WiFi Status ===");
        
        bool enabled = wifiPref.getBool("enabled", false);
        Serial.printf("Enabled: %s\n", enabled ? "Yes" : "No");
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Status: Connected");
            Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
            Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
            Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
            Serial.printf("Signal: %d dBm\n", WiFi.RSSI());
            Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
        } else {
            Serial.println("Status: Disconnected");
        }
        Serial.println("==================");
    }
    else if (subCmd == "scan") {
        Serial.println("[WiFi] Scanning for networks...");
        
        int n = WiFi.scanNetworks();
        
        if (n == 0) {
            Serial.println("[WiFi] No networks found");
        } else {
            Serial.printf("[WiFi] Found %d networks:\n\n", n);
            Serial.println("  #  SSID                            RSSI  Ch  Encryption");
            Serial.println("---  ------------------------------  ----  --  ----------");
            
            for (int i = 0; i < n; i++) {
                Serial.printf("%3d  %-30s  %4d  %2d  ", 
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i),
                    WiFi.channel(i)
                );
                
                switch(WiFi.encryptionType(i)) {
                    case WIFI_AUTH_OPEN: Serial.println("Open"); break;
                    case WIFI_AUTH_WEP: Serial.println("WEP"); break;
                    case WIFI_AUTH_WPA_PSK: Serial.println("WPA-PSK"); break;
                    case WIFI_AUTH_WPA2_PSK: Serial.println("WPA2-PSK"); break;
                    case WIFI_AUTH_WPA_WPA2_PSK: Serial.println("WPA/WPA2-PSK"); break;
                    case WIFI_AUTH_WPA2_ENTERPRISE: Serial.println("WPA2-Enterprise"); break;
                    default: Serial.println("Unknown");
                }
            }
            Serial.println();
        }
        
        WiFi.scanDelete();
    }
    else if (subCmd == "show") {
        Serial.println("=== Saved WiFi Configuration ===");
        
        bool enabled = wifiPref.getBool("enabled", false);
        String ssid = wifiPref.getString("ssid", "");
        String password = wifiPref.getString("password", "");
        
        Serial.printf("Enabled: %s\n", enabled ? "Yes" : "No");
        Serial.printf("SSID: %s\n", ssid.length() > 0 ? ssid.c_str() : "(not set)");
        Serial.printf("Password: %s\n", password.length() > 0 ? "********" : "(not set)");
        Serial.println("==============================");
    }
    else if (subCmd == "clear") {
        wifiPref.end();
        wifiPref.begin("wifi", false);
        wifiPref.clear();
        wifiPref.end();
        wifiPref.begin("wifi", true);
        
        WiFi.disconnect(true);
        Serial.println("[WiFi] ✓ Configuration cleared");
    }
    else if (subCmd == "ping") {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] ✗ Error: WiFi not connected");
            return;
        }
        
        if (subArgs.length() == 0) {
            Serial.println("[WiFi] ✗ Error: Host required");
            Serial.println("Usage: wifi ping <host>");
            Serial.println("Example: wifi ping google.com");
            Serial.println("Example: wifi ping 8.8.8.8");
            return;
        }
        
        Serial.printf("[WiFi] Pinging %s...\n", subArgs.c_str());
        
        // Try to resolve hostname if it's not an IP
        IPAddress ip;
        if (!ip.fromString(subArgs)) {
            // It's a hostname, resolve it
            if (WiFi.hostByName(subArgs.c_str(), ip)) {
                Serial.printf("[WiFi] Resolved %s to %s\n", subArgs.c_str(), ip.toString().c_str());
            } else {
                Serial.printf("[WiFi] ✗ Failed to resolve hostname: %s\n", subArgs.c_str());
                return;
            }
        }
        
        // Perform TCP connection test (since ICMP ping requires raw sockets)
        const int testPort = 80;  // Try HTTP port
        WiFiClient testClient;
        
        Serial.printf("[WiFi] Testing TCP connection to %s:%d...\n", ip.toString().c_str(), testPort);
        
        unsigned long startTime = millis();
        bool connected = testClient.connect(ip, testPort, 5000);  // 5 second timeout
        unsigned long endTime = millis();
        
        if (connected) {
            testClient.stop();
            Serial.printf("[WiFi] ✓ Host is reachable (response time: %lu ms)\n", endTime - startTime);
        } else {
            Serial.printf("[WiFi] ✗ Host unreachable or port %d closed\n", testPort);
            Serial.println("[WiFi] Note: This tests TCP connectivity, not ICMP ping");
        }
    }
    else {
        Serial.printf("[WiFi] ✗ Unknown command: %s\n", subCmd.c_str());
        Serial.println("[WiFi] Type 'wifi help' for available commands");
    }
}


// ==================== ETHERNET COMMAND HANDLER ====================
void handleEthernetCommand(String args) {
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
        printEthernetHelp();
    }
    else if (subCmd == "enable") {
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        ethernetPref.putBool("enabled", true);
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);
        
        Serial.println("[Ethernet] ✓ Enabled (will connect on next boot)");
    }
    else if (subCmd == "disable") {
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        ethernetPref.putBool("enabled", false);
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);
        
        Serial.println("[Ethernet] ✓ Disabled");
    }
    else if (subCmd == "dhcp") {
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        ethernetPref.putBool("dhcp", true);
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);
        
        Serial.println("[Ethernet] ✓ DHCP mode enabled");
        Serial.println("[Ethernet] Use 'eth reconnect' to apply changes");
    }
    else if (subCmd == "static") {
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        ethernetPref.putBool("dhcp", false);
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);
        
        Serial.println("[Ethernet] ✓ Static IP mode enabled");
        Serial.println("[Ethernet] Configure IP, gateway, subnet, and DNS");
        Serial.println("[Ethernet] Use 'eth reconnect' to apply changes");
    }
    else if (subCmd == "ip") {
        if (subArgs.length() == 0) {
            Serial.println("[Ethernet] ✗ Error: IP address required");
            Serial.println("Usage: eth ip <ip_address>");
            Serial.println("Example: eth ip 192.168.1.100");
            return;
        }
        
        // Validate IP address format
        IPAddress ip;
        if (!ip.fromString(subArgs)) {
            Serial.println("[Ethernet] ✗ Error: Invalid IP address format");
            return;
        }
        
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        ethernetPref.putString("ip", subArgs);
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);
        
        Serial.printf("[Ethernet] ✓ IP address set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "gateway") {
        if (subArgs.length() == 0) {
            Serial.println("[Ethernet] ✗ Error: Gateway address required");
            Serial.println("Usage: eth gateway <ip_address>");
            Serial.println("Example: eth gateway 192.168.1.1");
            return;
        }
        
        IPAddress gateway;
        if (!gateway.fromString(subArgs)) {
            Serial.println("[Ethernet] ✗ Error: Invalid IP address format");
            return;
        }
        
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        ethernetPref.putString("gateway", subArgs);
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);
        
        Serial.printf("[Ethernet] ✓ Gateway set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "subnet") {
        if (subArgs.length() == 0) {
            Serial.println("[Ethernet] ✗ Error: Subnet mask required");
            Serial.println("Usage: eth subnet <subnet_mask>");
            Serial.println("Example: eth subnet 255.255.255.0");
            return;
        }
        
        IPAddress subnet;
        if (!subnet.fromString(subArgs)) {
            Serial.println("[Ethernet] ✗ Error: Invalid subnet mask format");
            return;
        }
        
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        ethernetPref.putString("subnet", subArgs);
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);
        
        Serial.printf("[Ethernet] ✓ Subnet mask set to: %s\n", subArgs.c_str());
    }else if (subCmd == "dns") {
        if (subArgs.length() == 0) {
            Serial.println("[Ethernet] ✗ Error: DNS server address required");
            Serial.println("Usage: eth dns <ip_address>");
            Serial.println("Example: eth dns 8.8.8.8");
            return;
        }
        
        IPAddress dns;
        if (!dns.fromString(subArgs)) {
            Serial.println("[Ethernet] ✗ Error: Invalid IP address format");
            return;
        }
        
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        ethernetPref.putString("dns", subArgs);
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);
        
        Serial.printf("[Ethernet] ✓ DNS server set to: %s\n", subArgs.c_str());
    }else if (subCmd == "status") {
        Serial.println("=== Ethernet Status ===");
        
        bool enabled = ethernetPref.getBool("enabled", true);
        bool useDHCP = ethernetPref.getBool("dhcp", true);
        
        Serial.printf("Enabled: %s\n", enabled ? "Yes" : "No");
        Serial.printf("Mode: %s\n", useDHCP ? "DHCP" : "Static IP");
        
        if (Ethernet.linkStatus() == LinkON) {
            Serial.println("Link: Connected");
            Serial.printf("IP Address: %s\n", Ethernet.localIP().toString().c_str());
            Serial.printf("Gateway: %s\n", Ethernet.gatewayIP().toString().c_str());
            Serial.printf("Subnet: %s\n", Ethernet.subnetMask().toString().c_str());
            Serial.printf("DNS: %s\n", Ethernet.dnsServerIP().toString().c_str());
            
            uint8_t mac[6];
            Ethernet.MACAddress(mac);
            Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        } else {
            Serial.println("Link: Disconnected");
            Serial.println("Status: Cable not connected or adapter not initialized");
        }
        Serial.println("======================");
    }else if (subCmd == "show") {
        Serial.println("=== Saved Ethernet Configuration ===");
        
        bool enabled = ethernetPref.getBool("enabled", true);
        bool useDHCP = ethernetPref.getBool("dhcp", true);
        String ip = ethernetPref.getString("ip", "");
        String gateway = ethernetPref.getString("gateway", "");
        String subnet = ethernetPref.getString("subnet", "");
        String dns = ethernetPref.getString("dns", "");
        
        Serial.printf("Enabled: %s\n", enabled ? "Yes" : "No");
        Serial.printf("Mode: %s\n", useDHCP ? "DHCP" : "Static IP");
        
        if (!useDHCP) {
            Serial.printf("IP Address: %s\n", ip.length() > 0 ? ip.c_str() : "(not set)");
            Serial.printf("Gateway: %s\n", gateway.length() > 0 ? gateway.c_str() : "(not set)");
            Serial.printf("Subnet: %s\n", subnet.length() > 0 ? subnet.c_str() : "(not set)");
            Serial.printf("DNS: %s\n", dns.length() > 0 ? dns.c_str() : "(not set)");
        }
        Serial.println("====================================");
    }
    else if (subCmd == "clear") {
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        ethernetPref.clear();
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);
        
        Serial.println("[Ethernet] ✓ Configuration cleared (will use DHCP on next boot)");
    }
    else if (subCmd == "reconnect" || subCmd == "reset") {
        Serial.println("[Ethernet] Reconnecting...");
        Serial.println("[Ethernet] Note: Full reconnection requires restart");
        Serial.println("[Ethernet] Current status:");
        
        // Show current connection info
        if (Ethernet.linkStatus() == LinkON) {
            Serial.printf("[Ethernet] IP: %s\n", Ethernet.localIP().toString().c_str());
        } else {
            Serial.println("[Ethernet] ✗ Not connected");
        }
    }
    else {
        Serial.printf("[Ethernet] ✗ Unknown command: %s\n", subCmd.c_str());
        Serial.println("[Ethernet] Type 'eth help' for available commands");
    }
}


// ==================== RTC COMMAND HANDLER ====================
void handleRTCCommand(String args) {
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
        printRTCHelp();
    }
    else if (subCmd == "show" || subCmd == "status") {
        Serial.println("=== RTC Status ===");
        extern bool RTC_OK;
        Serial.printf("RTC Type: %s\n", RTC_OK ? "External (DS3231)" : "Internal (ESP32)");
        Serial.printf("Date/Time: %s\n", getDateTime());
        Serial.printf("Year: %d\n", getYear());
        Serial.printf("Month: %02d\n", getMonth());
        Serial.printf("Date: %02d\n", getDate());
        Serial.printf("Hour: %02d\n", getHour());
        Serial.printf("Minute: %02d\n", getMinute());
        Serial.printf("Second: %02d\n", getSeconds());
        Serial.println("==================");
    }
    else if (subCmd == "date") {
        Serial.printf("Date: %d-%02d-%02d\n", getYear(), getMonth(), getDate());
    }
    else if (subCmd == "time") {
        Serial.printf("Time: %02d:%02d:%02d\n", getHour(), getMinute(), getSeconds());
    }
    else if (subCmd == "set") {
        if (subArgs.length() == 0) {
            Serial.println("[RTC] ✗ Error: Date and time required");
            Serial.println("Usage: rtc set <YYYY-MM-DD HH:MM:SS>");
            Serial.println("Example: rtc set 2025-12-31 23:59:59");
            return;
        }
        
        // Parse date and time: YYYY-MM-DD HH:MM:SS
        int year, month, day, hour, minute, second;
        int parsed = sscanf(subArgs.c_str(), "%d-%d-%d %d:%d:%d", 
                           &year, &month, &day, &hour, &minute, &second);
        
        if (parsed != 6) {
            Serial.println("[RTC] ✗ Error: Invalid format");
            Serial.println("Format: YYYY-MM-DD HH:MM:SS");
            Serial.println("Example: rtc set 2025-12-31 23:59:59");
            return;
        }
        
        // Validate ranges
        if (year < 2000 || year > 2099) {
            Serial.println("[RTC] ✗ Error: Year must be between 2000-2099");
            return;
        }
        if (month < 1 || month > 12) {
            Serial.println("[RTC] ✗ Error: Month must be between 1-12");
            return;
        }
        if (day < 1 || day > 31) {
            Serial.println("[RTC] ✗ Error: Day must be between 1-31");
            return;
        }
        if (hour < 0 || hour > 23) {
            Serial.println("[RTC] ✗ Error: Hour must be between 0-23");
            return;
        }
        if (minute < 0 || minute > 59) {
            Serial.println("[RTC] ✗ Error: Minute must be between 0-59");
            return;
        }
        if (second < 0 || second > 59) {
            Serial.println("[RTC] ✗ Error: Second must be between 0-59");
            return;
        }
        
        // Set RTC
        setRTC(day, month, year, hour, minute, second);
        
        Serial.println("[RTC] ✓ Date/time set successfully");
        Serial.printf("[RTC] New date/time: %s\n", getDateTime());
    }
    else {
        Serial.printf("[RTC] ✗ Unknown command: %s\n", subCmd.c_str());
        Serial.println("[RTC] Type 'rtc help' for available commands");
    }
}


// ==================== SYSTEM COMMAND HANDLER ====================
void handleSystemCommand(String cmd, String args) {
    cmd.trim();
    cmd.toLowerCase();
    
    if (cmd == "reboot" || cmd == "restart") {
        Serial.println("[System] Rebooting device in 2 seconds...");
        Serial.flush();
        delay(2000);
        ESP.restart();
    }
    else if (cmd == "factory" || cmd == "reset") {
        Serial.println("[System] Factory reset requested");
        Serial.println("[System] This will clear ALL saved settings!");
        Serial.println("[System] Type 'factory confirm' to proceed");
        
        if (args == "confirm") {
            Serial.println("[System] Clearing all preferences...");
            
            // Clear WiFi preferences
            wifiPref.end();
            wifiPref.begin("wifi", false);
            wifiPref.clear();
            wifiPref.end();
            Serial.println("[System] WiFi config cleared");
            
            // Clear Ethernet preferences
            ethernetPref.end();
            ethernetPref.begin("ethernet", false);
            ethernetPref.clear();
            ethernetPref.end();
            Serial.println("[System] Ethernet config cleared");
            
            Serial.println("[System] ✓ Factory reset complete");
            Serial.println("[System] Rebooting in 2 seconds...");
            Serial.flush();
            delay(2000);
            ESP.restart();
        } else {
            Serial.println("[System] Factory reset cancelled");
        }
    }
    else if (cmd == "show" || cmd == "status") {
        Serial.println("\n======== System Status ========");
        Serial.printf("Chip Model: %s\n", ESP.getChipModel());
        Serial.printf("Chip Cores: %d\n", ESP.getChipCores());
        Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
        Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        
        Serial.println("\n--- Network Status ---");
        // WiFi Status
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("WiFi: Connected to %s\n", WiFi.SSID().c_str());
            Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
        } else {
            wifiPref.begin("wifi", true);
            bool wifiEnabled = wifiPref.getBool("enabled", false);
            wifiPref.end();
            Serial.printf("WiFi: Disconnected (%s)\n", wifiEnabled ? "enabled" : "disabled");
        }
        
        // Ethernet Status
        if (Ethernet.linkStatus() == LinkON) {
            Serial.println("Ethernet: Connected");
            Serial.printf("  IP: %s\n", Ethernet.localIP().toString().c_str());
        } else {
            ethernetPref.begin("ethernet", true);
            bool ethEnabled = ethernetPref.getBool("enabled", true);
            ethernetPref.end();
            Serial.printf("Ethernet: Disconnected (%s)\n", ethEnabled ? "enabled" : "disabled");
        }
        
        Serial.println("==============================\n");
    }
    else {
        Serial.printf("[System] ✗ Unknown command: %s\n", cmd.c_str());
        printSystemHelp();
    }
}


// ==================== COMMAND EXECUTOR ====================
void executeCommand(String cmd, String args) {
    cmd.trim();
    args.trim();
    cmd.toLowerCase();
    
    
    if (cmd == "help") {
        // DEBUG_PRINT("help"); 
        printHelp();
    }
    else if (cmd == "wifi") {
        // DEBUG_PRINT("wifi"); 
        handleWiFiCommand(args);
    }
    else if (cmd == "eth" || cmd == "ethernet") {
        // DEBUG_PRINT("eth");
        handleEthernetCommand(args);
    }
    else if (cmd == "mqtt") {
        DEBUG_PRINT("mqtt");
        //handleMQTTCommand(args);
    }
    else if (cmd == "modbus") {
        DEBUG_PRINT("modbus");
        //handleModbusCommand(args);
    }
    else if (cmd == "rtc") {
        handleRTCCommand(args);
    }
    else if (cmd == "show" || cmd == "reboot" || cmd == "factory") {
        handleSystemCommand(cmd, args);
    }
    else {
        Serial.printf("[CMD] ✗ Unknown command: %s\n", cmd.c_str());
        Serial.println("[CMD] Type 'help' for available commands");
    }
}


// ==================== COMMAND PARSER ====================
void parseSerialCommand(String command) {
    if (command.length() == 0) return;
    
    // Find first space to separate command and arguments
    int spaceIndex = command.indexOf(' ');
    String cmd, args;
    
    if (spaceIndex > 0) {
        cmd = command.substring(0, spaceIndex);
        args = command.substring(spaceIndex + 1);
    } else {
        cmd = command;
        args = "";
    }
    
    executeCommand(cmd, args);
}


void handleSerialCommands(){
    if(Serial.available()){
        String command = Serial.readStringUntil('\n');
        command.trim();

        parseSerialCommand(command);

        Serial.print("\n> ");
    }
}

#endif // SERIAL_MANAGER_H
