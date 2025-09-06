// PCF8574_Input.h - Header file for PCF8574 Input Expander Library
#ifndef PCF8574_INPUT_H
#define PCF8574_INPUT_H

#include "pindefinition.h"
#include "Arduino.h"
#include "PCF8574.h"
#include <Wire.h>

#define INPUT_ADDR  0x25  // Address for the input expander
class PCF8574_Input {
public:
    PCF8574_Input(uint8_t address=INPUT_ADDR, uint8_t scl = SCL_PIN, uint8_t sda = SDA_PIN, uint8_t interruptPin = INPUT_INTERRUPT);
    PCF8574_Input();
    bool begin();
    int16_t readInputs();
    bool inputChanged();
    void attachInt(void (*isr)(), int mode);

private:
    PCF8574 pcf;
    uint8_t intPin;
    uint8_t sclPin;
    uint8_t sdaPin;
    bool status = false;
};

#endif