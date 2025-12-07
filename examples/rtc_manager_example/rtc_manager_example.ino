/**
 * @file rtc_manager_example.ino
 * @brief Example sketch demonstrating RTCManager class usage
 * @date November 2, 2025
 * 
 * This example shows how to use the RTCManager class to:
 * - Initialize RTC (external DS3231 or internal ESP32)
 * - Read date and time
 * - Set date and time
 * - Get individual time components
 * - Sync between external and internal RTC
 */

#include <RTCManager.h>

// Create RTCManager instance with timezone offset (0 for UTC)
// For other timezones, use seconds offset: e.g., 19800 for IST (UTC+5:30)
RTCManager rtc(0);

// SDA and SCL pins are automatically loaded from pindefinition.h
// based on your ESP32 variant (ESP32, ESP32-S3, or ESP32-C6)

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== RTCManager Example ===\n");
    
    // Initialize RTC (pins from pindefinition.h)
    Serial.println("Initializing RTC...");
    bool externalRTCFound = rtc.begin(); // Uses default pins from pindefinition.h
    // Or specify custom pins: rtc.begin(21, 22);
    
    if (externalRTCFound) {
        Serial.println("✓ External DS3231 RTC is active");
    } else {
        Serial.println("⚠ Using internal ESP32 RTC (fallback mode)");
    }
    
    Serial.println();
    
    // Print current date and time
    rtc.printDateTime();
    
    // Uncomment to set RTC time (adjust values as needed)
    // Format: setDateTime(day, month, year, hour, minute, second)
    // rtc.setDateTime(2, 11, 2025, 14, 30, 0);
    // Serial.println("Time set to: 2025-11-02 14:30:00");
    
    Serial.println("\n=== Starting continuous time display ===\n");
}

void loop() {
    // Display full date and time
    Serial.print("DateTime: ");
    Serial.println(rtc.getDateTime());
    
    // Display individual components
    Serial.print("Individual values -> ");
    Serial.print("Year: ");
    Serial.print(rtc.getYear());
    Serial.print(", Month: ");
    Serial.print(rtc.getMonth());
    Serial.print(", Day: ");
    Serial.print(rtc.getDay());
    Serial.print(", Hour: ");
    Serial.print(rtc.getHour());
    Serial.print(", Minute: ");
    Serial.print(rtc.getMinute());
    Serial.print(", Second: ");
    Serial.println(rtc.getSecond());
    
    // Get day of week
    Serial.print("Day of Week: ");
    Serial.print(rtc.getDayOfWeek());
    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    Serial.print(" (");
    Serial.print(days[rtc.getDayOfWeek()]);
    Serial.println(")");
    
    // If external RTC is available, display temperature
    if (rtc.isExternalRTCAvailable()) {
        float temp = rtc.getTemperature();
        Serial.print("DS3231 Temperature: ");
        Serial.print(temp);
        Serial.println(" °C");
    }
    
    Serial.println("---");
    
    delay(5000); // Update every 5 seconds
}
