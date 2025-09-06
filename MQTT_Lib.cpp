// MQTT_Lib.cpp - Implementation file for MQTT Library
#include "MQTT_Lib.h"
#include <WiFi.h>

// Constructor - Initializes MQTT settings

MQTT_Lib::MQTT_Lib() : subtopic(300){

}

void MQTT_Lib::setMacAddress(String temp_mac){
    macAddress = temp_mac;
}
// Generate full topic string based on configuration
String MQTT_Lib::getTopic(String request) {
    String temp_topic="";
    if(subtopic.size() > 0){
    temp_topic  = String((const char *)subtopic["company_name"]);
    temp_topic += String("/");
    temp_topic += String((const char *)subtopic["location"]);
    temp_topic += String("/");
    temp_topic += String((const char *)subtopic["department"]);
    temp_topic += String("/");
    
    
    if(String((const char *)subtopic["machinename"]).equals("+")){
        temp_topic += String((const char *)subtopic["gateway"]);
    }else{
        temp_topic += String((const char *)subtopic["machinename"]);
    }
    temp_topic += String("/") + request;
    // Serial.println(temp_topic);

    // serializeJsonPretty(subtopic, Serial);
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
    String will_topic = getTopic("connection/status");
    char buffer[200];
    DynamicJsonDocument status(100);
    status["connected"] = 0; // Mark as disconnected initially
    serializeJsonPretty(status, buffer);
    
    // Try to establish connection with MQTT broker
    
    if(!PubSubClient::connect(macAddress.c_str(), mqtt_user.c_str(), mqtt_password.c_str(), will_topic.c_str(), 0, true, buffer)) {
        Serial.print(".");
        Serial.print(PubSubClient::state()); // Print connection state
            return false; // Exit if connection fails after multiple attempts
        
    }
    Serial.println("Connected!");
    
    // Subscribe to device-specific topic
    PubSubClient::subscribe(String("devices/" + macAddress + "/+").c_str());

    PubSubClient::subscribe(getTopic(sub_to).c_str());
    
    // Mark as connected
    status["connected"] = 1;
    serializeJsonPretty(status, buffer);
    PubSubClient::publish(getTopic("connection/status").c_str() , buffer,true) ;
    PubSubClient::publish(getTopic("status").c_str() , buffer) ;

    return true;

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
