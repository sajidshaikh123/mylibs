#include "PCF8574_Output.h"
#include "PCF8574_Input.h"

#define OUTPUT_ADDR 0x26  // Address for the output expander
#define INPUT_ADDR  0x25  // Address for the input expander
#define SCL_PIN     22    // SCL pin
#define SDA_PIN     21    // SDA pin
#define INT_PIN     35    // Interrupt pin for input

PCF8574_Output outputExpander(OUTPUT_ADDR, SCL_PIN, SDA_PIN);
PCF8574_Input inputExpander(INPUT_ADDR, SCL_PIN, SDA_PIN, INT_PIN);

void inputISR() {
    Serial.println("Interrupt detected! Input state changed.");
}

void setup() {
    Serial.begin(115200);
    
    outputExpander.begin();
    inputExpander.begin();

    // Attach an interrupt for input changes
    inputExpander.attachInterrupt(inputISR, FALLING);
}

void loop() {
    // Example: Toggle an output pin
    outputExpander.writeOutput(0, HIGH);
    delay(1000);
    outputExpander.writeOutput(0, LOW);
    delay(1000);

    // Example: Read inputs
    uint8_t inputs = inputExpander.readInputs();
    Serial.print("Input State: ");
    Serial.println(inputs, BIN);
    
    delay(500);
}
