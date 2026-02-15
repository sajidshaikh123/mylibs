// PCF8574_Input.cpp - Implementation file for PCF8574 Input Expander Library
#include "PCF8574_Input.h"

PCF8574_Input::PCF8574_Input(uint8_t address, uint8_t scl, uint8_t sda, uint8_t interruptPin)
    : pcf(address), intPin(interruptPin), sclPin(scl), sdaPin(sda) {}

PCF8574_Input::PCF8574_Input()
    : pcf(INPUT_ADDR), intPin(INPUT_INTERRUPT), sclPin(SCL_PIN), sdaPin(SDA_PIN) {}


bool PCF8574_Input::begin() {
    Wire.setPins(sdaPin,sclPin);
    Wire.begin(sdaPin, sclPin);
    Wire.setTimeOut(50); // Set 50ms I2C timeout to prevent blocking
    if (pcf.begin(0xFF)) {
        status = true;
    } else {
        status = false;
    }
    pinMode(intPin, INPUT_PULLUP);
    return status;
}

int16_t PCF8574_Input::readInputs() {
    if(!status){
        return -1;
    }
    
    yield(); // Feed watchdog before I2C operation
    
    // Add timeout protection
    unsigned long startTime = millis();
    int16_t result = -1;
    
    // Try to read with timeout check
    Wire.beginTransmission(INPUT_ADDR);
    if (Wire.endTransmission() == 0) {
        // Device responded, do the actual read
        result = pcf.read8();
    } else {
        // Device not responding
        Serial.println("[PCF8574] Device not responding");
    }
    
    // Check if operation took too long
    if (millis() - startTime > 100) {
        Serial.println("[PCF8574] Read timeout");
        result = -1;
    }
    
    yield(); // Feed watchdog after I2C operation
    return result;
}

bool PCF8574_Input::inputChanged() {
    return digitalRead(intPin) == LOW;
}

void PCF8574_Input::attachInt(void (*isr)(), int mode) {
    attachInterrupt(digitalPinToInterrupt(intPin), isr, mode);
}