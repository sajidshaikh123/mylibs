
#include <dwindisplay.h>
// #include "dwin_res.h"

#define DWIN_READ 0X83



dwindisplay HMI;
uint8_t upload_flag = 0;


// uint16_t compound = 0x5510;
// uint16_t sr_no = 0x5500;

// uint16_t counter = 0x5100;
// uint16_t SP_compound = 0x8010;

//uint16_t COLOR = { 0x001F };

uint8_t dwin_input[20] = { 0 };
//uint8_t P_char = 0;
volatile uint8_t dwin_rec_flag = 0;

void dwinLoop() {
  while (1 == upload_flag) {
    yield(); // Feed watchdog during upload mode
    while (HMI.available()) {
      Serial.write(HMI.read());
      yield(); // Feed watchdog while reading from HMI
    }

    while (Serial.available()) {
      HMI.write(Serial.read());
      yield(); // Feed watchdog while writing to HMI
    }
  } 
  if(0 == upload_flag){
    while (HMI.available()) {
      char ch = HMI.read();
      //Serial.write(ch);
      if (ch == 0X5A) {
        yield(); // Feed watchdog instead of delay
        if (HMI.available()) ch = HMI.read();
        if (ch == 0XA5) {
          yield(); // Feed watchdog instead of delay
          if (HMI.available()) {
            uint8_t size = HMI.read();
            uint16_t index = 0;
            unsigned long timeout = millis();
            while (size-- && (millis() - timeout < 100)) { // Add timeout protection
              if (HMI.available()) {
                dwin_input[index++] = HMI.read();
                timeout = millis(); // Reset timeout on successful read
              }
              yield(); // Feed watchdog while reading
            }
            dwin_rec_flag = size;
          }
        }
      }
    }
  }

}
