#include "HardwareSerial.h"
#include <ModbusRTU.h>
#include "pindefinition.h"

ModbusRTU mb;

void modbusInit(){
    Serial2.begin(9600,SERIAL_8N1,RX2_PIN,TX2_PIN);
    
    Serial2.setTimeout(1000);     // Shorter timeout for faster detection
    Serial2.setRxTimeout(100);    // T3.5 timeout for Modbus RTU (3.5 character times)
    
    mb.begin(&Serial2);
    mb.master();
}

void modbusLoop() {
  mb.task();
  yield();
}