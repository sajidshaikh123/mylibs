// MQTT_Lib.cpp - Implementation file for MQTT Library
#include "MQTT_Lib.h"
#include <WiFi.h>
#include "RTCManager.h"

// Declare external rtc instance from iotboard.h
extern RTCManager rtc;

// Constructor - Initializes MQTT settings

MQTT_Lib::MQTT_Lib() : subtopic(300){

}

void MQTT_Lib::setMacAddress(String temp_mac){
    macAddress = temp_mac;
}
String MQTT_Lib::getMacAddress(){
    return macAddress;
}
// Generate full topic string based on configuration
// Update the getTopic function with flexible key validation

String MQTT_Lib::getTopic(String request) {
    String temp_topic = "";
    
    // Validate if subtopic has data
    if (subtopic.size() == 0 || subtopic.isNull()) {
        Serial.println("⚠ Warning: Subtopic is empty or null");
        return temp_topic; // Return empty string
    }
    
    // Extract company name (handle both formats)
    String company_name = "";
    if (subtopic.containsKey("company_name")) {
        company_name = String((const char *)subtopic["company_name"]);
    } else if (subtopic.containsKey("companyname")) {
        company_name = String((const char *)subtopic["companyname"]);
    }
    
    if (company_name.length() == 0 || company_name == "null") {
        Serial.println("⚠ Warning: company_name/companyname is empty or null");
        return temp_topic;
    }
    
    // Extract location
    String location = "";
    if (subtopic.containsKey("location")) {
        location = String((const char *)subtopic["location"]);
    }
    
    if (location.length() == 0 || location == "null") {
        Serial.println("⚠ Warning: location is empty or null");
        return temp_topic;
    }
    
    // Extract department
    String department = "";
    if (subtopic.containsKey("department")) {
        department = String((const char *)subtopic["department"]);
    }
    
    if (department.length() == 0 || department == "null") {
        Serial.println("⚠ Warning: department is empty or null");
        return temp_topic;
    }

    // Extract department
    String line = "";
    if (subtopic.containsKey("line")) {
        line = String((const char *)subtopic["line"]);
    }
    
    // if (line.length() == 0 || line == "null") {
    //     Serial.println("⚠ Warning: line is empty or null");
    //     return temp_topic;
    // }
    
    // Extract machine name (handle both formats)
    String machinename = "";
    if (subtopic.containsKey("machinename")) {
        machinename = String((const char *)subtopic["machinename"]);
    } else if (subtopic.containsKey("machine_name")) {
        machinename = String((const char *)subtopic["machine_name"]);
    }
    
    if (machinename.length() == 0 || machinename == "null") {
        Serial.println("⚠ Warning: machinename/machine_name is empty or null");
        return temp_topic;
    }
    
    // Build topic string
    temp_topic = company_name;
    temp_topic += "/";
    temp_topic += location;
    temp_topic += "/";
    temp_topic += department;
    temp_topic += "/";

    if(line.length() > 0){
        temp_topic += line;
        temp_topic += "/";
    }
    
    // Handle special case for wildcard subscription
    if (machinename.equals("+")) {
        // Check for gateway field (both formats)
        String gateway = "";
        if (subtopic.containsKey("gateway")) {
            gateway = String((const char *)subtopic["gateway"]);
        }
        
        if (gateway.length() > 0 && gateway != "null") {
            temp_topic += gateway;
        } else {
            temp_topic += machinename;
        }
    } else {
        temp_topic += machinename;
    }
    
    temp_topic += "/" + request;
    
    // Final validation - ensure topic is valid
    if (temp_topic.length() == 0 || temp_topic == "/") {
        Serial.println("⚠ Warning: Generated topic is invalid");
        return "";
    }
    
    return temp_topic;
}

void MQTT_Lib::config(const char *ip, uint16_t port, const char *user, const char *password, const char *willMsg, Client &client) {
    IPAddress mqttIP;
    mqttIP.fromString(ip);
    Serial.print("MQTT IP:");
    Serial.println(mqttIP);
    Serial.print("MQTT PORT:");
    Serial.println(port);
    
    PubSubClient::setClient(client);
    PubSubClient::setServer(mqttIP, port); // Convert port from string to integer
    PubSubClient::setBufferSize(32000); // Set buffer size for large MQTT packets
    PubSubClient::setKeepAlive(60); // Keep-alive interval for connection
    PubSubClient::setSocketTimeout(60); // Socket timeout in seconds
  
    
    mqtt_user = String(user);
    mqtt_password = String(password);
    will_message = String(willMsg);
}

void MQTT_Lib::setClient(Client &client){
    PubSubClient::setClient(client);
}

// Start MQTT connection
void MQTT_Lib::begin() {
    connect();
}

// Configure MQTT topic details
void MQTT_Lib::setsubtopic(const DynamicJsonDocument &obj) {
    subtopic = obj;
}

void MQTT_Lib::setsubscribeto(String _sub_to){
    sub_to = _sub_to;
}

// Attempt to connect to MQTT broker
bool MQTT_Lib::connect() {
    counter = 0;
    String will_topic = getTopic("events/connection_status");
    char buffer[200];
    DynamicJsonDocument status(100);
    status["status"] = "disconnected"; // Mark as disconnected initially
    
    serializeJsonPretty(status, buffer);
    
    // Try to establish connection with MQTT broker
    
    if(!PubSubClient::connect(macAddress.c_str(), mqtt_user.c_str(), mqtt_password.c_str(), will_topic.c_str(), 1, true, buffer)) {
        Serial.print(".");
        Serial.print(PubSubClient::state()); // Print connection state
            return false; // Exit if connection fails after multiple attempts
        
    }
    Serial.println("Connected!");
    
    // Subscribe to device-specific topic
    PubSubClient::subscribe(String("devices/" + macAddress + "/+").c_str());

    if(subtopic.size() > 0 ){
        PubSubClient::subscribe(getTopic(sub_to).c_str());
    }
    
    
    // Mark as connected
    status["status"] = "connected";
    status["timestamp"] = rtc.getDateTime();
    serializeJsonPretty(status, buffer);
    PubSubClient::publish(getTopic("events/connection_status").c_str() , buffer,true) ;
    // PubSubClient::publish(getTopic("status").c_str() , buffer) ;

    return true;

}

String MQTT_Lib::getMacTopic(String request){
    String temp_topic = "devices/" + macAddress + "/" + request;
    return temp_topic;
}


// Handle MQTT loop operations with a 50ms interval
void MQTT_Lib::loop() {
    if ((millis() - loop_timer) >= MQTT_LOOP_INTERVAL) {
        loop_timer = millis();
        if (MQTT_CONNECTED == PubSubClient::state()) {
            PubSubClient::loop();  // Maintain MQTT connection and handle incoming messages
            mqtt_timer = millis();
        } else {
            // If disconnected, attempt reconnection at defined intervals
            if ((millis() - mqtt_timer) > MQTT_RECONNECT_INTERVAL) {
                mqtt_timer = millis();
                Serial.println("connecting to MQTT");
                connect();
            }
        }
    }
}

uint8_t MQTT_Lib::connectionStatus(){
    return (PubSubClient::state());
}


// Set MQTT callback function for handling incoming messages
void MQTT_Lib::setCallback(MQTT_CALLBACK_SIGNATURE) {
    PubSubClient::setCallback(callback);
}
