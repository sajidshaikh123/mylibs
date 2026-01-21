/**
 * @file RTCManager.cpp
 * @brief Implementation of RTCManager class
 * @date November 2, 2025
 */

#include "RTCManager.h"

/**
 * @brief Constructor
 */
RTCManager::RTCManager(int32_t timezoneOffset) 
    : internalRTC(timezoneOffset), 
      externalRTCAvailable(false),
      century(false),
      h12(false),
      pm(false),
      sclPin(SCL_PIN),
      sdaPin(SDA_PIN) {
    dateTimeBuffer.reserve(32); // Reserve space for date/time string
}

/**
 * @brief Initialize the RTC module
 */
bool RTCManager::begin(uint8_t sda, uint8_t scl) {
    sdaPin = sda;
    sclPin = scl;
    
    // Initialize I2C
    Wire.begin(sdaPin, sclPin);
    
    // Try to communicate with external RTC
    // Check if we can read a valid second value (0-59)
    uint8_t seconds = externalRTC.getSecond();
    
    if (seconds < 60) {
        externalRTCAvailable = true;
        Serial.println("[RTCManager] External DS3231 RTC initialized successfully");
        
        // Sync internal RTC with external RTC
        syncInternalWithExternal();
    } else {
        externalRTCAvailable = false;
        Serial.println("[RTCManager] External RTC not found, using internal ESP32 RTC");
    }
    
    return externalRTCAvailable;
}

/**
 * @brief Check if external RTC is available
 */
bool RTCManager::isExternalRTCAvailable() const {
    return externalRTCAvailable;
}

/**
 * @brief Format number with leading zero if needed
 */
String RTCManager::formatTwoDigits(uint8_t value) {
    if (value < 10) {
        return "0" + String(value);
    }
    return String(value);
}

/**
 * @brief Get formatted date and time as string
 */
const char* RTCManager::getDateTime() {
    dateTimeBuffer = "";
    
    if (externalRTCAvailable) {
        // Use external DS3231 RTC
        uint16_t year = externalRTC.getYear() + 2000;
        uint8_t month = externalRTC.getMonth(century);
        uint8_t day = externalRTC.getDate();
        uint8_t hour = externalRTC.getHour(h12, pm);
        uint8_t minute = externalRTC.getMinute();
        uint8_t second = externalRTC.getSecond();
        
        dateTimeBuffer = String(year) + "-" + 
                        formatTwoDigits(month) + "-" + 
                        formatTwoDigits(day) + " " + 
                        formatTwoDigits(hour) + ":" + 
                        formatTwoDigits(minute) + ":" + 
                        formatTwoDigits(second);
    } else {
        // Use internal ESP32 RTC
        uint16_t year = internalRTC.getYear();
        uint8_t month = internalRTC.getMonth() + 1; // ESP32Time returns 0-11
        uint8_t day = internalRTC.getDay();
        uint8_t hour = internalRTC.getHour(true); // true for 24-hour format
        uint8_t minute = internalRTC.getMinute();
        uint8_t second = internalRTC.getSecond();
        
        dateTimeBuffer = String(year) + "-" + 
                        formatTwoDigits(month) + "-" + 
                        formatTwoDigits(day) + " " + 
                        formatTwoDigits(hour) + ":" + 
                        formatTwoDigits(minute) + ":" + 
                        formatTwoDigits(second);
    }
    
    return dateTimeBuffer.c_str();
}

/**
 * @brief Set the RTC date and time
 */
void RTCManager::setDateTime(uint8_t day, uint8_t month, uint16_t year, 
                             uint8_t hour, uint8_t minute, uint8_t second) {
    if (externalRTCAvailable) {
        // Set external DS3231 RTC
        externalRTC.setClockMode(false); // 24-hour mode
        
        uint8_t yearForDS3231 = year;
        if (year > 2000) {
            yearForDS3231 = year - 2000;
        }
        
        externalRTC.setYear(yearForDS3231);
        externalRTC.setMonth(month);
        externalRTC.setDate(day);
        externalRTC.setHour(hour);
        externalRTC.setMinute(minute);
        externalRTC.setSecond(second);
        
        // Sync internal RTC with external
        syncInternalWithExternal();
        
        Serial.println("[RTCManager] External RTC time set");
    } else {
        // Set internal ESP32 RTC
        internalRTC.setTime(second, minute, hour, day, month, year);
        Serial.println("[RTCManager] Internal RTC time set");
    }
}

/**
 * @brief Get current second
 */
uint8_t RTCManager::getSecond() {
    if (externalRTCAvailable) {
        return externalRTC.getSecond();
    } else {
        return internalRTC.getSecond();
    }
}


/**
 * @brief Get current minute
 */
uint8_t RTCManager::getMinute() {
    if (externalRTCAvailable) {
        return externalRTC.getMinute();
    } else {
        return internalRTC.getMinute();
    }
}

/**
 * @brief Get current hour
 */
uint8_t RTCManager::getHour() {
    if (externalRTCAvailable) {
        return externalRTC.getHour(h12, pm);
    } else {
        return internalRTC.getHour(true); // true for 24-hour format
    }
}

/**
 * @brief Get current day of month
 */
uint8_t RTCManager::getDay() {
    if (externalRTCAvailable) {
        return externalRTC.getDate();
    } else {
        return internalRTC.getDay();
    }
}

/**
 * @brief Get current month
 */
uint8_t RTCManager::getMonth() {
    if (externalRTCAvailable) {
        return externalRTC.getMonth(century);
    } else {
        return internalRTC.getMonth() + 1; // ESP32Time returns 0-11
    }
}

/**
 * @brief Get current year
 */
uint16_t RTCManager::getYear() {
    if (externalRTCAvailable) {
        return externalRTC.getYear() + 2000;
    } else {
        return internalRTC.getYear();
    }
}

/**
 * @brief Get day of week
 */
uint8_t RTCManager::getDayOfWeek() {
    if (externalRTCAvailable) {
        return externalRTC.getDoW();
    } else {
        return internalRTC.getDayofWeek();
    }
}

/**
 * @brief Get temperature from DS3231
 */
float RTCManager::getTemperature() {
    if (externalRTCAvailable) {
        return externalRTC.getTemperature();
    } else {
        return NAN; // Not available on internal RTC
    }
}

/**
 * @brief Get Unix timestamp
 */
unsigned long RTCManager::getEpoch() {
    
    return internalRTC.getEpoch();
}

/**
 * @brief Set time from Unix timestamp
 */
void RTCManager::setEpoch(unsigned long epoch) {
    if (externalRTCAvailable) {
        // Convert epoch to date/time and set
        // This requires date/time conversion - implement if needed
        internalRTC.setTime(epoch);
        syncExternalWithInternal();
    } else {
        internalRTC.setTime(epoch);
    }
}

/**
 * @brief Print current date and time to Serial
 */
void RTCManager::printDateTime() {
    Serial.print("[RTCManager] Current time: ");
    Serial.print(getDateTime());
    Serial.print(" (Source: ");
    Serial.print(externalRTCAvailable ? "External DS3231" : "Internal ESP32");
    Serial.println(")");
}

/**
 * @brief Sync internal RTC with external RTC
 */
bool RTCManager::syncInternalWithExternal() {
    if (!externalRTCAvailable) {
        return false;
    }
    
    uint16_t year = externalRTC.getYear() + 2000;
    uint8_t month = externalRTC.getMonth(century);
    uint8_t day = externalRTC.getDate();
    uint8_t hour = externalRTC.getHour(h12, pm);
    uint8_t minute = externalRTC.getMinute();
    uint8_t second = externalRTC.getSecond();
    
    internalRTC.setTime(second, minute, hour, day, month, year);
    
    Serial.println("[RTCManager] Internal RTC synchronized with external RTC");
    return true;
}

/**
 * @brief Sync external RTC with internal RTC
 */
bool RTCManager::syncExternalWithInternal() {
    if (!externalRTCAvailable) {
        return false;
    }
    
    uint16_t year = internalRTC.getYear();
    uint8_t month = internalRTC.getMonth() + 1;
    uint8_t day = internalRTC.getDay();
    uint8_t hour = internalRTC.getHour(true);
    uint8_t minute = internalRTC.getMinute();
    uint8_t second = internalRTC.getSecond();
    
    uint8_t yearForDS3231 = year;
    if (year > 2000) {
        yearForDS3231 = year - 2000;
    }
    
    externalRTC.setClockMode(false);
    externalRTC.setYear(yearForDS3231);
    externalRTC.setMonth(month);
    externalRTC.setDate(day);
    externalRTC.setHour(hour);
    externalRTC.setMinute(minute);
    externalRTC.setSecond(second);
    
    Serial.println("[RTCManager] External RTC synchronized with internal RTC");
    return true;
}
