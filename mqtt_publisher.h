#include "mqtt_lib.h"
#include "RTCManager.h"

class MQTTPublisher {
    private:
        MQTT_Lib* mqttClient;
        bool send_flag;    
        DynamicJsonDocument* doc; 
        const char* topic; 
        bool retained; 
        int qos = 0;
    public:
        MQTTPublisher(MQTT_Lib* client, const char* pub_topic, int json_size ,bool retain_msg = false, int quality_of_service = 0)
            : mqttClient(client), topic(pub_topic), retained(retain_msg), qos(quality_of_service) {
            send_flag = false;
            doc = new DynamicJsonDocument(json_size);
        }

        ~MQTTPublisher() {
            delete doc;
        }

        void settopic(const char* pub_topic) {
            topic = pub_topic;
        }

        void setJson(const DynamicJsonDocument &jsonDoc) {
            doc->clear();
            doc->set(jsonDoc);
            send_flag = true;
        }

        void setData(const char* key, const char* value) {
            (*doc)[key] = value;
            (*doc)["timestamp"] = rtc.getDateTime();
            send_flag = true;
        }

        void setData(const char* key, int value) {
            (*doc)[key] = value;
            (*doc)["timestamp"] = rtc.getDateTime();
            send_flag = true;
        }
        void setData(const char* key, float value) {
            (*doc)[key] = value;
            (*doc)["timestamp"] = rtc.getDateTime();
            send_flag = true;
        }
        
        void setData(const char* key, double value) {
            (*doc)[key] = value;
            (*doc)["timestamp"] = rtc.getDateTime();
            send_flag = true;
        }
        
        const char * getdata(const char* topic) {
            return (*doc)[topic].as<String>().c_str();
        }
        int getdataint(const char* topic) {
            return (*doc)[topic].as<int>();
        }
        float getdatafloat(const char* topic) {
            return (*doc)[topic].as<float>();
        }
        bool getdatastr(const char* topic, char* buffer, size_t bufferSize) {
            String value = (*doc)[topic].as<String>();
            if (value.length() + 1 > bufferSize) {
                return false; // Buffer too small
            }
            value.toCharArray(buffer, bufferSize);
            return true;
        }

        void loop() {
            if (send_flag && mqttClient->connectionStatus() == MQTT_CONNECTED) {
                String payload;
                serializeJson(*doc, payload);
                // Serial.println("[MQTT PUBLISH] Topic: " + String(mqtt_obj.getTopic(topic)) + " Payload: " + payload);
                send_flag = !mqttClient->publish(mqttClient->getTopic(topic).c_str(), payload.c_str(), retained);
                
            }
        }
};