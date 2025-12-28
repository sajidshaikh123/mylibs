#pragma once
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"
#include "FFat.h"
#include "SD.h"
#include "WString.h"

enum class FilesystemType {
    FFAT,
    SPIFFS,
    SD_CARD
};

class FilesystemManager {
public:
    FilesystemManager();
    FilesystemManager(FilesystemType fsType);
    bool init();
    bool init(FilesystemType fsType);
    void displayPartitionAndFilesystemInfo();
    String getFilesystemName();
    fs::FS* getActiveFilesystem();
    bool isFilesystemMounted();
    void listDir(String dirname, uint8_t levels = 0);
    String listDirStr(const String &dirname,int counter = 255);
    void createDir(const String &path);
    bool search(const String &path);
    void displayFile(const String &path);
    String readFile(const String &path);
    bool writeFile(const String &path, const char *message);
    void appendFile(const String &path, const char *message);
    bool renameFile(const String &path1, const String &path2);
    bool deleteFile(const String &path);
    bool deleteDir(const String &path);
    bool format(bool quickFormat = true);
    bool readJSON(const String &path, DynamicJsonDocument &jsonDoc);
    bool readJSONOBJ(const String &path, DynamicJsonDocument *&jsonDoc);
    bool writeJSON(const String &path, const DynamicJsonDocument &jsonDoc);
    size_t totalBytes();
    size_t usedBytes();
    size_t freeBytes();
private:
    fs::FS *activeFS;
    SPIClass spi = SPIClass(HSPI); // Use the HSPI hardware peripheral
    FilesystemType currentFSType;
    bool FS_status = false;
    bool mountFilesystem(FilesystemType fsType);
};
