#include "iotboard.h"

bool inputEvent = false;

void IRAM_ATTR inputISR() {
    inputEvent = true;
}

void setup() {
    boardinit();
}

void loop() {
    boardloop();
}
