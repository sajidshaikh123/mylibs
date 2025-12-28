
#ifndef SHIFT_TIMING_H
#define SHIFT_TIMING_H

/*************************************************************************************************/
DynamicJsonDocument shift(4000);

uint8_t current_shift = 0;
String current_shift_name="";
uint8_t current_date = 0;
uint8_t current_hr = -1;

// FUNCTION PROTOTYPES
int getShiftWorkingTime(JsonObject shift);
int getShiftTotalBreakTime(JsonObject shift);
float getShiftWorkingTimeHours(JsonObject shift);
int getShiftWorkingTimeMinutes(JsonObject shift);
String getShiftWorkingTimeFormatted(JsonObject shift);
int getElapsedWorkingTime(JsonObject shift);
int getRemainingWorkingTime(JsonObject shift);
float getWorkingTimeProgress(JsonObject shift);
void testShiftWorkingTime();

// Helper function prototypes (assuming these exist in your codebase)
int timeStringToSeconds(const char* timeStr);
int getCurrentSeconds();
int getTotalBreakSeconds(JsonObject shift, int currentTime);

// FUNCTION IMPLEMENTATIONS

// Function to get total shift working time excluding breaks (in seconds)
int getShiftWorkingTime(JsonObject shift) {
    if(shift.isNull()) {
        Serial.println("[SHIFT] Invalid shift object");
        return 0;
    }
    
    // Check required fields
    if(!shift.containsKey("shift_start_time") || !shift.containsKey("shift_end_time")) {
        Serial.println("[SHIFT] Missing start/end time fields");
        return 0;
    }
    
    const char* startStr = shift["shift_start_time"] | "";
    const char* endStr = shift["shift_end_time"] | "";
    
    if(strlen(startStr) < 7 || strlen(endStr) < 7) {
        Serial.println("[SHIFT] Invalid time format");
        return 0;
    }
    
    int shiftStart = timeStringToSeconds(startStr);
    int shiftEnd = timeStringToSeconds(endStr);
    
    // Calculate total shift duration (handle overnight shifts)
    int totalShiftTime;
    if (shiftEnd <= shiftStart) {
        // Overnight shift (e.g., 22:00 to 06:00)
        totalShiftTime = (86400 - shiftStart) + shiftEnd; // 86400 = 24 hours in seconds
    } else {
        // Normal shift (e.g., 06:00 to 14:00)
        totalShiftTime = shiftEnd - shiftStart;
    }
    
    // Calculate total break time
    int totalBreakTime = getShiftTotalBreakTime(shift);
    
    // Working time = Total shift time - Break time
    int workingTime = totalShiftTime - totalBreakTime;
    
    if(workingTime < 0) {
        Serial.println("[SHIFT] Warning: Working time is negative, setting to 0");
        workingTime = 0;
    }
    
    Serial.printf("[SHIFT] Total shift: %d sec, Breaks: %d sec, Working: %d sec\n", 
                  totalShiftTime, totalBreakTime, workingTime);
    
    return workingTime;
}

// Helper function to get total break time for a shift (in seconds)
int getShiftTotalBreakTime(JsonObject shift) {
    if(shift.isNull()) {
        return 0;
    }
    
    int totalBreakTime = 0;
    
    // Define break field pairs
    const char* breaks[][2] = {
        {"break1_start_time", "break1_end_time"},
        {"break2_start_time", "break2_end_time"},
        {"break3_start_time", "break3_end_time"}
    };
    
    for (int i = 0; i < 3; i++) {
        // Check if break fields exist
        if(!shift.containsKey(breaks[i][0]) || !shift.containsKey(breaks[i][1])) {
            continue; // Skip this break if fields don't exist
        }
        
        const char* breakStartStr = shift[breaks[i][0]] | "";
        const char* breakEndStr = shift[breaks[i][1]] | "";
        
        // Skip if either field is empty
        if(strlen(breakStartStr) < 7 || strlen(breakEndStr) < 7) {
            continue;
        }
        
        int breakStart = timeStringToSeconds(breakStartStr);
        int breakEnd = timeStringToSeconds(breakEndStr);
        
        // Skip if times are invalid or equal
        if(breakStart == breakEnd) {
            continue;
        }
        
        // Calculate break duration (handle overnight breaks if needed)
        int breakDuration;
        if (breakEnd <= breakStart) {
            // Overnight break (rare but possible)
            breakDuration = (86400 - breakStart) + breakEnd;
        } else {
            // Normal break
            breakDuration = breakEnd - breakStart;
        }
        
        // Validate break duration (reasonable limits)
        if(breakDuration > 0 && breakDuration <= 3600) { // Max 1 hour per break
            totalBreakTime += breakDuration;
            Serial.printf("[SHIFT] Break %d: %s to %s = %d sec\n", 
                         i+1, breakStartStr, breakEndStr, breakDuration);
        } else {
            Serial.printf("[SHIFT] Warning: Invalid break %d duration: %d sec\n", 
                         i+1, breakDuration);
        }
    }
    
    return totalBreakTime;
}

// Function to get working time in hours (decimal)
float getShiftWorkingTimeHours(JsonObject shift) {
    int workingSeconds = getShiftWorkingTime(shift);
    return (float)workingSeconds / 3600.0;
}

// Function to get working time in minutes
int getShiftWorkingTimeMinutes(JsonObject shift) {
    int workingSeconds = getShiftWorkingTime(shift);
    return workingSeconds / 60;
}

// Function to format working time as HH:MM:SS string
String getShiftWorkingTimeFormatted(JsonObject shift) {
    int workingSeconds = getShiftWorkingTime(shift);
    
    int hours = workingSeconds / 3600;
    int minutes = (workingSeconds % 3600) / 60;
    int seconds = workingSeconds % 60;
    
    return String(hours) + ":" + 
           (minutes < 10 ? "0" : "") + String(minutes) + ":" +
           (seconds < 10 ? "0" : "") + String(seconds);
}

// Function to get elapsed working time from shift start (excluding breaks)
int getElapsedWorkingTime(JsonObject shift) {
    if(shift.isNull()) {
        Serial.println("[ELAPSED] Invalid shift object");
        return 0;
    }
    
    const char* startStr = shift["shift_start_time"] | "";
    if(strlen(startStr) < 7) {
        Serial.println("[ELAPSED] Invalid start time");
        return 0;
    }
    
    int shiftStart = timeStringToSeconds(startStr);
    int currentTime = getCurrentSeconds();
    
    // Handle overnight shift
    if(currentTime < shiftStart) {
        currentTime += 86400;
    }
    
    int elapsedTotal = currentTime - shiftStart;
    if(elapsedTotal <= 0) {
        return 0;
    }
    
    // Get breaks that have occurred up to current time
    int elapsedBreakTime = getTotalBreakSeconds(shift, currentTime);
    
    int elapsedWorking = elapsedTotal - elapsedBreakTime;
    
    if(elapsedWorking < 0) {
        elapsedWorking = 0;
    }
    
    return elapsedWorking;
}

// Function to get remaining working time in shift
int getRemainingWorkingTime(JsonObject shift) {
    int totalWorkingTime = getShiftWorkingTime(shift);
    int elapsedWorkingTime = getElapsedWorkingTime(shift);
    
    int remaining = totalWorkingTime - elapsedWorkingTime;
    
    if(remaining < 0) {
        remaining = 0;
    }
    
    return remaining;
}

// Function to calculate working time percentage completed
float getWorkingTimeProgress(JsonObject shift) {
    int totalWorkingTime = getShiftWorkingTime(shift);
    int elapsedWorkingTime = getElapsedWorkingTime(shift);
    
    if(totalWorkingTime <= 0) {
        return 0.0;
    }
    
    float progress = ((float)elapsedWorkingTime / (float)totalWorkingTime) * 100.0;
    
    if(progress > 100.0) {
        progress = 100.0;
    }
    
    return progress;
}

// Test function for shift working time calculations
void testShiftWorkingTime() {
    Serial.println("\n=== TESTING: Shift Working Time Functions ===");
    
    if(shift.isNull() || shift.size() == 0) {
        Serial.println("[ERROR] No shift data available");
        return;
    }
    
    if(current_shift == 0 || current_shift > shift.size()) {
        Serial.println("[ERROR] Invalid current shift");
        return;
    }
    
    JsonObject currentShiftObj = shift[current_shift - 1];
    
    if(currentShiftObj.isNull()) {
        Serial.println("[ERROR] Current shift object is null");
        return;
    }
    
    Serial.printf("Testing shift %d:\n", current_shift);
    Serial.printf("Start: %s\n", (const char*)currentShiftObj["shift_start_time"]);
    Serial.printf("End: %s\n", (const char*)currentShiftObj["shift_end_time"]);
    
    // Test all working time functions
    int workingSeconds = getShiftWorkingTime(currentShiftObj);
    float workingHours = getShiftWorkingTimeHours(currentShiftObj);
    int workingMinutes = getShiftWorkingTimeMinutes(currentShiftObj);
    String formattedTime = getShiftWorkingTimeFormatted(currentShiftObj);
    
    int elapsedWorking = getElapsedWorkingTime(currentShiftObj);
    int remainingWorking = getRemainingWorkingTime(currentShiftObj);
    float progress = getWorkingTimeProgress(currentShiftObj);
    
    Serial.println("\n--- Results ---");
    Serial.printf("Total working time: %d seconds\n", workingSeconds);
    Serial.printf("Total working time: %.2f hours\n", workingHours);
    Serial.printf("Total working time: %d minutes\n", workingMinutes);
    Serial.printf("Formatted time: %s\n", formattedTime.c_str());
    Serial.printf("Elapsed working: %d seconds (%.2f hours)\n", elapsedWorking, elapsedWorking/3600.0);
    Serial.printf("Remaining working: %d seconds (%.2f hours)\n", remainingWorking, remainingWorking/3600.0);
    Serial.printf("Progress: %.1f%%\n", progress);
    
    Serial.println("=== Test Complete ===");
}

#endif // SHIFT_TIMING_H