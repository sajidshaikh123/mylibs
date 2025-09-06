// PCF8574_Output.cpp - Implementation file for PCF8574 Output Expander Library
#include "PCF8574_Output.h"

PCF8574_Output::PCF8574_Output(uint8_t address, uint8_t scl, uint8_t sda)
    : pcf2(address), sclPin(scl), sdaPin(sda) {}

PCF8574_Output::PCF8574_Output()
    : pcf2(OUTPUT_ADDR), sclPin(SCL_PIN),sdaPin(SDA_PIN) {}

bool PCF8574_Output::begin() {
    Wire.setPins(sdaPin,sclPin);
    Wire.begin(sdaPin, sclPin);
    if (pcf2.begin(0x00)) {
        Serial.println("OUTPUT INIT OK");
        status = true;
    } else {
        status = false;
        Serial.println("OUTPUT INIT ERROR");
    }
    return status;
}

void PCF8574_Output::writeOutput(uint8_t pin, uint8_t val) {
    if(status){
        pcf2.write(pin, val);
    }
}

int16_t PCF8574_Output::readOutputs() {
    if(status){
        return pcf2.valueOut();
    }else{
            return -1;
    }
}