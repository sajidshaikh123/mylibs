#ifndef DELAY_TIMER_H
#define DELAY_TIMER_H

// ==================== VERSION INFORMATION ====================
// Library: DelayTimer
// Version: 1.1.0 (MAJOR.MINOR.PATCH)
// Description: Non-blocking timer for Arduino/ESP32 with callback support
// Author: Your Name/Organization
// Last Updated: 2025-11-01
// Changelog:
//   v1.1.0 - Added callback function support
//   v1.0.0 - Initial release
// ============================================================

#define DELAY_TIMER_VERSION_MAJOR 1
#define DELAY_TIMER_VERSION_MINOR 1
#define DELAY_TIMER_VERSION_PATCH 0
#define DELAY_TIMER_VERSION "1.1.0"

// Version check macro
#define DELAY_TIMER_VERSION_CHECK(major, minor, patch) \
    ((DELAY_TIMER_VERSION_MAJOR > (major)) || \
     (DELAY_TIMER_VERSION_MAJOR == (major) && DELAY_TIMER_VERSION_MINOR > (minor)) || \
     (DELAY_TIMER_VERSION_MAJOR == (major) && DELAY_TIMER_VERSION_MINOR == (minor) && DELAY_TIMER_VERSION_PATCH >= (patch)))

#include <Arduino.h>

class DelayTimer {
  private:
    uint32_t interval = 0;
    uint32_t lastTrigger = 0;
    bool firstRun = true;
    void (*callback)() = nullptr;  // Function pointer for callback

  public:
    // Get library version
    static const char* getVersion() {
      return DELAY_TIMER_VERSION;
    }
    
    // Get version components
    static uint8_t getMajorVersion() { return DELAY_TIMER_VERSION_MAJOR; }
    static uint8_t getMinorVersion() { return DELAY_TIMER_VERSION_MINOR; }
    static uint8_t getPatchVersion() { return DELAY_TIMER_VERSION_PATCH; }
    
    DelayTimer(uint32_t timeMs) {
        interval = timeMs;
        lastTrigger = millis();
        firstRun = true;
        callback = nullptr;
    }

    void setTimer(uint32_t timeMs) {
      interval = timeMs;
      lastTrigger = millis();
      firstRun = true;
    }
    
    void resetTimer(){
      lastTrigger = millis();
    }
    
    // Set callback function to be called when timer triggers
    void setCallback(void (*callbackFunc)()) {
      callback = callbackFunc;
    }
    
    // Remove callback function
    void removeCallback() {
      callback = nullptr;
    }
    
    // Check if callback is set
    bool hasCallback() {
      return (callback != nullptr);
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
        
        // Call callback function if available
        if (callback != nullptr) {
          callback();
        }
        
        return true;
      }
      return false;
    }
    
    // Update timer and execute callback (if available) without returning status
    void update() {
      ontime();  // This will trigger callback automatically
    }
};

#endif
