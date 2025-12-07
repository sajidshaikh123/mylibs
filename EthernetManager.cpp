#include "EthernetManager.h"
#include "pindefinition.h"

EthernetManager::EthernetManager(uint8_t sck , uint8_t miso , uint8_t mosi , uint8_t cs , uint8_t rst )
  : ethernet_CS(cs), ethernet_RST(rst) {
  SPI.begin(sck, miso, mosi, cs);
  pinMode(RST_PIN,OUTPUT);
}

EthernetManager::EthernetManager(){
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  ethernet_CS = CS_PIN;
  ethernet_RST = RST_PIN;
  pinMode(RST_PIN,OUTPUT);
}

void EthernetManager::setIPSettings(const uint8_t* mac, bool useDHCP,
    const char* ip, const char* subnet,
    const char* gateway, const char* dns) {
    memcpy(this->mac, mac, 6);       // Store MAC address
    this->useDHCP = useDHCP;
    myIP.fromString(ip);
    mySubnet.fromString(subnet);
    myGateway.fromString(gateway);
    myDNS.fromString(dns);

}

void EthernetManager::begin() {
    // Ethernet.softReset();
    digitalWrite(RST_PIN,LOW);
    delay(10);
    digitalWrite(RST_PIN,HIGH);
    delay(10);
    Ethernet.init(ethernet_CS);

    if (useDHCP) {
        // Serial.println("DHCP Enabled");
        Ethernet.begin(mac);
    } else {
        // Serial.println("STORED Static IP");
        // Serial.println(myIP.toString());
        // Serial.println(mySubnet.toString());
        // Serial.println(myGateway.toString());
        // Serial.println(myDNS.toString());
        // Serial.println("Using Static IP");
        Ethernet.begin(mac, myIP, myDNS, myGateway, mySubnet);
        Ethernet.setLocalIP(myIP);
        Ethernet.setSubnetMask(mySubnet);
        Ethernet.setGatewayIP(myGateway);
        Ethernet.setDnsServerIP(myDNS);
    }

    // Serial.println(Ethernet.localIP());
    // Serial.println(Ethernet.subnetMask());
    // Serial.println(Ethernet.gatewayIP());
    // Serial.println(Ethernet.dnsServerIP());
}

uint8_t EthernetManager::status() {
    return (linkStatus);
}
void EthernetManager::poll() {

  
  uint8_t currentStatus = Ethernet.linkStatus();
  if (currentStatus != linkStatus) {
    linkStatus = currentStatus;
    // if (linkStatus == Unknown || linkStatus == LinkOFF) {
    //   // Serial.println(" Ethernet Disconnected");
      
    // } else {
    //   // Serial.println(" Ethernet Connected");
    //   // Serial.println(Ethernet.localIP());
    // }
  }
  if (linkStatus == Unknown || linkStatus == LinkOFF) {
      begin();
  }
//   Serial.println("STATUS : "+String(currentStatus));
}

// Static polling task function
void EthernetManager::pollTask(void* pvParameters) {
  EthernetManager* instance = static_cast<EthernetManager*>(pvParameters);
  TickType_t delayTicks = pdMS_TO_TICKS(instance->pollInterval);
  for (;;) {
    instance->poll();
    yield();
    vTaskDelay(delayTicks);
  }
}

// Start the FreeRTOS task
void EthernetManager::startPolling(uint8_t core, uint32_t intervalMs) {
  pollInterval = intervalMs;
  if (ethTaskHandle == nullptr) {
    xTaskCreatePinnedToCore(
      pollTask,
      "EthernetPollTask",
      2048,
      this,
      2 ,
      &ethTaskHandle,
      core
    );
  }
}
