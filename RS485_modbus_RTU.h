#include "HardwareSerial.h"
#include <ModbusRTU.h>
#include "pindefinition.h"
#include <Preferences.h>

HardwareSerial RTUSerial(2); // Use UART2 for RS485 Modbus

ModbusRTU mb;
static bool modbusInitialized = false; // Guard to prevent double initialization

void modbusInit(){
    if (modbusInitialized) {
        Serial.println("[MODBUS] Already initialized, skipping");
        return;
    }

    Preferences rs485ModbusPref;
    
    // Serial.println("[MODBUS] Skipped - disabled due to FreeRTOS queue assertion");
    // Serial.println("[MODBUS] To enable: Fix ModbusRTU library FreeRTOS compatibility");
    // The ModbusRTU library causes: assert failed: xQueueSemaphoreTake queue.c:1713
    // This happens because mb.begin() or mb.master() creates FreeRTOS objects
    // that conflict with the current task/queue state during initialization
    
    // COMMENTED OUT TO PREVENT CRASH:
    Serial.println("[MODBUS] Initializing Modbus RTU on RS485...");
    rs485ModbusPref.begin("rs485modbus", true);
    uint32_t baud = rs485ModbusPref.getULong("baudrate", 9600);
    uint32_t serialCfg = rs485ModbusPref.getULong("config", SERIAL_8N1);
    rs485ModbusPref.end();
    RTUSerial.begin(baud, serialCfg, RX2_PIN, TX2_PIN);
    RTUSerial.setTimeout(500);
    RTUSerial.setRxTimeout(100);
    mb.begin(&RTUSerial);
    mb.master();
    
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