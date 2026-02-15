#include "FilesystemManager.h"
#include <Arduino.h>
#include "esp_task_wdt.h"

#define MMC_SCK 14 // Define the pin number for SCK
#define MMC_MISO 12 // Define the pin number for MISO
#define MMC_MOSI 13 // Define the pin number for MOSI
#define MMC_CS 15 // Define the pin number for chip select (CS)

// ========== Internal-RAM Stack Helper for FFat Operations ==========
// On ESP32-S3 with PSRAM enabled (CONFIG_SPIRAM_USE_MALLOC=y), the Arduino
// loopTask stack is allocated in PSRAM via heap_caps_malloc_default().
// When FFat.begin() or FFat.format() trigger SPI flash I/O, the ESP-IDF flash
// driver freezes the data cache. Since PSRAM is accessed through the data cache,
// any stack variable not already cached becomes inaccessible — the MSPI bus is
// busy with the flash transaction and can't service PSRAM cache misses.
// This causes "Guru Meditation: Cache error / MMU entry fault".
//
// Solution: Run FFat flash operations on a dedicated FreeRTOS task whose stack
// is explicitly allocated in internal SRAM (not PSRAM) using heap_caps_malloc
// with MALLOC_CAP_INTERNAL. Internal SRAM is directly addressable without cache.

#include "esp_heap_caps.h"

static volatile bool _fsOpResult = false;
static bool (*_fsOpFunc)() = nullptr;
static SemaphoreHandle_t _fsOpDone = NULL;
static TaskHandle_t _fsOpTaskHandle = NULL;

static void fsOpRunner(void* param) {
    if (_fsOpFunc) {
        _fsOpResult = _fsOpFunc();
    }
    xSemaphoreGive(_fsOpDone);
    vTaskSuspend(NULL);  // Suspend; calling task will delete us
}

// Static wrappers for FFat operations (used as function pointers)
static bool _ffatBeginNoFormat() { return FFat.begin(false); }
static bool _ffatFormat() { return FFat.format(); }

bool FilesystemManager::execOnInternalStack(bool (*func)()) {
    const size_t STACK_WORDS = 4096;  // 16KB stack in internal SRAM

    // Allocate stack in INTERNAL RAM (critical: must NOT be in PSRAM)
    StackType_t* stack = (StackType_t*)heap_caps_malloc(
        STACK_WORDS * sizeof(StackType_t),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (!stack) {
        Serial.println("[FS] WARN: Cannot allocate internal RAM for task stack");
        Serial.printf("[FS] Free internal: %u bytes\n",
            heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
        return func();  // Fallback: run directly (works when PSRAM is disabled)
    }

    static StaticTask_t tcbBuffer;
    _fsOpDone = xSemaphoreCreateBinary();
    if (!_fsOpDone) {
        heap_caps_free(stack);
        return func();
    }

    _fsOpFunc = func;
    _fsOpResult = false;

    _fsOpTaskHandle = xTaskCreateStaticPinnedToCore(
        fsOpRunner,
        "fs_op",
        STACK_WORDS,
        NULL,
        5,              // Above-normal priority
        stack,
        &tcbBuffer,
        1               // Core 1 (same as loopTask)
    );

    if (!_fsOpTaskHandle) {
        Serial.println("[FS] WARN: Task creation failed - running directly");
        heap_caps_free(stack);
        vSemaphoreDelete(_fsOpDone);
        return func();
    }

    // Wait for operation to complete
    xSemaphoreTake(_fsOpDone, portMAX_DELAY);

    // Task suspended itself — safe to delete and free memory
    vTaskDelete(_fsOpTaskHandle);
    _fsOpTaskHandle = NULL;
    vSemaphoreDelete(_fsOpDone);
    _fsOpDone = NULL;
    heap_caps_free(stack);

    return _fsOpResult;
}


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

size_t FilesystemManager::totalBytes() {
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return 0;
    }
    
    switch (currentFSType) {
        case FilesystemType::FFAT:
            return FFat.totalBytes();
            
        case FilesystemType::SPIFFS:
            return SPIFFS.totalBytes();
            
        case FilesystemType::SD_CARD:
            // SD card doesn't have direct totalBytes() method
            // Return approximate value or 0
            Serial.println("Warning: SD Card totalBytes() not available via standard API");
            return SD.totalBytes(); // This may or may not work depending on SD library version
            
        default:
            return 0;
    }
}

size_t FilesystemManager::usedBytes() {
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return 0;
    }
    
    switch (currentFSType) {
        case FilesystemType::FFAT:
            return FFat.usedBytes();
            
        case FilesystemType::SPIFFS:
            return SPIFFS.usedBytes();
            
        case FilesystemType::SD_CARD:
            // SD card doesn't have direct usedBytes() method
            Serial.println("Warning: SD Card usedBytes() not available via standard API");
            return SD.usedBytes(); // This may or may not work depending on SD library version
            
        default:
            return 0;
    }
}

size_t FilesystemManager::freeBytes() {
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return 0;
    }
    
    switch (currentFSType) {
        case FilesystemType::FFAT:
            return FFat.freeBytes();
            
        case FilesystemType::SPIFFS:
            // SPIFFS doesn't have freeBytes() method, calculate it
            return SPIFFS.totalBytes() - SPIFFS.usedBytes();
            
        case FilesystemType::SD_CARD:
            // SD card calculation
            Serial.println("Warning: SD Card freeBytes() calculated from total - used");
            return totalBytes() - usedBytes();
            
        default:
            return 0;
    }
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
        case FilesystemType::FFAT: {
            Serial.println("Attempting to mount FFat...");
            yield();
            delay(50);

            // Run on internal-RAM stack task (prevents PSRAM + cache freeze crash)
            bool mounted = execOnInternalStack(_ffatBeginNoFormat);

            if (mounted) {
                activeFS = &FFat;
                currentFSType = FilesystemType::FFAT;
                Serial.printf("FFat Total: %.2f MB\n", FFat.totalBytes() / (1024.0 * 1024.0));
                Serial.printf("FFat Used: %.2f MB\n", FFat.usedBytes() / (1024.0 * 1024.0));
                Serial.printf("FFat Free: %.2f MB\n", FFat.freeBytes() / (1024.0 * 1024.0));
                FS_status = true;
                return true;
            }

            Serial.println("FFat mount failed - partition may be unformatted.");
            Serial.println("Attempting format...");
            yield();
            delay(100);

            bool formatted = execOnInternalStack(_ffatFormat);

            if (formatted) {
                Serial.println("FFat format successful! Mounting...");
                yield();
                delay(50);

                mounted = execOnInternalStack(_ffatBeginNoFormat);

                if (mounted) {
                    activeFS = &FFat;
                    currentFSType = FilesystemType::FFAT;
                    Serial.println("FFat mounted after format.");
                    FS_status = true;
                    return true;
                }
                Serial.println("FFat mount after format failed.");
            } else {
                Serial.println("FFat format failed.");
            }

            Serial.println("FFat unavailable. Continuing without filesystem.");
            break;
        }
            
        case FilesystemType::SPIFFS: {
            yield();
            delay(50);
            if (SPIFFS.begin(true)) {
                activeFS = &SPIFFS;
                currentFSType = FilesystemType::SPIFFS;
                Serial.printf("SPIFFS Total: %.2f MB\n", SPIFFS.totalBytes() / (1024.0 * 1024.0));
                Serial.printf("SPIFFS Used: %.2f MB\n", SPIFFS.usedBytes() / (1024.0 * 1024.0));
                Serial.printf("SPIFFS Free: %.2f MB\n", (SPIFFS.totalBytes() - SPIFFS.usedBytes()) / (1024.0 * 1024.0));
                FS_status = true;
                return true;
            }
            break;
        }
            
        case FilesystemType::SD_CARD: {
            yield();
            spi.begin(MMC_SCK, MMC_MISO, MMC_MOSI, MMC_CS);
            yield();
            delay(100);
            if (SD.begin(MMC_CS, spi, 8000000, "/sd", 5, true)) {
                activeFS = &SD;
                currentFSType = FilesystemType::SD_CARD;
                Serial.println("SD Card mounted successfully");
                // Note: SD card doesn't have totalBytes() method like SPIFFS/FFat
                Serial.println("SD Card filesystem information not available via standard API");
                FS_status = true;
                return true;
            } else {
                Serial.println("SD Card mount failed");
            }
            break;
        }
    }
    FS_status = false;
    return false;
}

void FilesystemManager::listDir(String dirname, uint8_t levels) {
    

    Serial.printf("Listing directory: %s\r\n", dirname.c_str());
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return;
    }
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

String FilesystemManager::listDirStr(const String &dirname,int counter ) {
    
    
    Serial.print("Listing directory String: ");
    Serial.println(dirname.c_str());
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return "";
    }
    String dirlist = "";
    File root = activeFS->open(dirname.c_str());
    if (!root || !root.isDirectory()) return "";
    File file = root.openNextFile();
    while (file) {
        dirlist += String(file.name()) + String(",");
        file = root.openNextFile();
        counter++;
        if(counter > 10)
            break;
    }
    if (dirlist.length() > 0) dirlist.remove(dirlist.length() - 1);
    return dirlist;
}

void FilesystemManager::createDir(const String &path) {
    
    Serial.print("Creating Dir: ");
    Serial.println(path);
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return;
    }
    if (activeFS->mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

bool FilesystemManager::search(const String &path) {
    
    Serial.print("Search File/Dir: ");
    Serial.println(path.c_str());
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return false;
    }
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
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return;
    }
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
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return "";
    }
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
    
    
    Serial.printf("Writing file: %s\r\n", path.c_str());
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return false;
    }
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
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return;
    }

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
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return false;
    }
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
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return false;
    }
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
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return false;
    }
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
        case FilesystemType::FFAT: {
            Serial.println("Formatting FFat filesystem...");
            FFat.end(); // Unmount before formatting
            yield();
            delay(100);

            success = execOnInternalStack(_ffatFormat);
            yield();
            delay(100);

            if (success) {
                success = execOnInternalStack(_ffatBeginNoFormat);
            }
            break;
        }
            
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

bool FilesystemManager::safeFormatFFat() {
    Serial.println("=== SAFE FFat FORMAT ===");

    // Unmount if currently mounted
    if (currentFSType == FilesystemType::FFAT && activeFS != nullptr) {
        FFat.end();
        activeFS = nullptr;
        FS_status = false;
    }

    yield();
    delay(100);

    bool formatted = execOnInternalStack(_ffatFormat);

    if (!formatted) {
        Serial.println("FFat format failed.");
        return false;
    }

    Serial.println("FFat formatted. Mounting...");
    yield();
    delay(100);

    bool mounted = execOnInternalStack(_ffatBeginNoFormat);

    if (mounted) {
        activeFS = &FFat;
        currentFSType = FilesystemType::FFAT;
        FS_status = true;
        Serial.println("FFat formatted and mounted successfully.");
        return true;
    }

    Serial.println("FFat mount after format failed.");
    return false;
}

bool FilesystemManager::readJSON(const String &path, DynamicJsonDocument &jsonDoc) {
    
    Serial.print("Reading JSON file: ");
    Serial.println(path.c_str());
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return false;
    }
    File file = activeFS->open(path.c_str());
    if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
        return false;
    }
    
    Serial.print("file Size :");
    Serial.println(file.size());

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
    Serial.println("- JSON read successfully: ");
    return true;
}


bool FilesystemManager::readJSONOBJ(const String &path, DynamicJsonDocument *&jsonDoc) {
    
    Serial.print("Reading JSON file: ");
    Serial.println(path.c_str());

    if (jsonDoc != nullptr) {
        delete jsonDoc;
        jsonDoc = nullptr;
    }
    
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return false;
    }
    
    File file = activeFS->open(path.c_str());
    if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
        return false;
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
    if (!isFilesystemMounted()) {
        Serial.println("Error: No filesystem mounted");
        return false;
    }

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
