/**
 * @file RTCManager.h
 * @brief Class-based RTC management for DS3231 and ESP32 internal RTC
 * @date November 2, 2025
 * 
 * This class provides a unified interface to manage both external DS3231 RTC
 * and ESP32's internal RTC, with automatic fallback to internal RTC if external
 * RTC is not available.
 */

#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <DS3231.h>
#include <ESP32Time.h>
#include "pindefinition.h"

/**
 * @class RTCManager
 * @brief Manages RTC operations with automatic fallback between external and internal RTC
 */
class RTCManager {
private:
    DS3231 externalRTC;           // External DS3231 RTC module
    ESP32Time internalRTC;        // ESP32 internal RTC
    
    bool externalRTCAvailable;    // Flag to indicate if external RTC is working
    bool century;                 // Century flag for DS3231
    bool h12;                     // 12-hour format flag
    bool pm;                      // PM flag for 12-hour format
    
    uint8_t sclPin;               // I2C SCL pin
    uint8_t sdaPin;               // I2C SDA pin
    
    String dateTimeBuffer;        // Buffer for date/time string formatting

public:
    /**
     * @brief Constructor
     * @param timezoneOffset Timezone offset in seconds (default: 0 for UTC)
     */
    RTCManager(int32_t timezoneOffset = 0);
    
    /**
     * @brief Initialize the RTC module
     * @param sda I2C SDA pin number (default from pindefinition.h)
     * @param scl I2C SCL pin number (default from pindefinition.h)
     * @return true if external RTC is available, false if using internal RTC fallback
     */
    bool begin(uint8_t sda = SDA_PIN, uint8_t scl = SCL_PIN);
    
    /**
     * @brief Check if external RTC is available and working
     * @return true if external RTC is available, false otherwise
     */
    bool isExternalRTCAvailable() const;
    
    /**
     * @brief Get formatted date and time as string
     * @return const char* Date and time in format "YYYY-MM-DD HH:MM:SS"
     */
    const char* getDateTime();
    
    /**
     * @brief Set the RTC date and time
     * @param day Day of month (1-31)
     * @param month Month (1-12)
     * @param year Full year (e.g., 2025)
     * @param hour Hour (0-23)
     * @param minute Minute (0-59)
     * @param second Second (0-59)
     */
    void setDateTime(uint8_t day, uint8_t month, uint16_t year, 
                     uint8_t hour, uint8_t minute, uint8_t second);
    
    /**
     * @brief Get current second
     * @return uint8_t Current second (0-59)
     */
    uint8_t getSecond();
    
    /**
     * @brief Get current minute
     * @return uint8_t Current minute (0-59)
     */
    uint8_t getMinute();
    
    /**
     * @brief Get current hour
     * @return uint8_t Current hour (0-23)
     */
    uint8_t getHour();
    
    /**
     * @brief Get current day of month
     * @return uint8_t Current day (1-31)
     */
    uint8_t getDay();
    
    /**
     * @brief Get current month
     * @return uint8_t Current month (1-12)
     */
    uint8_t getMonth();
    
    /**
     * @brief Get current year
     * @return uint16_t Current year (e.g., 2025)
     */
    uint16_t getYear();
    
    /**
     * @brief Get day of week
     * @return uint8_t Day of week (0-6, where 0 = Sunday)
     */
    uint8_t getDayOfWeek();
    
    /**
     * @brief Get temperature from DS3231 (if available)
     * @return float Temperature in Celsius, or NAN if not available
     */
    float getTemperature();
    
    /**
     * @brief Get Unix timestamp (epoch time)
     * @return unsigned long Unix timestamp
     */
    unsigned long getEpoch();
    
    /**
     * @brief Set time from Unix timestamp
     * @param epoch Unix timestamp
     */
    void setEpoch(unsigned long epoch);
    
    /**
     * @brief Print current date and time to Serial
     */
    void printDateTime();
    
    /**
     * @brief Sync internal RTC with external RTC
     * @return true if sync successful, false otherwise
     */
    bool syncInternalWithExternal();
    
    /**
     * @brief Sync external RTC with internal RTC
     * @return true if sync successful, false otherwise
     */
    bool syncExternalWithInternal();

private:
    /**
     * @brief Format number with leading zero if needed
     * @param value Number to format
     * @return String Formatted number
     */
    String formatTwoDigits(uint8_t value);
};

#endif // RTC_MANAGER_H
