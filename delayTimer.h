#ifndef DELAY_TIMER_H
#define DELAY_TIMER_H

#include <Arduino.h>

class DelayTimer {
  private:
    uint32_t interval = 0;
    uint32_t lastTrigger = 0;
    bool firstRun = true;

  public:
    DelayTimer(uint32_t timeMs) {
        interval = timeMs;
        lastTrigger = millis();
        firstRun = true;
    }

    void setTimer(uint32_t timeMs) {
      interval = timeMs;
      lastTrigger = millis();
      firstRun = true;
    }
    void resetTimer(){
      lastTrigger = millis();
    }

    bool ontime() {
      uint32_t now = millis();

      // First call always returns false to let delay pass
      if (firstRun) {
        firstRun = false;
        return false;
      }

      if ((uint32_t)(now - lastTrigger) >= interval) {
        lastTrigger = now;  // auto-reset
        return true;
      }
      return false;
    }
};

#endif
