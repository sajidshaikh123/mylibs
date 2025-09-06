#ifndef WEIGHING_SCALE_H
#define WEIGHING_SCALE_H

#include <Arduino.h>
#include <HX711.h>
#include "pindefinition.h"

class WeighingScale {
  private:
    HX711 scale;
    uint8_t doutPin;
    uint8_t sckPin;
    float calibrationFactor;
    bool status;  // 🔹 New: internal status flag

  public:
    WeighingScale(uint8_t dout = HX711_DOUT, uint8_t sck = HX711_SCK, float calib = 1.0f);

    bool begin();
    uint16_t readWeight(bool applyTare = false);
    float readraw();
    void tare();
    void setCalibration(float factor);
    bool isReady();
    bool isOK();  // 🔹 New: public getter for status
};

#endif  // WEIGHING_SCALE_H
