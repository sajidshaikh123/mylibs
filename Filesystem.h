#include <ArduinoJson.h>

#include "WString.h"
#include "FS.h"
#include "SPIFFS.h"

#include "FFat.h"



/* You only need to format FILE_SYSTEM the first time you run a
   test or else use the FILE_SYSTEM plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_FILE_SYSTEM_IF_FAILED true

// #define FILE_SYSTEM SPIFFS
#define FILE_SYSTEM FFat 


void deleteFile(fs::FS &fs, String path);

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    // yield();
    // vTaskDelay(0);
    try { 
      File root = fs.open(dirname);
    
      if(!root){
          Serial.println("- failed to open directory");
          return;
      }
      if(!root.isDirectory()){
          Serial.println(" - not a directory");
          return;
      }

      File file = root.openNextFile();
      while(file){
          if(file.isDirectory()){
              Serial.print("  DIR : ");
              Serial.println(file.name());
              if(levels){
                  vTaskDelay(0);
                  yield();
                  listDir(fs, file.path(), levels -1);
              }
          } else {
              Serial.print("  FILE: ");
              Serial.print(file.name());
              Serial.print("\tSIZE: ");
              Serial.println(file.size());
            
          }
          file = root.openNextFile();
      }
    }catch (String error) {
      Serial.println("ERROR ---- ");
    }
}

String listDirstr(fs::FS &fs,const String& dirname) {
  Serial.print("Listing directory: ");
  Serial.println(dirname.c_str());
  String dirlist = "";
  // Open the directory
  File root = fs.open(dirname.c_str());
  if (!root) {
    Serial.println("Failed to open directory");
    return "";
  }

  // Check if it's actually a directory
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return "";
  }

  // Iterate over the files and sub-directories in the directory
  File file = root.openNextFile();
  while (file) {
    // If it's a sub-directory, print the name and recursively list its contents
    if (file.isDirectory()) {
      dirlist += String(file.name()) + String(",");
    }
    // If it's a file, print the name and size
    else {
      dirlist +=String(file.name()) + String(",");
    }
    file = root.openNextFile();
  }
  dirlist[dirlist.length()-1] = '\0';
  return(dirlist);
}

// Create a directory
void createDir(fs::FS &fs,const String path) {
  Serial.print("Creating Dir: ");
  Serial.println(path);
  if(fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

// Search for a file or directory
bool search(fs::FS &fs,const String path) {
  Serial.print("Search File/Dir: ");
  Serial.println(path.c_str());
  if(fs.exists(path)) {
    Serial.println("File/Dir exists");
    return true;
  } else {
    Serial.println("File/Dir Not exists");
    return false;
  }
}

void displayFile(fs::FS &fs, String path){
    Serial.printf("Reading file: %s\r\n", path.c_str());
    String data="";
    File file = fs.open(path.c_str());
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return ;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

String readFile(fs::FS &fs, String path){
    Serial.printf("Reading file: %s\r\n", path.c_str());
    String data="";
    yield();
    vTaskDelay(0);
    File file = fs.open(path.c_str());
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return "";
    }

    Serial.println("- read from file:");
    while(file.available()){
        data += String((char )file.read());
    }
    file.close();
    return data;
}


bool writeFile(fs::FS &fs, String path, const char * message){
    Serial.printf("Writing file: %s\r\n", path.c_str());

    File file = fs.open(path.c_str(), FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return false;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
		file.close();
		return false;
    }
    file.close();
	return true;
}

void appendFile(fs::FS &fs, String path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path.c_str(), FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, String path1, String path2){
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1.c_str(), path2.c_str() )) {
        Serial.println("- file renamed");
    } else {
        Serial.println("- rename failed");
    }
}

void deleteFile(fs::FS &fs, String path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path.c_str())){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

// Read a JSON file from the SD card at the given path and return it as a DynamicJsonDocument
DynamicJsonDocument readJSON(fs::FS &fs,const String path ) {
  Serial.print("Reading File : ");
  Serial.println(path.c_str());
  // Create an error JSON to return if there is an issue with reading the file
  DynamicJsonDocument JSON_OBJ(5000);
  JSON_OBJ.clear();

  // Open the file at the given path
  File file = fs.open(path.c_str());
  if (!file || file.isDirectory()) {
    // If the file can't be opened or is a directory, return the error JSON
    Serial.println("- failed to open file for reading");
    return JSON_OBJ;
  }
  Serial.print("- read from file: ");
  Serial.println(file.size());

  // Create a JSON object with a capacity of 32000 bytes (large enough for most JSON files)
  
  
  // Deserialize the contents of the file into the JSON object
  DeserializationError error = deserializeJson(JSON_OBJ, file);
  if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return JSON_OBJ;
  }
  // Serial.println(JSON_OBJ.memoryUsage());
  file.close();

  // // Print the JSON object to the Serial monitor
  // if(env)
  //     serializeJsonPretty(JSON_OBJ, Serial);

  // Return the JSON object
  return JSON_OBJ;
}

// Read a JSON file from the SD card at the given path and return it as a DynamicJsonDocument
bool readJSON2(fs::FS &fs,const String path,DynamicJsonDocument &json_ ) {
  Serial.print("Reading File : ");
  Serial.println(path.c_str());
  // Serial.println("1234");
  // Create an error JSON to return if there is an issue with reading the file
  // json_.clear();

  // Serial.print("Opening FILE ");
  // Open the file at the given path
  File file = fs.open(path.c_str());
  // Serial.print("FILE Opened ");
  if (!file || file.isDirectory()) {
    // If the file can't be opened or is a directory, return the error JSON
    Serial.println("- failed to open file for reading");
    return false;
  }
  Serial.print("- read from file: ");
  Serial.println(file.size());

  // Create a JSON object with a capacity of 32000 bytes (large enough for most JSON files)
  
  
  // Deserialize the contents of the file into the JSON object
  DeserializationError error = deserializeJson(json_, file);
  if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      json_.clear();
      return false;
  }
  // Serial.println(JSON_OBJ.memoryUsage());
  file.close();

  // // Print the JSON object to the Serial monitor
  // if(env)
      serializeJsonPretty(json_, Serial);

  // Return the JSON object
  return true;
}

bool readJSON3(fs::FS &fs,const String path,DynamicJsonDocument* &jsonPtr ) {
  Serial.print("Reading File 3: ");
  Serial.println(path.c_str());
  // Serial.println("1234");
  // Create an error JSON to return if there is an issue with reading the file
  // json_.clear();

  // Serial.print("Opening FILE ");
  // Open the file at the given path
  File file = fs.open(path.c_str());
  // Serial.print("FILE Opened ");
  if (!file || file.isDirectory()) {
    // If the file can't be opened or is a directory, return the error JSON
    Serial.println("- failed to open file for reading");
    return false;
  }
  size_t fileSize = file.size();
  Serial.print("- File size: ");
  Serial.println(fileSize);


  // Delete old document if already allocated
  if (jsonPtr != nullptr) {
    delete jsonPtr;
    jsonPtr = nullptr;
  }

  // Allocate new JSON document on heap with a buffer slightly larger than file
  jsonPtr = new DynamicJsonDocument(fileSize * 1.5);  // heuristic buffer size

  DeserializationError error = deserializeJson(*jsonPtr, file);
  file.close();

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    delete jsonPtr;
    jsonPtr = nullptr;
    return false;
  }

  return true;
}

// Read a JSON file from the SD card at the given path and return it as a DynamicJsonDocument
bool readJSON_OBJ(fs::FS &fs,const String& path ,DynamicJsonDocument *&json_) {
  Serial.print("Reading File : ");
  Serial.println(path.c_str());
  
  // Clean up previous allocation if it exists
  if (json_ != nullptr) {
    delete json_;
    json_ = nullptr;
  }

  //Open the file at the given path
  File file = fs.open(path.c_str());
  if (!file || file.isDirectory()) {
    // If the file can't be opened or is a directory, return the error JSON
    Serial.println("- failed to open file for reading");
    return false;
  }
  Serial.print("- read from file: ");
  Serial.println(file.size());

   json_ = new DynamicJsonDocument(file.size() * 1.6); 
  // Deserialize the contents of the file into the JSON object
  DeserializationError error = deserializeJson(*json_, file);
  if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
  }
  // Serial.println(JSON_OBJ.memoryUsage());
  file.close();
  return true;
}


bool resizeJsonDocument(DynamicJsonDocument *&doc, size_t newSize) {
  size_t json_size = 0;
  if (doc == nullptr) {
    // Allocate if not created yet
    doc = new DynamicJsonDocument(newSize);
    Serial.print("Allocated new JSON document with size: ");
    Serial.println(newSize);
    return true;
  }else{
      json_size = doc->memoryUsage();
      Serial.print(" OLD JSON size: ");
      Serial.println(json_size);
  }


  // Allocate new larger document
  DynamicJsonDocument *newDoc = new DynamicJsonDocument(json_size + newSize);
  if (!newDoc) {
    Serial.println("Failed to allocate larger JSON document");
    return false;
  }

  // Copy existing content into the new document
  newDoc->set(*doc);

  // Delete old document and replace with new one
  delete doc;
  doc = newDoc;

  Serial.print(" Resized JSON document to: ");
  Serial.println(newSize + json_size);
  return true;
}



bool filesystemInit(){

    // FILE_SYSTEM.format();
    if (!FILE_SYSTEM.begin(FORMAT_FILE_SYSTEM_IF_FAILED)) {
      Serial.println("FILE_SYSTEM Mount Failed");
      FILE_SYSTEM.format();
      return false;
    }

  listDir(FILE_SYSTEM, "/", 0);
  Serial.println(FILE_SYSTEM.totalBytes());
  Serial.println(FILE_SYSTEM.usedBytes());
  return true;
}

// Get filesystem name as string
String getFilesystemName() {
    #ifdef USE_FFAT
        return "FFat";
    #elif defined(USE_SPIFFS)
        return "SPIFFS";
    #elif defined(USE_SD)
        return "SD Card";
    #else
        return "Unknown";
    #endif
}



// Display all ESP32 partitions and filesystem info
void displayPartitionAndFilesystemInfo() {
    Serial.println("=== ESP32 PARTITION & FILESYSTEM INFO ===");
    
    // 1. Display all partitions
    Serial.println("\n--- PARTITION TABLE ---");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    
    int partitionCount = 0;
    while (it != NULL) {
        const esp_partition_t* partition = esp_partition_get(it);
        partitionCount++;
        
        Serial.printf("Partition %d:\n", partitionCount);
        Serial.printf("  Label: %s\n", partition->label);
        Serial.printf("  Type: ");
        
        // Display partition type
        switch(partition->type) {
            case ESP_PARTITION_TYPE_APP:
                Serial.print("APP");
                break;
            case ESP_PARTITION_TYPE_DATA:
                Serial.print("DATA");
                break;
            case ESP_PARTITION_TYPE_BOOTLOADER:
                Serial.print("BOOTLOADER");
                break;
            default:
                Serial.printf("UNKNOWN(0x%02x)", partition->type);
        }
        
        Serial.printf(" | Subtype: ");
        
        // Display partition subtype
        if(partition->type == ESP_PARTITION_TYPE_APP) {
            switch(partition->subtype) {
                case ESP_PARTITION_SUBTYPE_APP_FACTORY:
                    Serial.print("FACTORY");
                    break;
                case ESP_PARTITION_SUBTYPE_APP_OTA_0:
                    Serial.print("OTA_0");
                    break;
                case ESP_PARTITION_SUBTYPE_APP_OTA_1:
                    Serial.print("OTA_1");
                    break;
                default:
                    Serial.printf("APP_OTHER(0x%02x)", partition->subtype);
            }
        } else if(partition->type == ESP_PARTITION_TYPE_DATA) {
            switch(partition->subtype) {
                case ESP_PARTITION_SUBTYPE_DATA_OTA:
                    Serial.print("OTA_DATA");
                    break;
                case ESP_PARTITION_SUBTYPE_DATA_NVS:
                    Serial.print("NVS");
                    break;
                case ESP_PARTITION_SUBTYPE_DATA_SPIFFS:
                    Serial.print("SPIFFS");
                    break;
                case ESP_PARTITION_SUBTYPE_DATA_FAT:
                    Serial.print("FAT");
                    break;
                default:
                    Serial.printf("DATA_OTHER(0x%02x)", partition->subtype);
            }
        } else {
            Serial.printf("0x%02x", partition->subtype);
        }
        
        Serial.printf("\n  Address: 0x%08x\n", partition->address);
        Serial.printf("  Size: %u bytes (%.2f MB)\n", 
                     partition->size, 
                     partition->size / (1024.0 * 1024.0));
        Serial.printf("  Encrypted: %s\n", partition->encrypted ? "Yes" : "No");
        Serial.printf("  Read-only: %s\n", partition->readonly ? "Yes" : "No");
        Serial.println();
        
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
    
    Serial.printf("Total partitions found: %d\n\n", partitionCount);
    
    // // 2. Current running partition info
    // const esp_partition_t* running = esp_ota_get_running_partition();
    // if(running) {
    //     Serial.println("--- CURRENT RUNNING PARTITION ---");
    //     Serial.printf("Label: %s\n", running->label);
    //     Serial.printf("Address: 0x%08x\n", running->address);
    //     Serial.printf("Size: %.2f MB\n", running->size / (1024.0 * 1024.0));
    //     Serial.println();
    // }
    
    // // 3. OTA partition info
    // const esp_partition_t* next_ota = esp_ota_get_next_update_partition(NULL);
    // if(next_ota) {
    //     Serial.println("--- NEXT OTA PARTITION ---");
    //     Serial.printf("Label: %s\n", next_ota->label);
    //     Serial.printf("Address: 0x%08x\n", next_ota->address);
    //     Serial.printf("Size: %.2f MB\n", next_ota->size / (1024.0 * 1024.0));
    //     Serial.println();
    // }
    
    // 4. Filesystem information
    Serial.println("--- FILESYSTEM STATUS ---");
    
    // Check FFat
    if(FFat.begin(false)) {
        Serial.println("FFat filesystem: MOUNTED");
        Serial.printf("  Total: %.2f MB\n", FFat.totalBytes() / (1024.0 * 1024.0));
        Serial.printf("  Used: %.2f MB\n", FFat.usedBytes() / (1024.0 * 1024.0));
        Serial.printf("  Free: %.2f MB\n", FFat.freeBytes() / (1024.0 * 1024.0));
        Serial.printf("  Usage: %.1f%%\n", (FFat.usedBytes() * 100.0) / FFat.totalBytes());
        FFat.end();
    } else {
        Serial.println("FFat filesystem: NOT AVAILABLE");
    }
    
    // Check SPIFFS
    if(SPIFFS.begin(false)) {
        Serial.println("SPIFFS filesystem: MOUNTED");
        Serial.printf("  Total: %.2f MB\n", SPIFFS.totalBytes() / (1024.0 * 1024.0));
        Serial.printf("  Used: %.2f MB\n", SPIFFS.usedBytes() / (1024.0 * 1024.0));
        Serial.printf("  Free: %.2f MB\n", (SPIFFS.totalBytes() - SPIFFS.usedBytes()) / (1024.0 * 1024.0));
        Serial.printf("  Usage: %.1f%%\n", (SPIFFS.usedBytes() * 100.0) / SPIFFS.totalBytes());
        SPIFFS.end();
    } else {
        Serial.println("SPIFFS filesystem: NOT AVAILABLE");
    }
    
    // 5. Flash chip info
    Serial.println("--- FLASH CHIP INFO ---");
    Serial.printf("Flash size: %.2f MB\n", ESP.getFlashChipSize() / (1024.0 * 1024.0));
    Serial.printf("Flash speed: %u MHz\n", ESP.getFlashChipSpeed() / 1000000);
    Serial.printf("Flash mode: %u\n", ESP.getFlashChipMode());
    
    Serial.println("==========================================");
}

// // Get filesystem name that's currently active
// String getActiveFilesystemName() {
//     if(FFat.begin(false)) {
//         FFat.end();
//         return "FFat";
//     }
//     if(SPIFFS.begin(false)) {
//         SPIFFS.end();
//         return "SPIFFS";
//     }
//     return "None";
// }

// // Get partition info as JSON for MQTT
// void getPartitionInfoJSON(DynamicJsonDocument &doc) {
//     doc["active_filesystem"] = getActiveFilesystemName();
    
//     // Running partition
//     const esp_partition_t* running = esp_ota_get_running_partition();
//     if(running) {
//         doc["running_partition"]["label"] = running->label;
//         doc["running_partition"]["size_mb"] = running->size / (1024.0 * 1024.0);
//         doc["running_partition"]["address"] = String("0x") + String(running->address, HEX);
//     }
    
//     // Next OTA partition
//     const esp_partition_t* next_ota = esp_ota_get_next_update_partition(NULL);
//     if(next_ota) {
//         doc["next_ota_partition"]["label"] = next_ota->label;
//         doc["next_ota_partition"]["size_mb"] = next_ota->size / (1024.0 * 1024.0);
//         doc["next_ota_partition"]["address"] = String("0x") + String(next_ota->address, HEX);
//     }
    
//     // Filesystem info
//     if(FFat.begin(false)) {
//         doc["ffat"]["mounted"] = true;
//         doc["ffat"]["total_mb"] = FFat.totalBytes() / (1024.0 * 1024.0);
//         doc["ffat"]["used_mb"] = FFat.usedBytes() / (1024.0 * 1024.0);
//         doc["ffat"]["free_mb"] = FFat.freeBytes() / (1024.0 * 1024.0);
//         doc["ffat"]["usage_percent"] = (FFat.usedBytes() * 100.0) / FFat.totalBytes();
//         FFat.end();
//     } else {
//         doc["ffat"]["mounted"] = false;
//     }
    
//     if(SPIFFS.begin(false)) {
//         doc["spiffs"]["mounted"] = true;
//         doc["spiffs"]["total_mb"] = SPIFFS.totalBytes() / (1024.0 * 1024.0);
//         doc["spiffs"]["used_mb"] = SPIFFS.usedBytes() / (1024.0 * 1024.0);
//         doc["spiffs"]["free_mb"] = (SPIFFS.totalBytes() - SPIFFS.usedBytes()) / (1024.0 * 1024.0);
//         doc["spiffs"]["usage_percent"] = (SPIFFS.usedBytes() * 100.0) / SPIFFS.totalBytes();
//         SPIFFS.end();
//     } else {
//         doc["spiffs"]["mounted"] = false;
//     }
    
//     // Flash info
//     doc["flash"]["size_mb"] = ESP.getFlashChipSize() / (1024.0 * 1024.0);
//     doc["flash"]["speed_mhz"] = ESP.getFlashChipSpeed() / 1000000;
//     doc["flash"]["mode"] = ESP.getFlashChipMode();
// }


