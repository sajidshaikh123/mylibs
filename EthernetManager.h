#ifndef ETHERNET_MANAGER_H
#define ETHERNET_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>


class EthernetManager {
  public:
    EthernetManager(uint8_t sck , uint8_t miso , uint8_t mosi , uint8_t cs , uint8_t rst );
    EthernetManager();
    void setIPSettings(const uint8_t* mac, bool useDHCP, const char* ip, const char* subnet, const char* gateway, const char* dns);

    void begin();
    void poll();
    void startPolling(uint8_t core = 1, uint32_t intervalMs = 1000);
    uint8_t status();

  private:
    volatile uint8_t linkStatus = 255;
    uint8_t mac[6];
    uint8_t ethernet_CS;
    uint8_t ethernet_RST;
    IPAddress myIP, mySubnet, myGateway, myDNS;
    bool useDHCP;
    TaskHandle_t ethTaskHandle = nullptr;
    uint32_t pollInterval = 1000;
    static void pollTask(void* pvParameters);
};

#endif
