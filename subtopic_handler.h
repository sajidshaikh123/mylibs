#ifndef SUBTOPIC_HANDLER_H
#define SUBTOPIC_HANDLER_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// External reference to subtopic preferences
extern Preferences subtopicsPref;
extern DynamicJsonDocument subtopic;

// ==================== SUBTOPIC MANAGEMENT FUNCTIONS ====================

// Load subtopic data from Preferences to JSON
void loadSubtopicFromPreferences() {
    subtopicsPref.begin("subtopics", true);
    subtopic.clear();
    subtopic["company_name"] = subtopicsPref.getString("company", "embedsol");
    subtopic["location"] = subtopicsPref.getString("location", "bhosari");
    subtopic["department"] = subtopicsPref.getString("department", "iiot");
    subtopic["line"] = subtopicsPref.getString("line", "testing");
    subtopic["machinename"] = subtopicsPref.getString("machine", "M-TEST");
    
    subtopicsPref.end();
}

// Save subtopic data from JSON to Preferences
void saveSubtopicToPreferences() {
    subtopicsPref.end();
    subtopicsPref.begin("subtopics", false);
    
    if (subtopic.containsKey("company_name")) {
        subtopicsPref.putString("company", subtopic["company_name"].as<String>());
    }
    if (subtopic.containsKey("location")) {
        subtopicsPref.putString("location", subtopic["location"].as<String>());
    }
    if (subtopic.containsKey("department")) {
        subtopicsPref.putString("department", subtopic["department"].as<String>());
    }
    if (subtopic.containsKey("line")) {
        subtopicsPref.putString("line", subtopic["line"].as<String>());
    }
    if (subtopic.containsKey("machinename")) {
        subtopicsPref.putString("machine", subtopic["machinename"].as<String>());
    }
    
    subtopicsPref.end();
    subtopicsPref.begin("subtopics", true);
}

// Initialize subtopic with default values if not set
void initSubtopic() {
    subtopicsPref.begin("subtopics", true);
    
    // Check if already initialized
    if (!subtopicsPref.isKey("company")) {
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", false);
        
        // Set defaults
        subtopicsPref.putString("company", "embedsol");
        subtopicsPref.putString("location", "bhosari");
        subtopicsPref.putString("department", "iiot");
        subtopicsPref.putString("line", "testing");
        subtopicsPref.putString("machine", "M-TEST");
        
        Serial.println("[Subtopic] Initialized with default values");
        
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", true);
    }
    
    // Load into JSON
    loadSubtopicFromPreferences();
    
    subtopicsPref.end();
}

// ==================== SERIAL COMMAND HANDLER ====================

void printSubtopicHelp() {
    Serial.println("========= Subtopic Commands ==========");
    Serial.println("  subtopic show            - Show current subtopic config");
    Serial.println("  subtopic company <name>  - Set company name");
    Serial.println("  subtopic location <loc>  - Set location");
    Serial.println("  subtopic department <dept> - Set department");
    Serial.println("  subtopic line <line>     - Set production line");
    Serial.println("  subtopic machine <name>  - Set machine name");
    Serial.println("  subtopic clear           - Clear all subtopic config");
    Serial.println("  subtopic reset           - Reset to default values");
    Serial.println("Examples:");
    Serial.println("  subtopic company embedsol");
    Serial.println("  subtopic location bhosari");
    Serial.println("  subtopic machine M-TEST");
}

void handleSubtopicCommand(String args) {
    args.trim();
    
    // Parse subcommand
    int spaceIndex = args.indexOf(' ');
    String subCmd, subArgs;
    
    if (spaceIndex > 0) {
        subCmd = args.substring(0, spaceIndex);
        subArgs = args.substring(spaceIndex + 1);
        subArgs.trim();
    } else {
        subCmd = args;
        subArgs = "";
    }
    
    subCmd.toLowerCase();
    
    if (subCmd == "" || subCmd == "help" || subCmd == "?") {
        printSubtopicHelp();
    }
    else if (subCmd == "show") {
        Serial.println("=== Subtopic Configuration ===");
        
        subtopicsPref.begin("subtopics", true);
        
        String company = subtopicsPref.getString("company", "");
        String location = subtopicsPref.getString("location", "");
        String department = subtopicsPref.getString("department", "");
        String line = subtopicsPref.getString("line", "");
        String machine = subtopicsPref.getString("machine", "");
        
        Serial.printf("Company:    %s\n", company.length() > 0 ? company.c_str() : "(not set)");
        Serial.printf("Location:   %s\n", location.length() > 0 ? location.c_str() : "(not set)");
        Serial.printf("Department: %s\n", department.length() > 0 ? department.c_str() : "(not set)");
        Serial.printf("Line:       %s\n", line.length() > 0 ? line.c_str() : "(not set)");
        Serial.printf("Machine:    %s\n", machine.length() > 0 ? machine.c_str() : "(not set)");
        
        subtopicsPref.end();
        
        Serial.println("==============================");
    }
    else if (subCmd == "company") {
        if (subArgs.length() == 0) {
            Serial.println("[Subtopic] ✗ Error: Company name required");
            Serial.println("Usage: subtopic company <company_name>");
            return;
        }
        
        if (subArgs.length() > 64) {
            Serial.println("[Subtopic] ✗ Error: Company name too long (max 64 chars)");
            return;
        }
        
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", false);
        subtopicsPref.putString("company", subArgs);
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", true);
        
        // Update JSON
        subtopic["company_name"] = subArgs;
        
        Serial.printf("[Subtopic] ✓ Company name set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "location" || subCmd == "loc") {
        if (subArgs.length() == 0) {
            Serial.println("[Subtopic] ✗ Error: Location required");
            Serial.println("Usage: subtopic location <location>");
            return;
        }
        
        if (subArgs.length() > 64) {
            Serial.println("[Subtopic] ✗ Error: Location too long (max 64 chars)");
            return;
        }
        
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", false);
        subtopicsPref.putString("location", subArgs);
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", true);
        
        // Update JSON
        subtopic["location"] = subArgs;
        
        Serial.printf("[Subtopic] ✓ Location set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "department" || subCmd == "dept") {
        if (subArgs.length() == 0) {
            Serial.println("[Subtopic] ✗ Error: Department required");
            Serial.println("Usage: subtopic department <department>");
            return;
        }
        
        if (subArgs.length() > 64) {
            Serial.println("[Subtopic] ✗ Error: Department too long (max 64 chars)");
            return;
        }
        
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", false);
        subtopicsPref.putString("department", subArgs);
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", true);
        
        // Update JSON
        subtopic["department"] = subArgs;
        
        Serial.printf("[Subtopic] ✓ Department set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "line") {
        if (subArgs.length() == 0) {
            Serial.println("[Subtopic] ✗ Error: Line name required");
            Serial.println("Usage: subtopic line <line_name>");
            return;
        }
        
        if (subArgs.length() > 64) {
            Serial.println("[Subtopic] ✗ Error: Line name too long (max 64 chars)");
            return;
        }
        
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", false);
        subtopicsPref.putString("line", subArgs);
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", true);
        
        // Update JSON
        subtopic["line"] = subArgs;
        
        Serial.printf("[Subtopic] ✓ Line set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "machine") {
        if (subArgs.length() == 0) {
            Serial.println("[Subtopic] ✗ Error: Machine name required");
            Serial.println("Usage: subtopic machine <machine_name>");
            return;
        }
        
        if (subArgs.length() > 64) {
            Serial.println("[Subtopic] ✗ Error: Machine name too long (max 64 chars)");
            return;
        }
        
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", false);
        subtopicsPref.putString("machine", subArgs);
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", true);
        
        // Update JSON
        subtopic["machinename"] = subArgs;
        
        Serial.printf("[Subtopic] ✓ Machine name set to: %s\n", subArgs.c_str());
    }
    else if (subCmd == "clear") {
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", false);
        subtopicsPref.clear();
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", true);
        
        // Clear JSON
        subtopic["company_name"] = "";
        subtopic["location"] = "";
        subtopic["department"] = "";
        subtopic["line"] = "";
        subtopic["machinename"] = "";
        
        Serial.println("[Subtopic] ✓ Configuration cleared");
    }
    else if (subCmd == "reset") {
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", false);
        
        subtopicsPref.putString("company", "embedsol");
        subtopicsPref.putString("location", "bhosari");
        subtopicsPref.putString("department", "iiot");
        subtopicsPref.putString("line", "testing");
        subtopicsPref.putString("machine", "M-TEST");
        
        subtopicsPref.end();
        subtopicsPref.begin("subtopics", true);
        
        // Update JSON
        subtopic["company_name"] = "embedsol";
        subtopic["location"] = "bhosari";
        subtopic["department"] = "iiot";
        subtopic["line"] = "testing";
        subtopic["machinename"] = "M-TEST";
        
        Serial.println("[Subtopic] ✓ Reset to default values");
    }
    else {
        Serial.printf("[Subtopic] ✗ Unknown command: %s\n", subCmd.c_str());
        Serial.println("[Subtopic] Type 'subtopic help' for available commands");
    }
}

#endif // SUBTOPIC_HANDLER_H
