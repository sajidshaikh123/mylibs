// PCF8574_Input.cpp - Implementation file for PCF8574 Input Expander Library
#include "PCF8574_Input.h"

PCF8574_Input::PCF8574_Input(uint8_t address, uint8_t scl, uint8_t sda, uint8_t interruptPin)
    : pcf(address), intPin(interruptPin), sclPin(scl), sdaPin(sda) {}

PCF8574_Input::PCF8574_Input()
    : pcf(INPUT_ADDR), intPin(INPUT_INTERRUPT), sclPin(SCL_PIN), sdaPin(SDA_PIN) {}


bool PCF8574_Input::begin() {
    Wire.setPins(sdaPin,sclPin);
    Wire.begin(sdaPin, sclPin);
    if (pcf.begin(0xFF)) {
        status = true;
    } else {
        status = false;
    }
    pinMode(intPin, INPUT_PULLUP);
    return status;
}

int16_t PCF8574_Input::readInputs() {
    if(status){
        return pcf.read8();
    }else{
            return -1;
    }
}

bool PCF8574_Input::inputChanged() {
    return digitalRead(intPin) == LOW;
}

void PCF8574_Input::attachInt(void (*isr)(), int mode) {
    attachInterrupt(digitalPinToInterrupt(intPin), isr, mode);
}