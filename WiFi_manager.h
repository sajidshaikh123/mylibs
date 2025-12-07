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
unsigned long wifi_connect_timer = 0;
uint8_t wifi_init_state = 0; // 0=init, 1=connecting, 2=connected, 3=portal

bool wm_nonblocking = true;

int timeout = 300; // seconds to run for

WiFiManager wm; // global wm instance
WiFiManagerParameter custom_field; // global param ( for non blocking w params )

void wifiInit(){
    pinMode(TRIGGER_PIN, INPUT_PULLUP);
    WiFi.mode(WIFI_STA);

    Serial.println("\n[WiFi] Initializing WiFi...");
    Serial.print("[WiFi] Trigger Pin: ");
    Serial.println(TRIGGER_PIN);
    
    // Get saved credentials
    String ssid = wm.getWiFiSSID();
    String pass = wm.getWiFiPass();
    
    Serial.print("[WiFi] Saved SSID: ");
    Serial.println(ssid.length() > 0 ? ssid : "(none)");
    
    // Check if credentials are saved
    if (ssid.length() > 0) {
        Serial.println("[WiFi] Will attempt to connect with saved credentials...");
        
        // Convert String to const char* for WiFi.begin()
        WiFi.begin(ssid.c_str(), pass.c_str());
        
        wifi_init_state = 1; // connecting state
        wifi_connect_timer = millis();
    } else {
        Serial.println("[WiFi] No saved credentials found");
        Serial.println("[WiFi] Will start configuration portal...");
        
        wifi_init_state = 3; // portal state
    }
    
    Wifi_timer = millis();
}
void readwificonfiginput(){
  if (digitalRead(TRIGGER_PIN) == LOW) {
    Serial.println("\n[WiFi] Configuration button pressed!");
    Serial.println("[WiFi] Starting configuration portal...");
    Serial.println("[WiFi] AP Name: EmailGateway_Config");
    Serial.println("[WiFi] Password: configme123");

    // Use global wm instance
    // WiFiManager wm;    

    //reset settings - for testing
    //wm.resetSettings();
  
    // set configportal timeout
    wm.setConfigPortalTimeout(timeout);

    if (!wm.startConfigPortal("EmailGateway_Config", "configme123")) {
      Serial.println("[WiFi] ✗ Failed to connect or hit timeout");
      Serial.println("[WiFi] Restarting in 3 seconds...");
      delay(3000);
      ESP.restart();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("[WiFi] ✓ Connected successfully!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
    Wifi_status = 1;
  }
}

void wificonfig(){
  Serial.println("\n[WiFi] Starting WiFi configuration portal...");
  Serial.println("[WiFi] AP Name: EmailGateway_Config");
  Serial.println("[WiFi] Password: configme123");
  Serial.println("[WiFi] Portal will run for 5 minutes");

  // Use global wm instance, don't create new one
  // WiFiManager wm;    

  //reset settings - for testing
  //wm.resetSettings();

  // set configportal timeout
  wm.setConfigPortalTimeout(timeout);

  if (!wm.startConfigPortal("EmailGateway_Config", "configme123")) {
    Serial.println("[WiFi] ✗ Failed to connect or hit timeout");
    Serial.println("[WiFi] Restarting in 3 seconds...");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("[WiFi] ✓ Connected successfully!");
  Serial.print("[WiFi] IP Address: ");
  Serial.println(WiFi.localIP());
  Wifi_status = 1;
}

void WiFiManagerloop(){
    if(wm_nonblocking) wm.process();

    // State machine for non-blocking WiFi connection
    switch(wifi_init_state) {
        case 1: // Connecting state
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("\n[WiFi] ✓ Connected successfully!");
                Serial.print("[WiFi] IP Address: ");
                Serial.println(WiFi.localIP());
                Wifi_status = 1;
                wifi_init_state = 2; // connected state
            } else if (millis() - wifi_connect_timer > 10000) { // 10 second timeout
                Serial.println("\n[WiFi] ✗ Failed to connect with saved credentials");
                Serial.println("[WiFi] Starting configuration portal...");
                wifi_init_state = 3; // move to portal state
            }
            break;
            
        case 3: // Portal state
            wm.setConfigPortalTimeout(300); // 5 minutes timeout
            wm.setConfigPortalBlocking(false);
            
            if (wm.autoConnect("EmailGateway_Config", "configme123")) {
                Serial.println("[WiFi] ✓ Connected via config portal!");
                Serial.print("[WiFi] IP Address: ");
                Serial.println(WiFi.localIP());
                Wifi_status = 1;
                wifi_init_state = 2; // connected state
            }
            // If autoConnect returns false, portal is running
            // It will continue in non-blocking mode
            break;
            
        case 2: // Connected state - monitor connection
            if(WiFi.status() == WL_CONNECTED) {
                if(Wifi_status == 0){
                    Wifi_status = 1;
                    Serial.println("[WiFi] ✓ WiFi reconnected");
                    Serial.print("[WiFi] IP address: ");
                    Serial.println(WiFi.localIP());
                }
            } else {
                if(Wifi_status == 1){
                    Wifi_status = 0;
                    Serial.println("[WiFi] ✗ WiFi disconnected");
                    Wifi_timer = millis();
                }
              
                if((millis() - Wifi_timer) > WIFI_RESTART_TIMER){
                    Serial.println("[WiFi] No connection for 5 minutes, restarting...");
                    ESP.restart();
                }
            }
            break;
    }
}