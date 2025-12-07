#include "FilesystemManager.h"
#include <Arduino.h>

#define MMC_SCK 14 // Define the pin number for SCK
#define MMC_MISO 12 // Define the pin number for MISO
#define MMC_MOSI 13 // Define the pin number for MOSI
#define MMC_CS 15 // Define the pin number for chip select (CS)


FilesystemManager::FilesystemManager() : activeFS(nullptr), currentFSType(FilesystemType::FFAT) {}

FilesystemManager::FilesystemManager(FilesystemType fsType) : activeFS(nullptr), currentFSType(fsType) {}

bool FilesystemManager::init() {
    return mountFilesystem(currentFSType);
}

String FilesystemManager::getFilesystemName() {
    switch (currentFSType) {
        case FilesystemType::FFAT:
            return "FFat";
        case FilesystemType::SPIFFS:
            return "SPIFFS";
        case FilesystemType::SD_CARD:
            return "SD Card";
        default:
            return "Unknown";
    }
}

fs::FS* FilesystemManager::getActiveFilesystem() {
    return activeFS;
}

bool FilesystemManager::isFilesystemMounted() {
    return activeFS != nullptr;
}

bool FilesystemManager::init(FilesystemType fsType) {
    Serial.println("=== FILESYSTEM INITIALIZATION ===");
    currentFSType = fsType;
    
    if (mountFilesystem(fsType)) {
        Serial.printf("Filesystem mounted successfully: %s\n", getFilesystemName().c_str());
        listDir("/", 0);
        return true;
    }
    
    // Try fallback options
    Serial.println("Trying fallback filesystems...");
    
    if (fsType != FilesystemType::FFAT && mountFilesystem(FilesystemType::FFAT)) {
        currentFSType = FilesystemType::FFAT;
        Serial.println("Fallback to FFat successful");
        return true;
    }
    
    if (fsType != FilesystemType::SPIFFS && mountFilesystem(FilesystemType::SPIFFS)) {
        currentFSType = FilesystemType::SPIFFS;
        Serial.println("Fallback to SPIFFS successful");
        return true;
    }
    
    Serial.println(" ERROR: All filesystem initialization attempts failed!");
    return false;
}

bool FilesystemManager::mountFilesystem(FilesystemType fsType) {
    // Clear previous filesystem pointer
    activeFS = nullptr;
    
    switch (fsType) {
        case FilesystemType::FFAT:
            if (FFat.begin(true)) {
                activeFS = &FFat;
                currentFSType = FilesystemType::FFAT;
                Serial.printf("FFat Total: %.2f MB\n", FFat.totalBytes() / (1024.0 * 1024.0));
                Serial.printf("FFat Used: %.2f MB\n", FFat.usedBytes() / (1024.0 * 1024.0));
                Serial.printf("FFat Free: %.2f MB\n", FFat.freeBytes() / (1024.0 * 1024.0));
                return true;
            } else {
                Serial.println("FFat mount failed - attempting format...");
                if (FFat.format(true) && FFat.begin(true)) {
                    activeFS = &FFat;
                    currentFSType = FilesystemType::FFAT;
                    Serial.println("FFat formatted and mounted successfully");
                    return true;
                }
            }
            break;
            
        case FilesystemType::SPIFFS:
            if (SPIFFS.begin(true)) {
                activeFS = &SPIFFS;
                currentFSType = FilesystemType::SPIFFS;
                Serial.printf("SPIFFS Total: %.2f MB\n", SPIFFS.totalBytes() / (1024.0 * 1024.0));
                Serial.printf("SPIFFS Used: %.2f MB\n", SPIFFS.usedBytes() / (1024.0 * 1024.0));
                Serial.printf("SPIFFS Free: %.2f MB\n", (SPIFFS.totalBytes() - SPIFFS.usedBytes()) / (1024.0 * 1024.0));
                return true;
            }
            break;
            
        case FilesystemType::SD_CARD:
            spi.begin(MMC_SCK, MMC_MISO, MMC_MOSI, MMC_CS);

            delay(100);
            if (SD.begin(MMC_CS, spi, 8000000, "/sd", 5, true)) {
                activeFS = &SD;
                currentFSType = FilesystemType::SD_CARD;
                Serial.println("SD Card mounted successfully");
                // Note: SD card doesn't have totalBytes() method like SPIFFS/FFat
                Serial.println("SD Card filesystem information not available via standard API");
                return true;
            } else {
                Serial.println("SD Card mount failed");
            }
            break;
    }
    return false;
}

void FilesystemManager::listDir(String dirname, uint8_t levels) {
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return;
    }

    Serial.printf("Listing directory: %s\r\n", dirname.c_str());
    File root = activeFS->open(dirname.c_str());
    if (!root) {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println(" - not a directory");
        return;
    }
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                vTaskDelay(0);
                yield();
                listDir(file.path(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

String FilesystemManager::listDirStr(const String &dirname) {
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return "";
    }
    
    Serial.print("Listing directory: ");
    Serial.println(dirname.c_str());
    String dirlist = "";
    File root = activeFS->open(dirname.c_str());
    if (!root || !root.isDirectory()) return "";
    File file = root.openNextFile();
    while (file) {
        dirlist += String(file.name()) + String(",");
        file = root.openNextFile();
    }
    if (dirlist.length() > 0) dirlist.remove(dirlist.length() - 1);
    return dirlist;
}

void FilesystemManager::createDir(const String &path) {
    Serial.print("Creating Dir: ");
    Serial.println(path);
    if (activeFS->mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

bool FilesystemManager::search(const String &path) {
    Serial.print("Search File/Dir: ");
    Serial.println(path.c_str());
    if (activeFS->exists(path)) {
        Serial.println("File/Dir exists");
        return true;
    } else {
        Serial.println("File/Dir Not exists");
        return false;
    }
}

void FilesystemManager::displayFile(const String &path) {
    Serial.printf("Reading file: %s\r\n", path.c_str());
    File file = activeFS->open(path.c_str());
    if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
        return;
    }
    Serial.println("- read from file:");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}

String FilesystemManager::readFile(const String &path) {
    Serial.printf("Reading file: %s\r\n", path.c_str());
    String data = "";
    File file = activeFS->open(path.c_str());
    if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
        return "";
    }
    while (file.available()) {
        data += String((char)file.read());
    }
    file.close();
    return data;
}

bool FilesystemManager::writeFile(const String &path, const char *message) {
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return false;
    }
    
    Serial.printf("Writing file: %s\r\n", path.c_str());
    File file = activeFS->open(path.c_str(), FILE_WRITE);
    if (!file) {
        Serial.println("- failed to open file for writing");
        return false;
    }
    bool ok = file.print(message);
    file.close();
    Serial.println(ok ? "- file written" : "- write failed");
    return ok;
}

void FilesystemManager::appendFile(const String &path, const char *message) {
    Serial.printf("Appending to file: %s\r\n", path.c_str());
    File file = activeFS->open(path.c_str(), FILE_APPEND);
    if (!file) {
        Serial.println("- failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

bool FilesystemManager::renameFile(const String &path1, const String &path2) {
    Serial.printf("Renaming file %s to %s\r\n", path1.c_str(), path2.c_str());
    if (activeFS->rename(path1.c_str(), path2.c_str())) {
        Serial.println("- file renamed");
        return true;
    } else {
        Serial.println("- rename failed");
        return false;
    }
}

bool FilesystemManager::deleteFile(const String &path) {
    Serial.printf("Deleting file: %s\r\n", path.c_str());
    if (activeFS->remove(path.c_str())) {
        Serial.println("- file deleted");
        return true;
    } else {
        Serial.println("- delete failed");
        return false;
    }
}

bool FilesystemManager::deleteDir(const String &path) {
    Serial.printf("Deleting directory: %s\r\n", path.c_str());
    
    // First, delete all contents recursively
    File root = activeFS->open(path.c_str());
    if (!root) {
        Serial.println("- failed to open directory");
        return false;
    }
    
    if (!root.isDirectory()) {
        Serial.println("- not a directory");
        root.close();
        return false;
    }
    
    // Get all files and subdirectories
    File file = root.openNextFile();
    while (file) {
        String fullPath = String(file.path());
        if (file.isDirectory()) {
            file.close();
            deleteDir(fullPath); // Recursive delete
        } else {
            file.close();
            deleteFile(fullPath); // Delete file
        }
        file = root.openNextFile();
    }
    root.close();
    
    // Now delete the empty directory
    if (activeFS->rmdir(path.c_str())) {
        Serial.println("- directory deleted");
        return true;
    } else {
        Serial.println("- directory delete failed");
        return false;
    }
}
bool FilesystemManager::format(bool quickFormat) {
    if (!activeFS) {
        Serial.println("Error: No active filesystem");
        return false;
    }

    bool success = false;
    
    switch (currentFSType) {
        case FilesystemType::FFAT:
            Serial.println("Formatting FFat filesystem...");
            FFat.end(); // Unmount before formatting
            success = FFat.format(!quickFormat); // true for full format, false for quick
            if (success) {
                success = FFat.begin(true); // Remount after formatting
            }
            break;
            
        case FilesystemType::SPIFFS:
            Serial.println("Formatting SPIFFS filesystem...");
            SPIFFS.end(); // Unmount before formatting
            success = SPIFFS.format();
            if (success) {
                success = SPIFFS.begin(true); // Remount after formatting
            }
            break;
            
        case FilesystemType::SD_CARD:
            Serial.println("Error: SD Card formatting is not supported through ESP32 SD library");
            Serial.println("Please format the SD card using a computer or dedicated tool");
            return false;
            
        default:
            Serial.println("Error: Unknown filesystem type");
            return false;
    }

    if (success) {
        Serial.println(" Filesystem formatted successfully");
        // Update activeFS pointer after remounting
        switch (currentFSType) {
            case FilesystemType::FFAT:
                activeFS = &FFat;
                break;
            case FilesystemType::SPIFFS:
                activeFS = &SPIFFS;
                break;
            default:
                break;
        }
    } else {
        Serial.println("Filesystem format failed");
        activeFS = nullptr;
    }

    return success;
}

bool FilesystemManager::readJSON(const String &path, DynamicJsonDocument &jsonDoc) {
    Serial.print("Reading JSON file: ");
    Serial.println(path.c_str());
    
    File file = activeFS->open(path.c_str());
    if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
        return false;
    }
    if(jsonDoc == nullptr){
        jsonDoc = DynamicJsonDocument(file.size() * 2); // Allocate double the file size for safety
    }  
    
    DeserializationError error = deserializeJson(jsonDoc, file);
    
    file.close();
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        jsonDoc.clear();
        return false;
    }
    return true;
}


bool FilesystemManager::readJSONOBJ(const String &path, DynamicJsonDocument *&jsonDoc) {
    Serial.print("Reading JSON file: ");
    Serial.println(path.c_str());
    
    File file = activeFS->open(path.c_str());
    if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
        return false;
    }
    if (jsonDoc != nullptr) {
        delete jsonDoc;
        jsonDoc = nullptr;
    }

    jsonDoc = new DynamicJsonDocument(file.size() * 2); // Allocate double the file size for safety

    DeserializationError error = deserializeJson(*jsonDoc, file);

    file.close();
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        jsonDoc->clear();
        return false;
    }
    return true;
}

bool FilesystemManager::writeJSON(const String &path, const DynamicJsonDocument &jsonDoc) {
    Serial.print("Writing JSON file: ");
    Serial.println(path.c_str());
    
    // Delete the file first to ensure we start fresh
    if (activeFS->exists(path.c_str())) {
        activeFS->remove(path.c_str());
    }
    
    File file = activeFS->open(path.c_str(), FILE_WRITE);
    if (!file) {
        Serial.println("- failed to open file for writing");
        return false;
    }
    
    size_t bytesWritten = serializeJson(jsonDoc, file);
    if (bytesWritten == 0) {
        Serial.println("- failed to write JSON");
        file.close();
        return false;
    }
    
    file.close();
    Serial.print("- JSON written successfully (");
    Serial.print(bytesWritten);
    Serial.println(" bytes)");
    return true;
}

void FilesystemManager::displayPartitionAndFilesystemInfo() {
    Serial.println("=== ESP32 PARTITION & FILESYSTEM INFO ===");
    // ...existing code...
}
