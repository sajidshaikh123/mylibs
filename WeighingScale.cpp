#include "WeighingScale.h"

WeighingScale::WeighingScale(uint8_t dout, uint8_t sck, float calib)
  : doutPin(dout), sckPin(sck), calibrationFactor(calib), status(false) {}

bool WeighingScale::begin() {
  // pinMode(HX711_SCK, OUTPUT);
  // pinMode(HX711_DOUT, INPUT_PULLUP);
  // digitalWrite(HX711_SCK, LOW);
  // delay(1);

  // if (digitalRead(HX711_DOUT) == LOW) {
  //   Serial.println("HX711 detected.");

    scale.begin(doutPin, sckPin);
    scale.set_scale(calibrationFactor);
    scale.tare();
    status = true;  //  Mark as OK
    return true;
  // }

  Serial.println("HX711 not detected.");
  status = false;   //  Mark as not ready
  return false;
}

uint16_t WeighingScale::readWeight(bool applyTare) {
  if (!status) return 0.0f;  //  Don't proceed if HX711 not OK
  if (applyTare) scale.tare();
  scale.set_scale(calibrationFactor);
  return scale.get_units(10);
}
float WeighingScale::readraw() {
  
  return scale.get_value(10);
}

void WeighingScale::tare() {
  if (!status) return;
  scale.tare();
}

void WeighingScale::setCalibration(float factor) {
  calibrationFactor = factor;
  if (!status) return;
  scale.set_scale(calibrationFactor);
}

bool WeighingScale::isReady() {
  return status && scale.is_ready();
}

bool WeighingScale::isOK() {
  return status;
}
