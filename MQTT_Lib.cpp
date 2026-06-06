// MQTT_Lib.cpp - Implementation file for MQTT Library
#include "MQTT_Lib.h"
#include <WiFi.h>
#include "RTCManager.h"

// Declare external rtc instance from iotboard.h
extern RTCManager rtc;

Preferences subtopicsPref;

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

String MQTT_Lib::getmachine(){
    return cached_machine;
}
String MQTT_Lib::getcompany(){
    return cached_company;
}
String MQTT_Lib::getlocation(){
    return cached_location;
}
String MQTT_Lib::getdepartment(){
    return cached_department;
}
String MQTT_Lib::getline(){
    return cached_line;
}

String MQTT_Lib::getTopic(String request) {
    String temp_topic = "empty";

    if(cached_company.length() == 0 || cached_location.length() == 0 || cached_department.length() == 0 || cached_machine.length() == 0) {
        subtopicsPref.begin("subtopics", true);
        cached_company = subtopicsPref.getString("company", "embedsol");
        cached_location = subtopicsPref.getString("location", "bhosari");
        cached_department = subtopicsPref.getString("department", "production");
        cached_line = subtopicsPref.getString("line", "");
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
    if(cached_line.length() ){
        temp_topic += cached_line;
        temp_topic += "/";
    }
    temp_topic += cached_machine;
    temp_topic += "/";
    
    if(temp_topic.length() == 0 || temp_topic == "//////") {
        return "empty";
    }
    
    temp_topic += request;
    
    return temp_topic;
}

void MQTT_Lib::loadSubtopicsFromPreferences() {
    subtopicsPref.begin("subtopics", true);
    cached_company = subtopicsPref.getString("company", "embedsol");
    cached_location = subtopicsPref.getString("location", "bhosari");
    cached_department = subtopicsPref.getString("department", "production");
    cached_line = subtopicsPref.getString("line", "");
    cached_machine = subtopicsPref.getString("machine", "testmachine");
    subtopicsPref.end();
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
    PubSubClient::setBufferSize(32000); // Reduced buffer size to prevent heap corruption (was 32000)
    PubSubClient::setKeepAlive(5); // Keep-alive interval for connection (increased for stability)
    PubSubClient::setSocketTimeout(1); // Socket timeout in seconds (increased for reliability)
  
    
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

void MQTT_Lib::setsubtopic(String _company, String _location, String _department, String _line, String _machine) {
    cached_company = _company;
    cached_location = _location;
    cached_department = _department;
    cached_line = _line;
    cached_machine = _machine;
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
    // status["status"] = "connected";
    // status["timestamp"] = rtc.getDateTime();
    // serializeJsonPretty(status, buffer);
    // PubSubClient::publish(getTopic("events/connection_status").c_str() , buffer,true) ;
    // PubSubClient::publish(getTopic("status").c_str() , buffer) ;

    return true;

}

String MQTT_Lib::getMacTopic(String request){
    String temp_topic = "devices/" + macAddress + "/" + request;
    return temp_topic;
}


// FreeRTOS background task: runs connect() so the main loop never blocks.
// Deletes itself when done (one-shot per attempt).
void MQTT_Lib::_connectTaskFn(void *pvParameters) {
    MQTT_Lib *self = static_cast<MQTT_Lib*>(pvParameters);
    Serial.println("connecting to MQTT");
    self->connect();
    self->_connecting        = false;
    self->_connectTaskHandle = NULL;
    vTaskDelete(NULL);
}

// Handle MQTT loop operations with a 50ms interval
void MQTT_Lib::loop() {
    if ((millis() - loop_timer) >= MQTT_LOOP_INTERVAL) {
        loop_timer = millis();
        if (MQTT_CONNECTED == PubSubClient::state()) {
            PubSubClient::loop();  // Maintain MQTT connection and handle incoming messages
            mqtt_timer = millis();
        } else {
            // If disconnected, spawn a background task to reconnect so connect()
            // doesn't block the main loop for ~3 s on each failed attempt.
            if (!_connecting && (millis() - mqtt_timer) > MQTT_RECONNECT_INTERVAL) {
                mqtt_timer  = millis();
                _connecting = true;
                xTaskCreate(_connectTaskFn, "mqtt_conn", 4096, this, 1,
                            &_connectTaskHandle);
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
