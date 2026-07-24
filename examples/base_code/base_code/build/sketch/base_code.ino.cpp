#include <Arduino.h>
#line 1 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\base_code\\base_code\\base_code.ino"
#include "iotboard.h"

bool inputEvent = false;

#line 9 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\base_code\\base_code\\base_code.ino"
void setup();
#line 13 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\base_code\\base_code\\base_code.ino"
void loop();
#line 5 "C:\\Users\\hinab\\OneDrive\\Documents\\Arduino\\libraries\\mylibs\\examples\\base_code\\base_code\\base_code.ino"
void IRAM_ATTR inputISR() {
    inputEvent = true;
}

void setup() {
    boardinit();
}

void loop() {
    boardloop();
}

