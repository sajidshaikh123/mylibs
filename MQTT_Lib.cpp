// MQTT_Lib.cpp - Implementation file for MQTT Library
#include "MQTT_Lib.h"
#include <WiFi.h>
#include "RTCManager.h"

// Declare external rtc instance from iotboard.h
extern RTCManager rtc;


MQTT_Lib::MQTT_Lib(){
    // subtopicsPref.begin("subtopics", true);
    // cached_company = subtopicsPref.getString("company", "embedsol");
    // cached_location = subtopicsPref.getString("location", "bhosari");
    // cached_department = subtopicsPref.getString("department", "production");
    // cached_line = subtopicsPref.getString("line", "test");
    // cached_machine = subtopicsPref.getString("machine", "testmachine");
    // subtopicsPref.end();
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
    String temp_topic = "empty";

    if(cached_company.length() == 0 || cached_location.length() == 0 || cached_department.length() == 0 || cached_line.length() == 0 || cached_machine.length() == 0) {
        subtopicsPref.begin("subtopics", true);
        cached_company = subtopicsPref.getString("company", "embedsol");
        cached_location = subtopicsPref.getString("location", "bhosari");
        cached_department = subtopicsPref.getString("department", "production");
        cached_line = subtopicsPref.getString("line", "test");
        cached_machine = subtopicsPref.getString("machine", "testmachine");
        subtopicsPref.end();
    }
    
    temp_topic = "";
    temp_topic += cached_company;
    temp_topic += "/";
    temp_topic += cached_location; 
    temp_topic += "/";
    temp_topic += cached_department;
    temp_topic += "/";
    temp_topic += cached_line;
    temp_topic += "/";
    temp_topic += cached_machine;
    temp_topic += "/";
    
    if(temp_topic.length() == 0 || temp_topic == "//////") {
        return "empty";
    }
    
    temp_topic += request;
    
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
    PubSubClient::setBufferSize(4096); // Reduced buffer size to prevent heap corruption (was 32000)
    PubSubClient::setKeepAlive(15); // Keep-alive interval for connection (increased for stability)
    PubSubClient::setSocketTimeout(5); // Socket timeout in seconds (increased for reliability)
  
    
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
    
    if(obj.containsKey("company")) cached_company = obj["company"].as<String>();
    if(obj.containsKey("company_name")) cached_company = obj["company_name"].as<String>();
    if(obj.containsKey("companyname")) cached_company = obj["companyname"].as<String>();
    
    if(obj.containsKey("location")) cached_location = obj["location"].as<String>();
    if(obj.containsKey("department")) cached_department = obj["department"].as<String>();
    if(obj.containsKey("line")) cached_line = obj["line"].as<String>();
    
    if(obj.containsKey("machine")) cached_machine = obj["machine"].as<String>();
    if(obj.containsKey("machinename")) cached_machine = obj["machinename"].as<String>();
    if(obj.containsKey("machine_name")) cached_machine = obj["machine_name"].as<String>();

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

    
    PubSubClient::subscribe(getTopic(sub_to).c_str());
    
    
    
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
