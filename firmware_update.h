// ...existing includes...
#include <Update.h>
#include <HTTPClient.h>
#include <Ethernet.h>   
#include <ArduinoJson.h>

// Callback function type for progress updates
typedef void (*OTAProgressCallback)(int progress, size_t currentBytes, size_t totalBytes, const String& status);


void publishOTAStatus(const String& status, const String& message);

// OTA Update variables
extern uint8_t conn_status; // 0: no connection, 1: connected, 2: authenticated
extern MQTT_Lib mqtt_obj;

bool ota_in_progress = false;
String ota_url = "";
unsigned long ota_start_time = 0;
const unsigned long OTA_TIMEOUT_MS = 300000; // 5 minutes timeout

// Progress tracking variables
int current_ota_progress = 0;
size_t current_bytes_downloaded = 0;
size_t total_firmware_size = 0;
String current_ota_status = "";

EthernetClient ethClient_update;

// Callback function pointer
OTAProgressCallback ota_progress_callback = nullptr;

// Function to set the callback
void setOTAProgressCallback(OTAProgressCallback callback) {
    ota_progress_callback = callback;
}

// Function to call the callback safely
void notifyOTAProgress(int progress, size_t currentBytes, size_t totalBytes, const String& status) {
    current_ota_progress = progress;
    current_bytes_downloaded = currentBytes;
    total_firmware_size = totalBytes;
    current_ota_status = status;
    
    if (ota_progress_callback != nullptr) {
        ota_progress_callback(progress, currentBytes, totalBytes, status);
    }
}

// Getter functions for main loop access
int getOTAProgress() { return current_ota_progress; }
size_t getCurrentBytes() { return current_bytes_downloaded; }
size_t getTotalBytes() { return total_firmware_size; }
String getOTAStatus() { return current_ota_status; }
bool isOTAInProgress() { return ota_in_progress; }

void sendOTAStatusToMQTT(const String& status, const String& message, const String& version = "") {
  if (!mqtt_obj.connected()) {
    Serial.println("✗ MQTT not connected, cannot send OTA status");
    return;
  }
  
  Serial.println("=== Sending OTA Status to MQTT ===");
  
  DynamicJsonDocument otaStatusDoc(400);
  
  otaStatusDoc["status"] = status;
  otaStatusDoc["message"] = message;
  otaStatusDoc["timestamp"] = rtc.getDateTime();
  otaStatusDoc["device_mac"] = WiFi.macAddress();
  
  if (version.length() > 0) {
    otaStatusDoc["new_version"] = version;
  }
  
  // Add partition information
  const esp_partition_t* running_partition = esp_ota_get_running_partition();
  if (running_partition != NULL) {
    otaStatusDoc["running_partition"] = running_partition->label;
    otaStatusDoc["partition_address"] = "0x" + String(running_partition->address, HEX);
  }
  
  // Add system info
  otaStatusDoc["free_heap"] = ESP.getFreeHeap();
  otaStatusDoc["uptime_ms"] = millis();
  
  // Serialize and send
  String otaStatusPayload;
  serializeJson(otaStatusDoc, otaStatusPayload);
  
  String otaStatusTopic = mqtt_obj.getMacTopic("ota/status");
  
  if (mqtt_obj.publish(otaStatusTopic.c_str(), otaStatusPayload.c_str(), true)) {
    Serial.println("✓ OTA status sent to MQTT successfully");
    Serial.println("Topic: " + otaStatusTopic);
    Serial.println("Status: " + status);
    Serial.println("Message: " + message);
  } else {
    Serial.println("✗ Failed to send OTA status to MQTT");
  }
  
  // Debug output
  Serial.println("=== OTA Status Sent ===");
  serializeJsonPretty(otaStatusDoc, Serial);
  Serial.println("=======================");
}

bool performOTAUpdateWithParts(const String& host, int port, const String& path) {
    if(ota_in_progress) {
        Serial.println("[OTA] Update already in progress");
        return false;
    }
    
    Serial.println("[OTA] Starting firmware update...");
    Serial.printf("[OTA] Host: %s, Port: %d, Path: %s\n", host.c_str(), port, path.c_str());
    
    ota_in_progress = true;
    ota_start_time = millis();
    
    // Notify start
    notifyOTAProgress(0, 0, 0, "Connecting to server...");
    
    // Direct connection using EthernetClient
    if(!ethClient_update.connect(host.c_str(), port)) {
        Serial.printf("[OTA] Connection failed to %s:%d\n", host.c_str(), port);
        notifyOTAProgress(0, 0, 0, "Connection failed");
        ota_in_progress = false;
        return false;
    }
    
    Serial.println("[OTA] Connected to server");
    notifyOTAProgress(5, 0, 0, "Connected, sending request...");
    
    // Send HTTP GET request
    ethClient_update.print("GET " + path + " HTTP/1.1\r\n");
    ethClient_update.print("Host: " + host + "\r\n");
    ethClient_update.print("User-Agent: ESP32-OTA-Client\r\n");
    ethClient_update.print("Connection: close\r\n\r\n");
    
    Serial.println("[OTA] HTTP request sent");
    notifyOTAProgress(10, 0, 0, "Request sent, waiting for response...");
    
    // Wait for response with timeout
    unsigned long timeout = millis() + 10000;
    while(!ethClient_update.available() && millis() < timeout) {
        delay(10);
        Ethernet.maintain();
    }
    
    if(!ethClient_update.available()) {
        Serial.println("[OTA] No response from server (timeout)");
        notifyOTAProgress(0, 0, 0, "Server timeout");
        ethClient_update.stop();
        ota_in_progress = false;
        return false;
    }
    
    Serial.println("[OTA] Response received, parsing headers...");
    notifyOTAProgress(15, 0, 0, "Parsing headers...");
    
    // Parse HTTP response headers
    String statusLine = "";
    int statusCode = 0;
    long contentLength = 0;
    bool headersEnd = false;
    String line = "";
    
    // Read status line first
    while(ethClient_update.available()) {
        char c = ethClient_update.read();
        if(c == '\n') {
            statusLine = line;
            Serial.print("[OTA] Status line: "); Serial.println(statusLine);
            
            // Extract status code (e.g., "HTTP/1.1 200 OK")
            int spaceIndex = statusLine.indexOf(' ');
            if(spaceIndex > 0) {
                statusCode = statusLine.substring(spaceIndex + 1, spaceIndex + 4).toInt();
            }
            break;
        } else if(c != '\r') {
            line += c;
        }
    }
    
    if(statusCode != 200) {
        Serial.printf("[OTA] HTTP status error: %d\n", statusCode);
        notifyOTAProgress(0, 0, 0, "HTTP Error: " + String(statusCode));
        ethClient_update.stop();
        ota_in_progress = false;
        return false;
    }
    
    Serial.println("[OTA] HTTP status OK (200)");
    notifyOTAProgress(20, 0, 0, "HTTP OK, reading headers...");
    
    // Parse remaining headers
    line = "";
    while(ethClient_update.available() && !headersEnd) {
        char c = ethClient_update.read();
        if(c == '\n') {
            if(line.startsWith("Content-Length:")) {
                contentLength = line.substring(15).toInt();
                Serial.printf("[OTA] Content-Length: %ld\n", contentLength);
            }
            if(line.length() <= 1) { // Empty line (just \r or empty)
                headersEnd = true;
                Serial.println("[OTA] Headers parsed, starting download...");
                break;
            }
            line = "";
        } else if(c != '\r') {
            line += c;
        }
    }
    
    if(contentLength <= 0) {
        Serial.println("[OTA] Invalid or missing Content-Length");
        notifyOTAProgress(0, 0, 0, "Invalid content length");
        ethClient_update.stop();
        ota_in_progress = false;
        return false;
    }
    
    delay(1000);
    notifyOTAProgress(25, 0, contentLength, "Preparing flash...");
    
    // Check available space
    if(!Update.begin(contentLength)) {
        Serial.printf("[OTA] Not enough space for update: %s\n", Update.errorString());
        notifyOTAProgress(0, 0, 0, "Not enough space");
        ethClient_update.stop();
        ota_in_progress = false;
        return false;
    }

    Serial.println("[OTA] Starting download...");
    notifyOTAProgress(30, 0, contentLength, "Starting download...");
    
    // Download and flash firmware
    long remaining = contentLength;
    size_t written = 0;
    uint8_t buffer[512]; // Smaller buffer for stability
    unsigned long lastProgressTime = 0;
    unsigned long lastDataTime = millis();
    unsigned long lastCallbackTime = 0;
    
    Serial.println("[OTA] Starting firmware download and flash...");
    
    while(remaining > 0 && ethClient_update.connected()) {
        // Check overall timeout
        if(millis() - ota_start_time > OTA_TIMEOUT_MS) {
            Serial.println("[OTA] Overall timeout during update");
            notifyOTAProgress(current_ota_progress, written, contentLength, "Timeout");
            break;
        }
        
        // Check data timeout (no data received for 30 seconds)
        if(millis() - lastDataTime > 30000) {
            Serial.println("[OTA] Data timeout (30s without data)");
            notifyOTAProgress(current_ota_progress, written, contentLength, "Data timeout");
            break;
        }
        
        if(ethClient_update.available()) {
            lastDataTime = millis();
            
            int toRead = min((int)sizeof(buffer), (int)remaining);
            int bytesRead = ethClient_update.readBytes(buffer, toRead);
            
            if(bytesRead > 0) {
                if(Update.write(buffer, bytesRead) != bytesRead) {
                    Serial.printf("[OTA] Write failed: %s\n", Update.errorString());
                    notifyOTAProgress(current_ota_progress, written, contentLength, "Write failed");
                    break;
                }
                
                written += bytesRead;
                remaining -= bytesRead;
                
                // Calculate progress (30% to 90% for download)
                int progress = 30 + (int)(((float)written / (float)contentLength) * 60.0);
                
                // Update callback every 1KB or every 1 second
                if((written % 1024 == 0) || (millis() - lastCallbackTime > 1000)) {
                    String status = "Downloading... " + String(written/1024) + "KB/" + String(contentLength/1024) + "KB";
                    notifyOTAProgress(progress, written, contentLength, status);
                    lastCallbackTime = millis();
                }
                
                // Progress update every 5KB or 5 seconds
                if((written % 5120 == 0) || (millis() - lastProgressTime > 5000)) {
                    float progressPercent = ((float)written / (float)contentLength) * 100.0;
                    Serial.printf("[OTA] Progress: %.1f%% (%ld/%ld bytes)\n", 
                                  progressPercent, written, contentLength);
                    lastProgressTime = millis();
                }
            }
        }
        
        Ethernet.maintain();
        delay(1); // Small delay to prevent watchdog
    }
    
    ethClient_update.stop();
    
    // Check if download completed successfully
    if(remaining != 0) {
        Serial.printf("[OTA] Download incomplete. Remaining: %ld bytes\n", remaining);
        notifyOTAProgress(current_ota_progress, written, contentLength, "Download incomplete");
        Update.abort();
        ota_in_progress = false;
        return false;
    }

    Serial.println("[OTA] Finalizing update...");
    notifyOTAProgress(95, written, contentLength, "Finalizing...");
    
    // Finalize the update
    if(Update.end(true)) {
        Serial.println("[OTA] Update completed successfully!");
        notifyOTAProgress(100, written, contentLength, "Update complete!");
        sendOTAStatusToMQTT("success", "Firmware update completed successfully", "2.0.0");
        
        Serial.println("[OTA] Restarting in 5 seconds...");
        
        delay(1000);
        
        // Countdown with callback updates
        for(int i = 5; i > 0; i--) {
            String countdownMsg = "Restarting in " + String(i) + " seconds...";
            notifyOTAProgress(100, written, contentLength, countdownMsg);
            Serial.printf("[OTA] Restarting in %d seconds...\n", i);
            delay(1000);
        }
        
        publishOTAStatus("SUCCESS", "Firmware updated successfully. Restarting...");
        notifyOTAProgress(100, written, contentLength, "Restarting now...");
        Serial.println("[OTA] Restarting now.");
        delay(1000);
        ESP.restart();
        return true;
    } else {
        Serial.printf("[OTA] Update failed during finalization: %s\n", Update.errorString());
        notifyOTAProgress(0, written, contentLength, "Finalization failed");
        ota_in_progress = false;
        return false;
    }
}

// Publish OTA status to server
void publishOTAStatus(const String& status, const String& message) {
    if(mqtt_obj.connectionStatus() != MQTT_CONNECTED) return;
    
    DynamicJsonDocument doc(300);
    doc["status"] = status;
    doc["message"] = message;
    doc["timestamp"] = rtc.getDateTime();
    doc["firmware_version"] = "1.0.0"; // Add your version
    doc["free_heap"] = ESP.getFreeHeap();
    
    String payload;
    serializeJson(doc, payload);
    
    // Publish to update/status topic
    bool published = mqtt_obj.publish(mqtt_obj.getTopic("update/status").c_str(), payload.c_str());
    Serial.print("[OTA] Status published: "); 
    Serial.print(payload);
    Serial.println(published ? " OK" : " FAIL");
}
// Updated handleOTARequest
void handleOTARequest(const String& firmwareUrl, const String& host = "", int port = 80, const String& path = "") {
    if(conn_status < 1) {
        Serial.println("[OTA] No network connection");
        publishOTAStatus("ERROR", "No network connection");
        return;
    }
    
    publishOTAStatus("STARTED", "OTA update started");
    
    bool success = false;
    
    if(host.length() > 0 && path.length() > 0) {
        // Use separate host/port/path
        success = performOTAUpdateWithParts(host, port, path);
    } else {
        // Parse from full URL
        String parseHost = "";
        int parsePort = 80;
        String parsePath = "/";
        
        if(firmwareUrl.startsWith("http://")) {
            String temp = firmwareUrl.substring(7);
            int slashIndex = temp.indexOf('/');
            if(slashIndex > 0) {
                parseHost = temp.substring(0, slashIndex);
                parsePath = temp.substring(slashIndex);
            } else {
                parseHost = temp;
            }
            
            int colonIndex = parseHost.indexOf(':');
            if(colonIndex > 0) {
                parsePort = parseHost.substring(colonIndex + 1).toInt();
                parseHost = parseHost.substring(0, colonIndex);
            }
        }
        
        success = performOTAUpdateWithParts(parseHost, parsePort, parsePath);
    }
    
    if(!success) {
        publishOTAStatus("FAILED", "OTA update failed");
    }
}