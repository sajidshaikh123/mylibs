// PCF8574_Output.h - Header file for PCF8574 Output Expander Library

#ifndef PCF8574_OUTPUT_H
#define PCF8574_OUTPUT_H
#include "pindefinition.h"
#include "Arduino.h"
#include "PCF8574.h"
#include <Wire.h>

#define OUTPUT_ADDR 0x26  // Address for the output expander

class PCF8574_Output {
public:
    PCF8574_Output(uint8_t address=OUTPUT_ADDR, uint8_t scl = SCL_PIN, uint8_t sda = SDA_PIN);
    PCF8574_Output();
    bool begin();
    void writeOutput(uint8_t pin, uint8_t val);
    int16_t readOutputs();

private:
    PCF8574 pcf2;
    uint8_t sclPin;
    uint8_t sdaPin;
    bool status = false;
};

#endif