/*
 * IOT Board Version Management Example
 * 
 * This example demonstrates how to use version management features
 * in your IOT Board library
 */

#include <iotboard.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n========================================");
    Serial.println("IOT Board Version Management Demo");
    Serial.println("========================================\n");
    
    // Method 1: Use the built-in printVersionInfo() function
    IOTBoard::printVersionInfo();
    
    Serial.println("\n--- Individual Version Access ---");
    
    // Method 2: Access individual version components
    Serial.print("Library Version: ");
    Serial.println(IOTBoard::getLibraryVersion());
    
    Serial.print("Firmware Version: ");
    Serial.println(IOTBoard::getFirmwareVersion());
    
    Serial.print("Board Model: ");
    Serial.println(IOTBoard::getBoardModel());
    
    Serial.print("Hardware Version: ");
    Serial.println(IOTBoard::getHardwareVersion());
    
    // Method 3: Access version macros directly
    Serial.println("\n--- Version Macros ---");
    Serial.print("Major: "); Serial.println(IOTBOARD_VERSION_MAJOR);
    Serial.print("Minor: "); Serial.println(IOTBOARD_VERSION_MINOR);
    Serial.print("Patch: "); Serial.println(IOTBOARD_VERSION_PATCH);
    
    // Method 4: Conditional compilation based on version
    #if IOTBOARD_VERSION_CHECK(1, 0, 0)
        Serial.println("\n✓ Library version is 1.0.0 or higher");
    #else
        Serial.println("\n✗ Library version is below 1.0.0");
    #endif
    
    // Example: Check if specific features are available
    checkFeatureAvailability();
    
    // Example: Version-specific code execution
    versionSpecificCode();
}

void loop() {
    // Your main code here
    delay(5000);
}

// Function to check feature availability based on version
void checkFeatureAvailability() {
    Serial.println("\n--- Feature Availability ---");
    
    #if IOTBOARD_VERSION_CHECK(1, 0, 0)
        Serial.println("✓ Basic Features Available");
    #endif
    
    #if IOTBOARD_VERSION_CHECK(1, 1, 0)
        Serial.println("✓ Advanced Features Available");
    #else
        Serial.println("✗ Advanced Features Not Available (requires v1.1.0+)");
    #endif
    
    #if IOTBOARD_VERSION_CHECK(2, 0, 0)
        Serial.println("✓ Next-Gen Features Available");
    #else
        Serial.println("✗ Next-Gen Features Not Available (requires v2.0.0+)");
    #endif
}

// Example of version-specific code execution
void versionSpecificCode() {
    Serial.println("\n--- Version-Specific Code Execution ---");
    
    #if IOTBOARD_VERSION_MAJOR >= 2
        // Code for version 2.x.x and above
        Serial.println("Running Version 2.x code");
        // newFeatureFunction();
    #elif IOTBOARD_VERSION_MAJOR >= 1 && IOTBOARD_VERSION_MINOR >= 5
        // Code for version 1.5.x and above
        Serial.println("Running Version 1.5.x code");
        // improvedFeatureFunction();
    #else
        // Code for earlier versions
        Serial.println("Running Version 1.0.x code");
        // legacyFeatureFunction();
    #endif
}
