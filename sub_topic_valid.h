 
 /*********************************  SUBTOPIC VALIDATION  ******************************** */
bool validateAndProcessSubtopic(const String& message);
bool validateSubtopicData(const DynamicJsonDocument& subtopicData);
void publishSubtopicStatus(const String& status, const String& message, const String& details);
bool isSubtopicDataChanged(const String& newData);
void logSubtopicChange(const DynamicJsonDocument& oldData, const DynamicJsonDocument& newData);
void testSubtopicValidation();
bool isValidSubtopicCharacter(char c);
bool isValidSubtopicString(const String& str);



// Publish subtopic status
void publishSubtopicStatus(const String& status, const String& message, const String& details) {
    if (mqtt_obj.connectionStatus() != MQTT_CONNECTED) return;
    
    DynamicJsonDocument statusDoc(400);
    statusDoc["status"] = status;
    statusDoc["message"] = message;
    statusDoc["details"] = details;
    statusDoc["timestamp"] = rtc.getDateTime();
    
    if (status == "SUCCESS") {
        statusDoc["current_subtopic"] = subtopic;
    }
    
    String payload;
    serializeJson(statusDoc, payload);
    
    // Publish to status topic using current MAC address
    String statusTopic = mqtt_obj.getMacTopic("/subtopic/status");
    mqtt_obj.publish(statusTopic.c_str(), payload.c_str());
    
    Serial.printf("Published subtopic status: %s\n", status.c_str());
}


// Validate topic string characters
bool isValidTopicString(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (!((c >= 'a' && c <= 'z') || 
              (c >= 'A' && c <= 'Z') || 
              (c >= '0' && c <= '9') || 
              c == '-' || c == '_')) {
            return false;
        }
    }
    return true;
}

// Additional content validation
bool validateSubtopicContent(const DynamicJsonDocument& doc) {
    // Validate company name
    const char* company = doc["company_name"] | "";
    if (strlen(company) < 3) {
        Serial.println("Company name too short (min 3 characters)");
        publishSubtopicStatus("ERROR", "Company name too short", "minimum 3 characters");
        return false;
    }
    
    // Validate location
    const char* location = doc["location"] | "";
    if (strlen(location) < 2) {
        Serial.println("Location too short (min 2 characters)");
        publishSubtopicStatus("ERROR", "Location too short", "minimum 2 characters");
        return false;
    }
    
    // Validate department
    const char* department = doc["department"] | "";
    const char* validDepartments[] = {"molding", "assembly", "packaging", "quality", "maintenance", "production"};
    bool validDept = false;
    for (int i = 0; i < 6; i++) {
        if (String(department).equalsIgnoreCase(validDepartments[i])) {
            validDept = true;
            break;
        }
    }
    if (!validDept) {
        Serial.printf("Invalid department: %s\n", department);
        publishSubtopicStatus("ERROR", "Invalid department", "Must be: molding, assembly, packaging, quality, maintenance, or production");
        return false;
    }
    
    // Validate machine name format (M-XX or similar)
    const char* machinename = doc["machinename"] | "";
    if (strlen(machinename) < 2) {
        Serial.println("Machine name too short");
        publishSubtopicStatus("ERROR", "Machine name too short", "minimum 2 characters");
        return false;
    }
    
    Serial.println("All content validation passed");
    return true;
}



// Log subtopic changes
// void logSubtopicChange(const DynamicJsonDocument& oldSub, const DynamicJsonDocument& newSub) {
//     if (mqtt_obj.connectionStatus() != MQTT_CONNECTED) return;
    
//     DynamicJsonDocument changeLog(800);
//     changeLog["event"] = "subtopic_changed";
//     changeLog["timestamp"] = rtc.getDateTime();
//     changeLog["old_subtopic"] = oldSub;
//     changeLog["new_subtopic"] = newSub;
    
//     // Generate old and new topic examples
//     mqtt_obj.setsubtopic(oldSub);
//     changeLog["old_topic_example"] = mqtt_obj.getTopic("example");
    
//     mqtt_obj.setsubtopic(newSub);
//     changeLog["new_topic_example"] = mqtt_obj.getTopic("example");
    
//     String payload;
//     serializeJson(changeLog, payload);

//     String logTopic = mqtt_obj.getMacTopic("/subtopic/change_log");
//     mqtt_obj.publish(logTopic.c_str(), payload.c_str());
    
//     Serial.println("Subtopic change logged");
// }


// Add this helper function
String readFileString(fs::FS &fs, const String& path) {
    File file = fs.open(path, "r");
    if (!file) {
        return "";
    }
    String content = file.readString();
    file.close();
    return content;
}


// Write subtopic to file with error handling
bool writeSubtopicToFile(const String& message) {
    Serial.println("Writing subtopic to file...");
    
    // Create backup of existing file
    if (fsManagerFFat.search("/subtopic")) {
        if (fsManagerFFat.search("/subtopic.bak")) {
            fsManagerFFat.search("/subtopic.bak");
        }
        if (!fsManagerFFat.renameFile("/subtopic", "/subtopic.bak")) {
            Serial.println("Warning: Failed to create backup");
        }
    }
    
    // Write new subtopic
    bool writeSuccess = fsManagerFFat.writeFile("/subtopic", message.c_str());
    
    if (!writeSuccess) {
        Serial.println("Failed to write subtopic file");
        
        // Restore backup if write failed
        if (fsManagerFFat.search("/subtopic.bak")) {
            if (fsManagerFFat.renameFile("/subtopic.bak", "/subtopic")) {
                Serial.println("Restored backup file");
            }
        }
        publishSubtopicStatus("ERROR", "File write failed", "Could not save subtopic");
        return false;
    }
    
    // Verify file was written correctly
    String readBack = fsManagerFFat.readFile("/subtopic");
    DynamicJsonDocument verifyDoc(1000);
    DeserializationError error = deserializeJson(verifyDoc, readBack);
    
    if (error) {
        Serial.println("File verification failed - corrupted write");
        // Restore backup
        if (fsManagerFFat.search("/subtopic.bak")) {
            fsManagerFFat.deleteFile("/subtopic");
            fsManagerFFat.renameFile("/subtopic.bak", "/subtopic");
        }
        publishSubtopicStatus("ERROR", "File verification failed", "Corrupted write detected");
        return false;
    }
    
    // Clean up backup on successful write
    if (fsManagerFFat.search("/subtopic.bak")) {
        fsManagerFFat.deleteFile("/subtopic.bak");
    }
    
    Serial.println("Subtopic file written and verified successfully");
    return true;
}

// Add this function to validate and process subtopic JSON
// Main validation and processing function for subtopic
bool validateAndProcessSubtopic(const String& message) {
    Serial.println("=== SUBTOPIC VALIDATION AND PROCESSING ===");
    
    // Parse JSON first
    DynamicJsonDocument tempSubtopic(1000);
    DeserializationError error = deserializeJson(tempSubtopic, message);
    if (error) {
        Serial.print(F("deserializeJson() failed -subtopic : "));
        Serial.println(error.f_str());
        publishSubtopicStatus("ERROR", "Invalid JSON format", error.f_str());
        return false;
    }
    
    Serial.println("JSON parsing successful, validating subtopic data...");
    serializeJsonPretty(tempSubtopic, Serial);
    
    // Validate required fields and content
    if (!validateSubtopicData(tempSubtopic)) {
        Serial.println("[SUBTOPIC] Validation failed - invalid data structure or content");
        publishSubtopicStatus("ERROR", "Validation failed", "Check subtopic data structure and content");
        return false;
    }
    
    // Check if data has actually changed
    if (!isSubtopicDataChanged(message)) {
        Serial.println("[SUBTOPIC] No changes detected - data is identical to saved version");
        publishSubtopicStatus("INFO", "No changes detected", "Data is identical to current configuration");
        return false; // Return false as no update was needed
    }
    
    Serial.println("[SUBTOPIC] Changes detected - updating subtopic data");
    
    // Backup current data before updating
    DynamicJsonDocument backupSubtopic(1000);
    backupSubtopic = subtopic;
    
    // Update subtopic data
    subtopic.clear();
    subtopic = tempSubtopic;
    
    // Update MQTT object with new subtopic
    mqtt_obj.setsubtopic(subtopic);
    
    // Write to file
    if (fsManagerFFat.writeFile("/subtopic", message.c_str())) {
        // Publish success status
        publishSubtopicStatus("SUCCESS", "Subtopic configuration updated", "Configuration changed and saved");
        
        // Log the changes
        logSubtopicChange(backupSubtopic, subtopic);
        
        Serial.println("[SUBTOPIC] Subtopic configuration updated successfully");
        return true;
        
    } else {
        // Revert changes if file write failed
        subtopic = backupSubtopic;
        mqtt_obj.setsubtopic(subtopic); // Restore old subtopic
        publishSubtopicStatus("ERROR", "File write failed", "Could not save configuration");
        return false;
    }
}

// Function to validate subtopic configuration required fields and content
bool validateSubtopicData(const DynamicJsonDocument& subtopicData) {
    Serial.println("=== CHECKING SUBTOPIC REQUIRED FIELDS ===");
    
    // Define required fields for subtopic configuration
    const char* requiredFields[] = {
        "company_name",
        "location",
        "department",
        "machinename"
    };
    
    const int fieldCount = sizeof(requiredFields) / sizeof(requiredFields[0]);
    bool allFieldsPresent = true;
    
    // Check each required field
    for (int i = 0; i < fieldCount; i++) {
        if (subtopicData.containsKey(requiredFields[i])) {
            // Check if field is not null
            if (subtopicData[requiredFields[i]].isNull()) {
                Serial.printf("❌ Field '%s': NULL VALUE\n", requiredFields[i]);
                allFieldsPresent = false;
            } else {
                const char* fieldValue = subtopicData[requiredFields[i]] | "";
                if (strlen(fieldValue) == 0) {
                    Serial.printf("❌ Field '%s': EMPTY VALUE\n", requiredFields[i]);
                    allFieldsPresent = false;
                } else if (!isValidSubtopicString(String(fieldValue))) {
                    Serial.printf("❌ Field '%s': INVALID CHARACTERS (%s)\n", requiredFields[i], fieldValue);
                    allFieldsPresent = false;
                } else if (strlen(fieldValue) > 20) {
                    Serial.printf("❌ Field '%s': TOO LONG (%d chars, max 20)\n", requiredFields[i], strlen(fieldValue));
                    allFieldsPresent = false;
                } else {
                    Serial.printf("✅ Field '%s': %s (VALID)\n", requiredFields[i], fieldValue);
                }
            }
        } else {
            Serial.printf("❌ Field '%s': MISSING\n", requiredFields[i]);
            allFieldsPresent = false;
        }
    }
    
    Serial.printf("Field check result: %s\n", allFieldsPresent ? "PASS" : "FAIL");
    Serial.println("======================================================");
    
    return allFieldsPresent;
}

// Helper function to validate subtopic string characters
bool isValidSubtopicString(const String& str) {
    for (int i = 0; i < str.length(); i++) {
        if (!isValidSubtopicCharacter(str.charAt(i))) {
            return false;
        }
    }
    return true;
}

// Helper function to validate individual characters
bool isValidSubtopicCharacter(char c) {
    // Allow alphanumeric, underscore, and hyphen
    return (c >= 'a' && c <= 'z') || 
           (c >= 'A' && c <= 'Z') || 
           (c >= '0' && c <= '9') || 
           c == '_' || c == '-';
}

// Function to check if subtopic data has changed
bool isSubtopicDataChanged(const String& newData) {
    Serial.println("=== CHECKING SUBTOPIC DATA CHANGES ===");
    
    // Read current saved data
    String savedData = fsManagerFFat.readFile("/subtopic");
    
    if (savedData.length() == 0) {
        Serial.println("[SUBTOPIC] No existing data found - treating as new configuration");
        return true;
    }
    
    // Parse both JSON objects for comparison
    DynamicJsonDocument savedDoc(1000);
    DynamicJsonDocument newDoc(1000);
    
    DeserializationError savedError = deserializeJson(savedDoc, savedData);
    DeserializationError newError = deserializeJson(newDoc, newData);
    
    if (savedError || newError) {
        Serial.printf("[SUBTOPIC] JSON parsing error - Saved: %s, New: %s\n", 
                     savedError.c_str(), newError.c_str());
        return true; // Treat parsing errors as changes
    }
    
    // Compare specific fields
    bool hasChanges = false;
    const char* fields[] = {"company_name", "location", "department", "machinename"};
    
    for (int i = 0; i < 4; i++) {
        String oldVal = savedDoc[fields[i]] | "";
        String newVal = newDoc[fields[i]] | "";
        
        if (!oldVal.equals(newVal)) {
            Serial.printf("[SUBTOPIC] %s changed: '%s' -> '%s'\n", fields[i], oldVal.c_str(), newVal.c_str());
            hasChanges = true;
        } else {
            Serial.printf("[SUBTOPIC] %s unchanged: '%s'\n", fields[i], oldVal.c_str());
        }
    }
    
    if (!hasChanges) {
        Serial.println("[SUBTOPIC] No changes detected in subtopic data");
    }
    
    Serial.println("====================================================");
    return hasChanges;
}

// // Status publishing function for subtopic
// void publishSubtopicStatus(const String& status, const String& message, const String& details) {
//     if (mqtt_obj.connectionStatus() != MQTT_CONNECTED) return;
    
//     DynamicJsonDocument statusDoc(400);
//     statusDoc["status"] = status;
//     statusDoc["message"] = message;
//     statusDoc["details"] = details;
//     statusDoc["timestamp"] = rtc.getDateTime();
    
//     if (status == "SUCCESS") {
//         statusDoc["current_config"] = subtopic;
//     }
    
//     String payload;
//     serializeJson(statusDoc, payload);
    
//     // Publish to subtopic status topic
//     mqtt_obj.publish(mqtt_obj.getMacTopic("subtopic/status").c_str(), payload.c_str(), true);
    
//     Serial.printf("Published subtopic status: %s\n", status.c_str());
// }

// Function to log subtopic changes
void logSubtopicChange(const DynamicJsonDocument& oldData, const DynamicJsonDocument& newData) {
    if (mqtt_obj.connectionStatus() != MQTT_CONNECTED) return;
    
    DynamicJsonDocument changeLog(1000);
    changeLog["event"] = "subtopic_changed";
    changeLog["timestamp"] = rtc.getDateTime();
    changeLog["old_config"] = oldData;
    changeLog["new_config"] = newData;
    
    // Highlight specific changes
    JsonObject changes = changeLog.createNestedObject("changes");
    const char* fields[] = {"company_name", "location", "department", "machinename"};
    
    for (int i = 0; i < 4; i++) {
        if (oldData[fields[i]] != newData[fields[i]]) {
            JsonObject fieldChange = changes.createNestedObject(fields[i]);
            fieldChange["from"] = oldData[fields[i]];
            fieldChange["to"] = newData[fields[i]];
        }
    }
    
    String payload;
    serializeJson(changeLog, payload);
    
    mqtt_obj.publish(mqtt_obj.getMacTopic("subtopic/change_log").c_str(), payload.c_str(), true);
    
    Serial.println("[SUBTOPIC] Change logged to MQTT");
}
