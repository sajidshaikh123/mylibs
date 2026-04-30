bool isValidIP(const String& ip);
bool isValidIdentifier(const String& str);


bool validateEthernetConfig(DynamicJsonDocument& config) {
    // Validate required fields exist
    if (!config.containsKey("dhcp")) {
        Serial.println("✗ Missing 'dhcp' field");
        return false;
    }
    
    bool dhcp = config["dhcp"];
    
    // If static IP configuration, validate IP settings
    if (!dhcp) {
        if (!config.containsKey("staticIp") || !config.containsKey("staticGateway") || 
            !config.containsKey("staticSubnet")) {
            Serial.println("✗ Missing static IP configuration fields");
            return false;
        }
        
        String ip = config["staticIp"];
        String gateway = config["staticGateway"];
        String subnet = config["staticSubnet"];
        String dns = config.containsKey("staticDns") ? config["staticDns"].as<String>() : "8.8.8.8";
        
        // Basic IP validation
        if (!isValidIP(ip)) {
            Serial.println("✗ Invalid static IP: " + ip);
            return false;
        }
        
        if (!isValidIP(gateway)) {
            Serial.println("✗ Invalid gateway IP: " + gateway);
            return false;
        }
        
        if (!isValidIP(subnet)) {
            Serial.println("✗ Invalid subnet mask: " + subnet);
            return false;
        }
        
        if (dns.length() > 0 && !isValidIP(dns)) {
            Serial.println("✗ Invalid DNS IP: " + dns);
            return false;
        }
        
        // Update config with validated values
        config["IP"] = ip;
        config["gateway"] = gateway;
        config["subnet"] = subnet;
        config["dns"] = (dns.length() > 0) ? dns : "8.8.8.8";
        config["DHCP"] = 0;
    } else {
        // DHCP configuration
        config["DHCP"] = 1;
        config["IP"] = "192.168.1.200";  // Default fallback
        config["gateway"] = "192.168.1.1";
        config["subnet"] = "255.255.255.0";
        config["dns"] = "8.8.8.8";
    }
    
    // Validate speed and duplex settings
    if (config.containsKey("speed")) {
        String speed = config["speed"];
        if (speed != "auto" && speed != "10" && speed != "100") {
            Serial.println("✗ Invalid speed setting: " + speed);
            return false;
        }
    }
    
    if (config.containsKey("duplex")) {
        String duplex = config["duplex"];
        if (duplex != "auto" && duplex != "half" && duplex != "full") {
            Serial.println("✗ Invalid duplex setting: " + duplex);
            return false;
        }
    }
    
    // Set enable flag (assume enabled if configuration is being set)
    config["enable"] = 1;
    
    Serial.println("✓ Ethernet configuration validation passed");
    return true;
}

bool isValidIP(const String& ip) {
    // Basic IP validation - check format xxx.xxx.xxx.xxx
    int dotCount = 0;
    int lastDot = -1;
    
    for (int i = 0; i < ip.length(); i++) {
        char c = ip.charAt(i);
        if (c == '.') {
            dotCount++;
            if (i - lastDot - 1 == 0 || i - lastDot - 1 > 3) {
                return false; // Empty or too long segment
            }
            lastDot = i;
        } else if (c < '0' || c > '9') {
            return false; // Non-numeric character
        }
    }
    
    if (dotCount != 3) {
        return false; // Must have exactly 3 dots
    }
    
    // Validate each octet
    int start = 0;
    for (int i = 0; i <= ip.length(); i++) {
        if (i == ip.length() || ip.charAt(i) == '.') {
            String octet = ip.substring(start, i);
            int value = octet.toInt();
            
            if (value < 0 || value > 255) {
                return false; // Octet out of range
            }
            
            // Check for leading zeros (except for "0")
            if (octet.length() > 1 && octet.charAt(0) == '0') {
                return false;
            }
            
            start = i + 1;
        }
    }
    
    return true;
}

// Add this validation function before the existing functions

bool validateSubtopicConfig(DynamicJsonDocument& config) {
    Serial.println("=== Validating Subtopic Configuration ===");
    
    // Check for required fields
    if (!config.containsKey("company_name")) {
        Serial.println("✗ Missing required field: company_name");
        return false;
    }
    
    if (!config.containsKey("location")) {
        Serial.println("✗ Missing required field: location");
        return false;
    }
    
    if (!config.containsKey("department")) {
        Serial.println("✗ Missing required field: department");
        return false;
    }
    
    if (!config.containsKey("machinename")) {
        Serial.println("✗ Missing required field: machinename");
        return false;
    }
    
    // Validate field values and lengths
    String company_name = config["company_name"];
    String location = config["location"];
    String department = config["department"];  
    String machinename = config["machinename"];
    
    // Check string lengths (prevent excessive memory usage)
    if (company_name.length() == 0 || company_name.length() > 50) {
        Serial.println("✗ Invalid company_name length: " + String(company_name.length()));
        return false;
    }
    
    if (location.length() == 0 || location.length() > 50) {
        Serial.println("✗ Invalid location length: " + String(location.length()));
        return false;
    }
    
    if (department.length() == 0 || department.length() > 50) {
        Serial.println("✗ Invalid department length: " + String(department.length()));
        return false;
    }
    
    if (machinename.length() == 0 || machinename.length() > 50) {
        Serial.println("✗ Invalid machinename length: " + String(machinename.length()));
        return false;
    }
    
    // Validate characters (alphanumeric + underscore + hyphen only)
    if (!isValidIdentifier(company_name)) {
        Serial.println("✗ Invalid characters in company_name: " + company_name);
        return false;
    }
    
    if (!isValidIdentifier(location)) {
        Serial.println("✗ Invalid characters in location: " + location);
        return false;
    }
    
    if (!isValidIdentifier(department)) {
        Serial.println("✗ Invalid characters in department: " + department);
        return false;
    }
    
    if (!isValidIdentifier(machinename)) {
        Serial.println("✗ Invalid characters in machinename: " + machinename);
        return false;
    }
    
    // Validate optional 'line' field if present
    if (config.containsKey("line")) {
        String line = config["line"];
        if (line.length() > 50) {
            Serial.println("✗ Invalid line length: " + String(line.length()));
            return false;
        }
        if (line.length() > 0 && !isValidIdentifier(line)) {
            Serial.println("✗ Invalid characters in line: " + line);
            return false;
        }
    }
    
    Serial.println("✓ Subtopic configuration validation passed");
    Serial.println("  Company: " + company_name);
    Serial.println("  Location: " + location);
    Serial.println("  Department: " + department);
    if (config.containsKey("line")) Serial.println("  Line: " + config["line"].as<String>());
    Serial.println("  Machine: " + machinename);
    
    return true;
}

bool isValidIdentifier(const String& str) {
    // Allow alphanumeric characters, underscore, hyphen, and spaces
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (!isalnum(c) && c != '_' && c != '-' && c != ' ') {
            return false;
        }
    }
    return true;
}

void processMac(String topic,String message){

    if (topic.equals("subtopic")) {
      Serial.println("IN TOPIC");
      
      // Check available memory before processing
      size_t freeHeap = ESP.getFreeHeap();
      Serial.println("Free heap before subtopic processing: " + String(freeHeap));
      
      if (freeHeap < 5000) {
        Serial.println("✗ Insufficient memory for subtopic processing");
        
        // Send error response
        DynamicJsonDocument resultDoc(200);
        resultDoc["success"] = false;
        resultDoc["message"] = "Insufficient memory";
        String resultPayload;
        serializeJson(resultDoc, resultPayload);
        mqtt_obj.publish(mqtt_obj.getMacTopic("subtopic/status").c_str(), resultPayload.c_str());
        return;
      }
      
      // Use temporary larger document for parsing
      DynamicJsonDocument tempSubtopic(2000);
      DeserializationError error = deserializeJson(tempSubtopic, message);
      
      if (error) {
        Serial.print(F("deserializeJson() failed MAC subtopic status : "));
        Serial.println(error.f_str());
        Serial.println("Message length: " + String(message.length()));
        Serial.println("Document capacity: " + String(tempSubtopic.capacity()));
        
        // Send error response
        DynamicJsonDocument resultDoc(300);
        resultDoc["success"] = false;
        resultDoc["message"] = "JSON parsing failed: " + String(error.f_str());
        resultDoc["payload_size"] = message.length();
        String resultPayload;
        serializeJson(resultDoc, resultPayload);
        mqtt_obj.publish(mqtt_obj.getMacTopic("subtopic/status").c_str(), resultPayload.c_str());
        return;
      }
      
      // Validate subtopic configuration
      if (validateSubtopicConfig(tempSubtopic)) {
        // Clear and copy validated data to main subtopic document
        subtopic.clear();
        subtopic["company_name"] = tempSubtopic["company_name"];
        subtopic["location"] = tempSubtopic["location"];
        subtopic["department"] = tempSubtopic["department"];
        if (tempSubtopic.containsKey("line")) subtopic["line"] = tempSubtopic["line"];
        subtopic["machinename"] = tempSubtopic["machinename"];
        
        serializeJsonPretty(subtopic, Serial);
        
        // Save to Preferences (NVS)
        Preferences subtopicPref;
        subtopicPref.begin("subtopics", false);
        subtopicPref.putString("company", tempSubtopic["company_name"].as<String>());
        subtopicPref.putString("location", tempSubtopic["location"].as<String>());
        subtopicPref.putString("department", tempSubtopic["department"].as<String>());
        subtopicPref.putString("line", tempSubtopic.containsKey("line") ? tempSubtopic["line"].as<String>() : "");
        subtopicPref.putString("machine", tempSubtopic["machinename"].as<String>());
        subtopicPref.end();
        Serial.println("✓ Subtopic configuration saved to Preferences");

        // Update MQTT object with new subtopic
        mqtt_obj.setsubtopic(subtopic);

        // Send success response
        DynamicJsonDocument resultDoc(200);
        resultDoc["success"] = true;
        resultDoc["message"] = "Subtopic configuration updated successfully";
        String resultPayload;
        serializeJson(resultDoc, resultPayload);
        mqtt_obj.publish(mqtt_obj.getMacTopic("subtopic/status").c_str(), resultPayload.c_str());
      } else {
        Serial.println("✗ Subtopic configuration validation failed");
        
        // Send error response
        DynamicJsonDocument resultDoc(200);
        resultDoc["success"] = false;
        resultDoc["message"] = "Invalid subtopic configuration";
        String resultPayload;
        serializeJson(resultDoc, resultPayload);
        mqtt_obj.publish(mqtt_obj.getMacTopic("subtopic/status").c_str(), resultPayload.c_str());
      }
    } else if (topic.equals("ethernet")) {
        Serial.println("IN ethernet");
        DynamicJsonDocument ethernetDoc(1024);
        DeserializationError error = deserializeJson(ethernetDoc, message);
        if (error) {
            Serial.print(F("deserializeJson() failed MAC ethernet status : "));
            Serial.println(error.f_str());
            return;
        }
        
        // Enhanced validation for ethernet configuration
        if (validateEthernetConfig(ethernetDoc)) {
            serializeJsonPretty(ethernetDoc, Serial);
            
            // Save to Preferences (NVS) using same keys as boardinit() reads
            Preferences ethPref;
            ethPref.begin("ethernet", false);
            ethPref.putBool("enabled", ethernetDoc.containsKey("enabled") ? ethernetDoc["enabled"].as<bool>() : true);
            ethPref.putBool("dhcp", ethernetDoc.containsKey("dhcp") ? ethernetDoc["dhcp"].as<bool>() : true);
            ethPref.putString("ip", ethernetDoc["IP"].as<String>());
            ethPref.putString("gateway", ethernetDoc["gateway"].as<String>());
            ethPref.putString("subnet", ethernetDoc["subnet"].as<String>());
            ethPref.putString("dns", ethernetDoc["dns"].as<String>());
            if (ethernetDoc.containsKey("speed")) ethPref.putString("speed", ethernetDoc["speed"].as<String>());
            if (ethernetDoc.containsKey("duplex")) ethPref.putString("duplex", ethernetDoc["duplex"].as<String>());
            ethPref.end();
            
            // Send success response
            DynamicJsonDocument resultDoc(200);
            resultDoc["success"] = true;
            resultDoc["message"] = "Ethernet configuration saved to Preferences";
            String resultPayload;
            serializeJson(resultDoc, resultPayload);
            mqtt_obj.publish(mqtt_obj.getMacTopic("ethernet/status").c_str(), resultPayload.c_str());
            
            Serial.println("✓ Ethernet configuration validated and saved to Preferences");
        } else {
            Serial.println("✗ Ethernet configuration validation failed");
            
            // Send error response
            DynamicJsonDocument resultDoc(200);
            resultDoc["success"] = false;
            resultDoc["message"] = "Invalid ethernet configuration";
            String resultPayload;
            serializeJson(resultDoc, resultPayload);
            mqtt_obj.publish(mqtt_obj.getMacTopic("ethernet/status").c_str(), resultPayload.c_str());
        } 
    } else if (topic.equals("wifi")) {
        Serial.println("IN wifi");
        DynamicJsonDocument wifiDoc(1024);
        DeserializationError error = deserializeJson(wifiDoc, message);
        if (error) {
            Serial.print(F("deserializeJson() failed MAC wifi : "));
            Serial.println(error.f_str());
            return;
        }

        // Validate required fields
        bool valid = true;
        String errMsg = "";

        if (!wifiDoc.containsKey("ssid")) {
            errMsg = "Missing required field: ssid";
            valid = false;
        } else if (wifiDoc["ssid"].as<String>().length() == 0 || wifiDoc["ssid"].as<String>().length() > 32) {
            errMsg = "Invalid ssid length (1-32)";
            valid = false;
        }

        if (valid && wifiDoc.containsKey("password")) {
            String pwd = wifiDoc["password"].as<String>();
            if (pwd.length() > 0 && pwd.length() < 8) {
                errMsg = "Password must be at least 8 characters";
                valid = false;
            } else if (pwd.length() > 63) {
                errMsg = "Password too long (max 63)";
                valid = false;
            }
        }

        if (valid && wifiDoc.containsKey("dhcp") && !wifiDoc["dhcp"].as<bool>()) {
            // Static IP mode - validate IP fields
            if (!wifiDoc.containsKey("ip") || !wifiDoc.containsKey("gateway") || !wifiDoc.containsKey("subnet")) {
                errMsg = "Static IP mode requires ip, gateway, subnet";
                valid = false;
            } else {
                if (!isValidIP(wifiDoc["ip"].as<String>())) { errMsg = "Invalid IP: " + wifiDoc["ip"].as<String>(); valid = false; }
                else if (!isValidIP(wifiDoc["gateway"].as<String>())) { errMsg = "Invalid gateway: " + wifiDoc["gateway"].as<String>(); valid = false; }
                else if (!isValidIP(wifiDoc["subnet"].as<String>())) { errMsg = "Invalid subnet: " + wifiDoc["subnet"].as<String>(); valid = false; }
                else if (wifiDoc.containsKey("dns") && wifiDoc["dns"].as<String>().length() > 0 && !isValidIP(wifiDoc["dns"].as<String>())) {
                    errMsg = "Invalid DNS: " + wifiDoc["dns"].as<String>(); valid = false;
                }
            }
        }

        if (valid) {
            bool useDHCP = wifiDoc.containsKey("dhcp") ? wifiDoc["dhcp"].as<bool>() : true;

            // Save to Preferences (NVS) using same keys as boardinit() reads
            Preferences wPref;
            wPref.begin("wifi", false);
            wPref.putBool("enabled", wifiDoc.containsKey("enabled") ? wifiDoc["enabled"].as<bool>() : true);
            wPref.putString("ssid", wifiDoc["ssid"].as<String>());
            wPref.putString("password", wifiDoc.containsKey("password") ? wifiDoc["password"].as<String>() : "");
            wPref.putBool("dhcp", useDHCP);
            if (!useDHCP) {
                wPref.putString("ip", wifiDoc["ip"].as<String>());
                wPref.putString("gateway", wifiDoc["gateway"].as<String>());
                wPref.putString("subnet", wifiDoc["subnet"].as<String>());
                wPref.putString("dns", wifiDoc.containsKey("dns") ? wifiDoc["dns"].as<String>() : "8.8.8.8");
            }
            wPref.end();

            Serial.println("✓ WiFi configuration validated and saved to Preferences");
            Serial.printf("  SSID: %s, DHCP: %s\n", wifiDoc["ssid"].as<String>().c_str(), useDHCP ? "Yes" : "No");

            DynamicJsonDocument resultDoc(200);
            resultDoc["success"] = true;
            resultDoc["message"] = "WiFi configuration saved to Preferences";
            String resultPayload;
            serializeJson(resultDoc, resultPayload);
            mqtt_obj.publish(mqtt_obj.getMacTopic("wifi/status").c_str(), resultPayload.c_str());
        } else {
            Serial.println("✗ WiFi validation failed: " + errMsg);

            DynamicJsonDocument resultDoc(300);
            resultDoc["success"] = false;
            resultDoc["message"] = errMsg;
            String resultPayload;
            serializeJson(resultDoc, resultPayload);
            mqtt_obj.publish(mqtt_obj.getMacTopic("wifi/status").c_str(), resultPayload.c_str());
        }
    } else if (topic.equals("rtc")) {
      Serial.println("IN datetime");
      DynamicJsonDocument dtDoc(256);
      DeserializationError error = deserializeJson(dtDoc, message);
      if (error) {
        Serial.print(F("deserializeJson() failed datetime status : "));
        Serial.println(error.f_str());
        return;
      }
      rtc.setDateTime((uint8_t)dtDoc["date"], (uint8_t)dtDoc["month"], (uint16_t)dtDoc["year"], (uint8_t)dtDoc["hours"], (uint8_t)dtDoc["minutes"], (uint8_t)dtDoc["seconds"]);
      DynamicJsonDocument resultDoc(200);
      resultDoc["success"] = true;
      String resultPayload;
      serializeJson(resultDoc, resultPayload);
      mqtt_obj.publish(mqtt_obj.getMacTopic("rtc/set").c_str(), resultPayload.c_str());
    } else if (topic.equals("ota_update")) {
        DynamicJsonDocument otaDoc(1024);
        deserializeJson(otaDoc, message);
        
        String firmwareUrl = otaDoc["url"];
        String host = otaDoc["host"];
        int port = otaDoc["port"];
        String path = otaDoc["path"];
        
        Serial.println("[MQTT] OTA update requested: " + firmwareUrl);
        
        // Start OTA update (callback will handle progress display)
        handleOTARequest(firmwareUrl, host, port, path);
    } else if (topic.equals("system_info")) {
      Serial.println("IN SYSTEM INFO REQUEST");
      
      DynamicJsonDocument sysDoc(600);
      sysDoc["timestamp"] = rtc.getDateTime();
      sysDoc["device_mac"] = WiFi.macAddress();
      sysDoc["free_heap"] = ESP.getFreeHeap();
      sysDoc["uptime_ms"] = millis();
      sysDoc["chip_model"] = ESP.getChipModel();
      sysDoc["chip_revision"] = ESP.getChipRevision();
      sysDoc["cpu_freq_mhz"] = ESP.getCpuFreqMHz();
      sysDoc["flash_size"] = ESP.getFlashChipSize();
      sysDoc["sketch_size"] = ESP.getSketchSize();
      sysDoc["free_sketch_space"] = ESP.getFreeSketchSpace();
      
      // Add running partition label
    //   const esp_partition_t* running = esp_ota_get_running_partition();
    //   if (running != NULL) {
    //     sysDoc["running_partition"] = running->label;
    //   }
      
      String sysPayload;
      serializeJson(sysDoc, sysPayload);
      
      String sysTopic = mqtt_obj.getMacTopic("system/info");
      mqtt_obj.publish(sysTopic.c_str(), sysPayload.c_str(), true);
      
      Serial.println("✓ System info sent to MQTT");
    } else if (topic.equals("reboot")) {
      mqtt_obj.publish(mqtt_obj.getMacTopic("reboot/status").c_str(), "rebooting");
      delay(1000);
      ESP.restart();
    } 
  
}