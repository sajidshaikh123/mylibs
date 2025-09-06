#include "esp32-hal.h"
#include "esp32-hal-gpio.h"

#include <WiFiManager.h> 

// Set your Static IP address
IPAddress local_IP(192, 168, 0, 220);
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional

// select which pin will trigger the configuration portal when set to LOW
#if defined(ESP32)
    #if CONFIG_IDF_TARGET_ESP32C6
        #define TRIGGER_PIN 9  // ESP32-C6
    #else
        #define TRIGGER_PIN 0  // Default ESP32
    #endif
#else
    #error "Unsupported board"
#endif

#define WIFI_RESTART_TIMER  (5 * 60 * 1000)

uint8_t Wifi_status = 0;
unsigned long Wifi_timer=0;

bool wm_nonblocking = true;

int timeout = 300; // seconds to run for

WiFiManager wm; // global wm instance
WiFiManagerParameter custom_field; // global param ( for non blocking w params )

void wifiInit(){
    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    WiFi.mode(WIFI_STA);

    // if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    //   Serial.println("STA Failed to configure");
    // }

    Serial.println(wm.getWiFiSSID());
    Serial.println(wm.getWiFiPass());
    Serial.println(TRIGGER_PIN);
    WiFi.begin(wm.getWiFiSSID(),wm.getWiFiPass());

    Wifi_timer = millis();

    // wm.setConfigPortalTimeout(120);
      
    // if (!wm.autoConnect()) {
    //   Serial.println("failed to connect or hit timeout");
    //   delay(3000);
    //   // ESP.restart();
    // } else {
    //   //if you get here you have connected to the WiFi
    //   Serial.println("connected...yeey :)");
    // }
    
}
void readwificonfiginput(){

  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    WiFiManager wm;    

    Serial.println("In WiFi configuration Switch");

    //reset settings - for testing
    //wm.resetSettings();
  
    // set configportal timeout
    wm.setConfigPortalTimeout(timeout);

    if (!wm.startConfigPortal("OnDemandAP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

  }
}

void wificonfig(){

  
  WiFiManager wm;    

  Serial.println("In WiFi configuration Serial");

  //reset settings - for testing
  //wm.resetSettings();

  // set configportal timeout
  wm.setConfigPortalTimeout(timeout);

  if (!wm.startConfigPortal("OnDemandAP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");


}

void WiFiManagerloop(){
    if(wm_nonblocking) wm.process();

    if(WiFi.status() == WL_CONNECTED) {
        if(Wifi_status == 0){
            Wifi_status = 1;
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
        }
    }else{
         if(Wifi_status == 1){
            Wifi_status = 0;
            Serial.println("WiFi DisConnected");
            Wifi_timer = millis();
         }
      
         if((millis() - Wifi_timer ) > WIFI_RESTART_TIMER){
            Serial.println("Restarting");
            ESP.restart();
         }
    }
}