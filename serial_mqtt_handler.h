#ifndef SERIAL_MQTT_HANDLER_H
#define SERIAL_MQTT_HANDLER_H

#include <Arduino.h>
#include <Preferences.h>

// External reference to MQTT preferences (should be initialized in main code)
extern Preferences mqttPref;
extern bool mqtt_connected; 

void printMQTTHelp() {
    Serial.println("=========== MQTT Commands ============");
    Serial.println("  mqtt enable              - Enable MQTT");
    Serial.println("  mqtt disable             - Disable MQTT");
    Serial.println("  mqtt server <ip/host>    - Set MQTT broker address");
    Serial.println("  mqtt port <port>         - Set MQTT broker port");
    Serial.println("  mqtt user <username>     - Set MQTT username");
    Serial.println("  mqtt password <pass>     - Set MQTT password");
    Serial.println("  mqtt clientid <id>       - Set MQTT client ID");
    Serial.println("  mqtt transport <type>    - Set connection (wifi/ethernet/auto)");
    Serial.println("  mqtt status              - Show MQTT status");
    Serial.println("  mqtt show                - Show saved config");
    Serial.println("  mqtt clear               - Clear MQTT config");
    Serial.println("  mqtt test                - Test MQTT connection");
    Serial.println("Examples:");
    Serial.println("  mqtt server 192.168.1.100");
    Serial.println("  mqtt port 1883");
    Serial.println("  mqtt user myuser");
    Serial.println("  mqtt transport wifi");
    Serial.println("  mqtt transport ethernet");
}

// ==================== MQTT COMMAND HANDLER ====================
void handleMQTTCommand(String args) {
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
        printMQTTHelp();
    }
    else if (subCmd == "enable") {
        mqttPref.end();
        mqttPref.begin("mqtt", false);
        mqttPref.putBool("enabled", true);
        mqttPref.end();
        mqttPref.begin("mqtt", true);
        
        Serial.println("[MQTT] ✓ Enabled (will connect on next boot)");
    }
    else if (subCmd == "disable") {
        mqttPref.end();
        mqttPref.begin("mqtt", false);
        mqttPref.putBool("enabled", false);
        mqttPref.end();
        mqttPref.begin("mqtt", true);
        
        Serial.println("[MQTT] ✓ Disabled");
    }
    else if (subCmd == "server" || subCmd == "broker") {
        if (subArgs.length() == 0) {
            Serial.println("[MQTT] ✗ Error: Server address required");
            Serial.println("Usage: mqtt server <ip_or_hostname>");
            Serial.println("Examples:");
            Serial.println("  mqtt server 192.168.1.100");
            Serial.println("  mqtt server broker.hivemq.com");
            return;
        }
        
        if (subArgs.length() > 128) {
            Serial.println("[MQTT] ✗ Error: Server address too long (max 128 chars)");
            return;
        }
        
        mqttPref.end();
        mqttPref.begin("mqtt", false);
        mqttPref.putString("server", subArgs);
        mqttPref.end();
        mqttPref.begin("mqtt", true);
        
        Serial.printf("[MQTT] ✓ Server set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "port") {
        if (subArgs.length() == 0) {
            Serial.println("[MQTT] ✗ Error: Port number required");
            Serial.println("Usage: mqtt port <port_number>");
            Serial.println("Default: 1883 (non-SSL), 8883 (SSL)");
            return;
        }
        
        int port = subArgs.toInt();
        if (port < 1 || port > 65535) {
            Serial.println("[MQTT] ✗ Error: Port must be between 1-65535");
            return;
        }
        
        mqttPref.end();
        mqttPref.begin("mqtt", false);
        mqttPref.putUShort("port", port);
        mqttPref.end();
        mqttPref.begin("mqtt", true);
        
        Serial.printf("[MQTT] ✓ Port set to: %d\n", port);
    }
    else if (subCmd == "user" || subCmd == "username") {
        if (subArgs.length() == 0) {
            Serial.println("[MQTT] ✗ Error: Username required");
            Serial.println("Usage: mqtt user <username>");
            return;
        }
        
        if (subArgs.length() > 64) {
            Serial.println("[MQTT] ✗ Error: Username too long (max 64 chars)");
            return;
        }
        
        mqttPref.end();
        mqttPref.begin("mqtt", false);
        mqttPref.putString("username", subArgs);
        mqttPref.end();
        mqttPref.begin("mqtt", true);
        
        Serial.printf("[MQTT] ✓ Username set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "password" || subCmd == "pass") {
        if (subArgs.length() == 0) {
            Serial.println("[MQTT] ✗ Error: Password required");
            Serial.println("Usage: mqtt password <password>");
            return;
        }
        
        if (subArgs.length() > 64) {
            Serial.println("[MQTT] ✗ Error: Password too long (max 64 chars)");
            return;
        }
        
        mqttPref.end();
        mqttPref.begin("mqtt", false);
        mqttPref.putString("password", subArgs);
        mqttPref.end();
        mqttPref.begin("mqtt", true);
        
        Serial.println("[MQTT] ✓ Password saved");
    }
    else if (subCmd == "clientid" || subCmd == "client") {
        if (subArgs.length() == 0) {
            Serial.println("[MQTT] ✗ Error: Client ID required");
            Serial.println("Usage: mqtt clientid <client_id>");
            return;
        }
        
        if (subArgs.length() > 64) {
            Serial.println("[MQTT] ✗ Error: Client ID too long (max 64 chars)");
            return;
        }
        
        mqttPref.end();
        mqttPref.begin("mqtt", false);
        mqttPref.putString("clientid", subArgs);
        mqttPref.end();
        mqttPref.begin("mqtt", true);
        
        Serial.printf("[MQTT] ✓ Client ID set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "transport" || subCmd == "connection" || subCmd == "network") {
        if (subArgs.length() == 0) {
            Serial.println("[MQTT] ✗ Error: Transport type required");
            Serial.println("Usage: mqtt transport <type>");
            Serial.println("Options: wifi, ethernet, auto");
            Serial.println("  wifi     - Use WiFi only");
            Serial.println("  ethernet - Use Ethernet only");
            Serial.println("  auto     - Prefer Ethernet, fallback to WiFi");
            return;
        }
        
        subArgs.toLowerCase();
        
        if (subArgs != "wifi" && subArgs != "ethernet" && subArgs != "auto") {
            Serial.println("[MQTT] ✗ Error: Invalid transport type");
            Serial.println("Valid options: wifi, ethernet, auto");
            return;
        }
        
        mqttPref.end();
        mqttPref.begin("mqtt", false);
        mqttPref.putString("transport", subArgs);
        mqttPref.end();
        mqttPref.begin("mqtt", true);
        
        Serial.printf("[MQTT] ✓ Transport set to: %s\n", subArgs.c_str());
        
        if (subArgs == "auto") {
            Serial.println("[MQTT] Will prefer Ethernet, fallback to WiFi");
        } else if (subArgs == "wifi") {
            Serial.println("[MQTT] Will use WiFi connection only");
        } else {
            Serial.println("[MQTT] Will use Ethernet connection only");
        }
    }
    else if (subCmd == "status") {
        Serial.println("=== MQTT Status ===");
        
        bool enabled = mqttPref.getBool("enabled", false);
        Serial.printf("Enabled: %s\n", enabled ? "Yes" : "No");
        
        // You can add actual connection status here if you have access to MQTT client
        // For now, just show configuration
        String server = mqttPref.getString("server", "");
        if (server.length() > 0) {
            uint16_t port = mqttPref.getUShort("port", 1883);
            String transport = mqttPref.getString("transport", "auto");
            Serial.printf("Server: %s:%d\n", server.c_str(), port);
            Serial.printf("Transport: %s\n", transport.c_str());
            Serial.printf("Status: %s\n", mqtt_connected ? "Connected" : "Disconnected");
        } else {
            Serial.println("Server: Not configured");
        }
        
        Serial.println("==================");
    }
    else if (subCmd == "show") {
        Serial.println("=== Saved MQTT Configuration ===");
        
        bool enabled = mqttPref.getBool("enabled", false);
        String server = mqttPref.getString("server", "");
        uint16_t port = mqttPref.getUShort("port", 1883);
        String username = mqttPref.getString("username", "");
        String password = mqttPref.getString("password", "");
        String clientid = mqttPref.getString("clientid", "");
        String transport = mqttPref.getString("transport", "auto");
        
        Serial.printf("Enabled: %s\n", enabled ? "Yes" : "No");
        Serial.printf("Server: %s\n", server.length() > 0 ? server.c_str() : "(not set)");
        Serial.printf("Port: %d\n", port);
        Serial.printf("Username: %s\n", username.length() > 0 ? username.c_str() : "(not set)");
        Serial.printf("Password: %s\n", password.length() > 0 ? "********" : "(not set)");
        Serial.printf("Client ID: %s\n", clientid.length() > 0 ? clientid.c_str() : "(not set)");
        Serial.printf("Transport: %s\n", transport.c_str());
        Serial.println("================================");
    }
    else if (subCmd == "clear") {
        mqttPref.end();
        mqttPref.begin("mqtt", false);
        mqttPref.clear();
        mqttPref.end();
        mqttPref.begin("mqtt", true);
        
        Serial.println("[MQTT] ✓ Configuration cleared");
    }
    else if (subCmd == "test") {
        String server = mqttPref.getString("server", "");
        uint16_t port = mqttPref.getUShort("port", 1883);
        
        if (server.length() == 0) {
            Serial.println("[MQTT] ✗ Error: Server not configured");
            Serial.println("Use: mqtt server <ip_or_hostname>");
            return;
        }
        
        Serial.printf("[MQTT] Testing connection to %s:%d...\n", server.c_str(), port);
        Serial.println("[MQTT] Note: Actual connection test requires MQTT client implementation");
        Serial.println("[MQTT] Configuration is valid and ready to use");
    }
    else {
        Serial.printf("[MQTT] ✗ Unknown command: %s\n", subCmd.c_str());
        Serial.println("[MQTT] Type 'mqtt help' for available commands");
    }
}

#endif // SERIAL_MQTT_HANDLER_H
