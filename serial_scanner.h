
#include <HardwareSerial.h>
#define SERIAL_SCANNER 2
#define DATA_LEN  100


HardwareSerial SCANNER(2);
volatile uint8_t scanner_Flag = 0;
volatile uint8_t scan_data_ready = 0;

String temp_scan_data = "",scan_data;

typedef void (*CallbackFunction)(String);

CallbackFunction callbackFunctionPointer = nullptr; // Function pointer variable to store the callback address

// Function to register a callback
void scannerCallback(CallbackFunction callback) {
  // Store the address of the callback function
  callbackFunctionPointer = callback;
}


void scannerInit(uint32_t baudrate,uint8_t rx_pin,uint8_t tx_pin)
{
    SCANNER.begin(baudrate, SERIAL_8N1, rx_pin, tx_pin);//RX,TX
    scanner_Flag = SERIAL_SCANNER;
}

void scannerEvent() {
    
    while(SCANNER.available())
    {
        char inChar= (char)SCANNER.read();
        // Serial.print(inChar);

        if(inChar ==  0x0A)
        {
            scan_data = temp_scan_data;
            callbackFunctionPointer(scan_data);
            temp_scan_data = "";
            scan_data_ready = 1;
        }else{
          temp_scan_data += inChar;
        }
    }
    
}

void clearscanner()
{
    while(SCANNER.available())
    {
        char inChar= (char)SCANNER.read();
        Serial.print(inChar);
    }

}