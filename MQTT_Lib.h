// MQTT_Lib.h - Header file for MQTT Library
#ifndef MQTT_LIB_H
#define MQTT_LIB_H

#include <Arduino.h>
// #include <MQTTPubSubClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <map>
#include <Preferences.h>

// Define reconnect and loop intervals
#define MQTT_RECONNECT_INTERVAL 5000  // Time interval for MQTT reconnect attempts (in ms)
#define MQTT_LOOP_INTERVAL 50          // Time interval for calling the loop function (in ms)

extern Preferences subtopicsPref;

class MQTT_Lib : public PubSubClient {
public:
    MQTT_Lib();
    String getTopic(String request); // Construct full topic string
    void config(const char *ip, uint16_t port, const char *user, const char *password, const char *willMsg, Client &client);
    void setMacAddress(String temp_mac);
    void setClient(Client &client);
    String getMacAddress();
    void setsubscribeto(String _sub_to);
    void begin();  // Initialize MQTT connection
    bool connect(); // Attempt to connect to MQTT broker
    void loop();  // Handle MQTT loop operations
    void setsubtopic(const DynamicJsonDocument &obj);
    void setCallback(MQTT_CALLBACK_SIGNATURE); // Set MQTT message callback function
    uint8_t connectionStatus();
    String getMacTopic(String request);

private:
    

    String cached_company ="";
    String cached_location ="";
    String cached_department ="";
    String cached_line ="";
    String cached_machine ="";

    String macAddress = "00:00:00:00:00:00"; 
    String sub_to = "+";  
    String mqtt_user;
    String mqtt_password;
    String will_message;
    unsigned long mqtt_timer = 0; // Timer for reconnect attempts
    unsigned long loop_timer = 0; // Timer for loop execution interval
    uint8_t counter = 0; // Connection attempt counter
    bool update_success_flag = false; // Flag for tracking MQTT connection status
};

#endif