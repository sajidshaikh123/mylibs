
#include <dwindisplay.h>
#include "dwin_res.h"

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
    while (HMI.available()) {
      Serial.write(HMI.read());
    }

    while (Serial.available()) {
      HMI.write(Serial.read());
    }
  } 
  if(0 == upload_flag){
    while (HMI.available()) {
      char ch = HMI.read();
      //Serial.write(ch);
      if (ch == 0X5A) {
        delay(1);
        ch = HMI.read();
        if (ch == 0XA5) {
          delay(1);
          uint8_t size = HMI.read();
          uint16_t index = 0;
          while (size--) {
            dwin_input[index++] = HMI.read();
          }
          dwin_rec_flag = size;
        }
      }
    }
  }

}
