#include "HardwareSerial.h"
#include <ModbusRTU.h>
#include "pindefinition.h"

HardwareSerial RTUSerial(2); // Use UART2 for RS485 Modbus

ModbusRTU mb;
static bool modbusInitialized = false; // Guard to prevent double initialization

void modbusInit(){
    if (modbusInitialized) {
        Serial.println("[MODBUS] Already initialized, skipping");
        return;
    }
    
    // Serial.println("[MODBUS] Skipped - disabled due to FreeRTOS queue assertion");
    // Serial.println("[MODBUS] To enable: Fix ModbusRTU library FreeRTOS compatibility");
    // The ModbusRTU library causes: assert failed: xQueueSemaphoreTake queue.c:1713
    // This happens because mb.begin() or mb.master() creates FreeRTOS objects
    // that conflict with the current task/queue state during initialization
    
    // COMMENTED OUT TO PREVENT CRASH:
    Serial.println("[MODBUS] Initializing Modbus RTU on RS485...");
    RTUSerial.begin(9600,SERIAL_8N1,RX2_PIN,TX2_PIN);
    Serial.println("[MODBUS] Serial port initialized");
    RTUSerial.setTimeout(500);
    Serial.println("[MODBUS] Serial timeout set");
    RTUSerial.setRxTimeout(100);
    Serial.println("[MODBUS] Serial RX timeout set");
    mb.begin(&RTUSerial);
    Serial.println("[MODBUS] Modbus RTU begun");
    mb.master();
    Serial.println("[MODBUS] Set as Modbus Master");
    
    modbusInitialized = true; // Don't set to true since we didn't actually init
    Serial.println("[MODBUS] Initialization complete");
}

void modbusLoop() {
  if (!modbusInitialized) {
    return; // Don't run if not initialized
  }
  mb.task();
  yield();
}