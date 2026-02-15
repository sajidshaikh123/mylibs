#include <Arduino.h>
#line 1 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\opcua_server_example\\opcua_server_example.ino"
/*
 * OPC UA Server Example for ESP32-S3
 * 
 * This example implements a basic OPC UA binary protocol server.
 * Limited functionality - use Python bridge for full UAExpert support.
 * 
 * Protocol: OPC UA Binary (discovery only)
 * Default Port: 4840
 */

#include <iotboard.h>
#include "opcua_server_open62541.h"

// Create OPC UA Server instance (Basic Binary Protocol)
OPCUAServer_Real opcuaServer(4840);

// Simulation variables
float temperature = 25.0;
float humidity = 50.0;
int32_t counter = 0;
bool relay1State = false;
bool relay2State = false;

// Timing
DelayTimer updateTimer(1000); // Update every second
DelayTimer sensorTimer(500);  // Simulate sensor readings
DelayTimer statusTimer(5000); // Status check every 5 seconds

#line 29 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\opcua_server_example\\opcua_server_example.ino"
void setup();
#line 95 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\opcua_server_example\\opcua_server_example.ino"
void loop();
#line 123 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\opcua_server_example\\opcua_server_example.ino"
void setupOPCUANodes();
#line 163 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\opcua_server_example\\opcua_server_example.ino"
void simulateSensors();
#line 183 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\opcua_server_example\\opcua_server_example.ino"
void updateOPCUAValues();
#line 211 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\opcua_server_example\\opcua_server_example.ino"
void handleRelayOutputs();
#line 228 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\opcua_server_example\\opcua_server_example.ino"
void inputISR();
#line 29 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\opcua_server_example\\opcua_server_example.ino"
void setup() {
    // Initialize board
    boardinit();
    
    delay(5000); // Wait for network initialization
    
    // Print PSRAM info
    Serial.println("\n=== Memory Information ===");
    if(psramFound()) {
        Serial.printf("PSRAM: %d bytes (%.2f MB)\n", 
                      ESP.getPsramSize(), ESP.getPsramSize() / (1024.0 * 1024.0));
        Serial.printf("Free PSRAM: %d bytes (%.2f MB)\n", 
                      ESP.getFreePsram(), ESP.getFreePsram() / (1024.0 * 1024.0));
    } else {
        Serial.println("⚠ PSRAM not found - enable in Arduino IDE");
    }
    Serial.printf("Free Heap: %d bytes (%.2f KB)\n", 
                  ESP.getFreeHeap(), ESP.getFreeHeap() / 1024.0);
    Serial.println("==========================\n");
    
    // Initialize OPC UA server buffer
    if(!opcuaServer.initBuffer()) {
        Serial.println("[Main] ✗ Failed to initialize OPC UA server!");
        Serial.println("[Main] System halted - restart required");
        while(1) { delay(1000); }
    }
    
    // Configure server
    opcuaServer.setServerName("ESP32-S3 Industrial IoT Gateway");
    opcuaServer.setApplicationUri("urn:embedsol:esp32s3:iotgateway");
    
    // Create OPC UA nodes
    setupOPCUANodes();
    
    // Start OPC UA server
    bool serverStarted = false;
    
    if(ethManager.status() == 1) {
        Serial.println("[Main] Starting Real OPC UA Server on Ethernet...");
        serverStarted = opcuaServer.begin(false); // false = use Ethernet
    }
    
    if(!serverStarted && WiFi.status() == WL_CONNECTED) {
        Serial.println("[Main] Starting Real OPC UA Server on WiFi...");
        serverStarted = opcuaServer.begin(true); // true = use WiFi
    }
    
    if(serverStarted) {
        Serial.println("[Main] ✓ Real OPC UA Server started successfully!");
        Serial.println("[Main] Available nodes: " + String(opcuaServer.getNodeCount()));
        Serial.println("\n=== System Ready ===");
        Serial.println("Real OPC UA Server is running!");
        
        if(ethManager.status() == 1) {
            Serial.println("Connect UAExpert to: opc.tcp://" + Ethernet.localIP().toString() + ":4840");
        } else if(WiFi.status() == WL_CONNECTED) {
            Serial.println("Connect UAExpert to: opc.tcp://" + WiFi.localIP().toString() + ":4840");
        }
        
        Serial.println("====================\n");
    } else {
        Serial.println("[Main] ✗ Failed to start OPC UA Server!");
        Serial.println("[Main] Please check network connection.");
    }
}

void loop() {
    // Run board loop (handles network, MQTT, etc.)
    boardloop();
    
    // Run OPC UA server loop
    opcuaServer.loop();
    
    // Update sensor simulations
    if(sensorTimer.ontime()) {
        simulateSensors();
    }
    
    // Update OPC UA values
    if(updateTimer.ontime()) {
        updateOPCUAValues();
    }
    
    // Print status
    if(statusTimer.ontime()) {
        Serial.printf("[Status] Free Heap: %d bytes, Uptime: %d seconds\n", 
                      ESP.getFreeHeap(), millis() / 1000);
    }
    
    // Handle relay outputs
    handleRelayOutputs();
}

// Setup OPC UA nodes
void setupOPCUANodes() {
    Serial.println("[OPC UA] Creating nodes...");
    
    // System Information Nodes (Read-only)
    int32_t zero = 0;
    opcuaServer.addVariable("system.uptime", "System Uptime (seconds)", "Int32", &zero, false);
    opcuaServer.addVariable("system.freeheap", "Free Heap Memory (bytes)", "Int32", &zero, false);
    opcuaServer.addVariable("system.cpufreq", "CPU Frequency (MHz)", "Int32", &zero, false);
    
    // Network Information (Read-only)
    String emptyStr = "";
    opcuaServer.addVariable("network.ip", "IP Address", "String", &emptyStr, false);
    opcuaServer.addVariable("network.mac", "MAC Address", "String", &emptyStr, false);
    
    // Sensor Nodes (Read-only)
    float zeroFloat = 0.0f;
    opcuaServer.addVariable("sensors.temperature", "Temperature (°C)", "Float", &zeroFloat, false);
    opcuaServer.addVariable("sensors.humidity", "Humidity (%)", "Float", &zeroFloat, false);
    opcuaServer.addVariable("sensors.pressure", "Pressure (hPa)", "Float", &zeroFloat, false);
    
    // Counter Node (Read-only)
    opcuaServer.addVariable("data.counter", "Counter", "Int32", &zero, false);
    
    // Control Nodes (Read-Write)
    bool falseVal = false;
    opcuaServer.addVariable("control.relay1", "Relay 1", "Boolean", &falseVal, true);
    opcuaServer.addVariable("control.relay2", "Relay 2", "Boolean", &falseVal, true);
    opcuaServer.addVariable("control.setpoint", "Temperature Setpoint (°C)", "Float", &zeroFloat, true);
    String modeStr = "AUTO";
    opcuaServer.addVariable("control.mode", "Operation Mode", "String", &modeStr, true);
    
    // Status Nodes (Read-only)
    opcuaServer.addVariable("status.mqtt", "MQTT Connected", "Boolean", &falseVal, false);
    opcuaServer.addVariable("status.ethernet", "Ethernet Status", "Boolean", &falseVal, false);
    opcuaServer.addVariable("status.wifi", "WiFi Status", "Boolean", &falseVal, false);
    
    Serial.println("[OPC UA] ✓ All nodes created successfully!");
}

// Simulate sensor readings
void simulateSensors() {
    // Simulate temperature with some variation
    static float tempOffset = 0;
    tempOffset += (random(-10, 11) / 100.0);
    if(tempOffset > 2.0) tempOffset = 2.0;
    if(tempOffset < -2.0) tempOffset = -2.0;
    temperature = 25.0 + tempOffset;
    
    // Simulate humidity
    static float humOffset = 0;
    humOffset += (random(-10, 11) / 100.0);
    if(humOffset > 5.0) humOffset = 5.0;
    if(humOffset < -5.0) humOffset = -5.0;
    humidity = 50.0 + humOffset;
    
    // Increment counter
    counter++;
}

// Update OPC UA node values
void updateOPCUAValues() {
    // Update system information
    opcuaServer.writeInt32("system.uptime", millis() / 1000);
    opcuaServer.writeInt32("system.freeheap", ESP.getFreeHeap());
    opcuaServer.writeInt32("system.cpufreq", ESP.getCpuFreqMHz());
    
    // Update network information
    if(ethManager.status() == 1) {
        opcuaServer.writeString("network.ip", Ethernet.localIP().toString());
    } else if(WiFi.status() == WL_CONNECTED) {
        opcuaServer.writeString("network.ip", WiFi.localIP().toString());
    }
    
    // Update sensor values
    opcuaServer.writeFloat("sensors.temperature", temperature);
    opcuaServer.writeFloat("sensors.humidity", humidity);
    opcuaServer.writeFloat("sensors.pressure", 1013.25 + random(-10, 11) / 10.0);
    
    // Update counter
    opcuaServer.writeInt32("data.counter", counter);
    
    // Update status
    opcuaServer.writeBool("status.mqtt", mqtt_connected);
    opcuaServer.writeBool("status.ethernet", ethManager.status() == 1);
    opcuaServer.writeBool("status.wifi", WiFi.status() == WL_CONNECTED);
}

// Handle relay outputs
void handleRelayOutputs() {
    static bool lastRelay1 = false;
    static bool lastRelay2 = false;
    
    // Update physical outputs when state changes
    if(relay1State != lastRelay1) {
        outputExpander.writeOutput(0, relay1State);
        lastRelay1 = relay1State;
    }
    
    if(relay2State != lastRelay2) {
        outputExpander.writeOutput(1, relay2State);
        lastRelay2 = relay2State;
    }
}

// Serial command handler for testing
void inputISR() {
    // Handle input interrupt
}

