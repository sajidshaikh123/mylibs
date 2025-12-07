# IOT Board Version Management Guide

## Overview
Version management helps you track library versions, firmware versions, and board hardware revisions, making it easier to maintain compatibility and debug issues.

---

## Version Definitions in iotboard.h

### 1. **Library Version** (Semantic Versioning)
```cpp
#define IOTBOARD_VERSION_MAJOR 1    // Breaking changes
#define IOTBOARD_VERSION_MINOR 0    // New features (backward compatible)
#define IOTBOARD_VERSION_PATCH 0    // Bug fixes
#define IOTBOARD_VERSION "1.0.0"    // Combined version string
```

### 2. **Firmware Version**
```cpp
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0
#define FIRMWARE_VERSION "1.0.0"
```

### 3. **Board Hardware Version**
```cpp
#define BOARD_HW_VERSION "1.0"      // PCB revision
#define BOARD_MODEL "ESP32-IOT-BOARD"
```

### 4. **Build Information**
```cpp
#define IOTBOARD_BUILD_DATE __DATE__  // Compile date
#define IOTBOARD_BUILD_TIME __TIME__  // Compile time
```

---

## Best Practices

### When to Update Versions

#### MAJOR Version (X.0.0)
- Breaking API changes
- Incompatible hardware changes
- Major restructuring
- **Example**: Changing function signatures, removing functions

#### MINOR Version (1.X.0)
- New features added (backward compatible)
- New library added
- New functionality
- **Example**: Adding new sensor support, new communication protocol

#### PATCH Version (1.0.X)
- Bug fixes
- Performance improvements
- Documentation updates
- **Example**: Fixing a communication timeout, correcting pin definitions

---

## How to Use

### 1. Print Version Information
```cpp
#include <iotboard.h>

void setup() {
    Serial.begin(115200);
    IOTBoard::printVersionInfo();
}
```

### 2. Check Version at Compile Time
```cpp
// Only compile if version is 1.0.0 or higher
#if IOTBOARD_VERSION_CHECK(1, 0, 0)
    // Your code here
#endif

// Version-specific features
#if IOTBOARD_VERSION_MAJOR >= 2
    // Features for version 2.x.x
    useNewAPI();
#else
    // Fallback for version 1.x.x
    useOldAPI();
#endif
```

### 3. Runtime Version Checking
```cpp
void checkCompatibility() {
    Serial.print("Library Version: ");
    Serial.println(IOTBoard::getLibraryVersion());
    
    if (IOTBOARD_VERSION_MAJOR < 2) {
        Serial.println("Warning: Please update to version 2.0.0+");
    }
}
```

### 4. Store Version in EEPROM/Flash
```cpp
#include <Preferences.h>

void saveVersionInfo() {
    Preferences prefs;
    prefs.begin("iotboard", false);
    prefs.putString("lib_ver", IOTBOARD_VERSION);
    prefs.putString("fw_ver", FIRMWARE_VERSION);
    prefs.putString("hw_ver", BOARD_HW_VERSION);
    prefs.end();
}
```

### 5. Send Version via Network
```cpp
void sendVersionOverMQTT() {
    String payload = "{";
    payload += "\"library\":\"" + String(IOTBOARD_VERSION) + "\",";
    payload += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\",";
    payload += "\"hardware\":\"" + String(BOARD_HW_VERSION) + "\",";
    payload += "\"model\":\"" + String(BOARD_MODEL) + "\"";
    payload += "}";
    
    mqttClient.publish("device/version", payload.c_str());
}
```

---

## Version Tracking Workflow

### 1. **Development Phase**
- Use `-dev` or `-alpha` suffix
- Example: `1.1.0-dev`, `2.0.0-alpha`

### 2. **Testing Phase**
- Use `-beta` or `-rc` (release candidate)
- Example: `1.1.0-beta`, `2.0.0-rc1`

### 3. **Release Phase**
- Remove suffix, use clean version
- Example: `1.1.0`, `2.0.0`

### 4. **Update iotboard.h**
```cpp
// For development
#define IOTBOARD_VERSION "1.1.0-dev"

// For release
#define IOTBOARD_VERSION "1.1.0"
```

---

## Git Tagging Best Practices

### Tag releases in Git
```bash
# Tag the release
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0

# List all tags
git tag -l

# Checkout a specific version
git checkout v1.0.0
```

---

## Changelog Management

### Create CHANGELOG.md in your project root

```markdown
# Changelog

## [1.0.0] - 2025-11-01
### Added
- Initial release
- WiFi Manager support
- MQTT Library
- Ethernet Manager
- All peripheral libraries

### Changed
- N/A

### Fixed
- N/A

## [Unreleased]
### Added
- Version management system
```

---

## Individual Library Versioning

### For each library header (optional)
Add version macros to individual libraries:

```cpp
// In MQTT_Lib.h
#ifndef MQTT_LIB_H
#define MQTT_LIB_H

#define MQTT_LIB_VERSION "1.0.0"
#define MQTT_LIB_REQUIRED_IOTBOARD "1.0.0"

// Rest of the library...
#endif
```

---

## Board Hardware Version Tracking

### Physical Board Revisions
Update `BOARD_HW_VERSION` when:
- PCB revision changes
- Component changes
- Pin mapping changes
- Hardware features added/removed

### Example Hardware Versions
```cpp
// Rev 1.0 - Initial production
#define BOARD_HW_VERSION "1.0"

// Rev 1.1 - Added pull-up resistors
#define BOARD_HW_VERSION "1.1"

// Rev 2.0 - Changed MCU to ESP32-S3
#define BOARD_HW_VERSION "2.0"
```

### Hardware-Specific Code
```cpp
#if BOARD_HW_VERSION == "1.0"
    #define LED_PIN 2
#elif BOARD_HW_VERSION == "2.0"
    #define LED_PIN 8
#endif
```

---

## Version Info Display Examples

### LCD/OLED Display
```cpp
void displayVersion() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("IOT Board");
    display.print("Lib: "); display.println(IOTBOARD_VERSION);
    display.print("FW: "); display.println(FIRMWARE_VERSION);
    display.print("HW: "); display.println(BOARD_HW_VERSION);
    display.display();
}
```

### Web Interface
```cpp
void handleVersionAPI() {
    String json = "{";
    json += "\"library\":\"" + String(IOTBOARD_VERSION) + "\",";
    json += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"hardware\":\"" + String(BOARD_HW_VERSION) + "\",";
    json += "\"build\":\"" + String(IOTBOARD_BUILD_DATE) + " " + String(IOTBOARD_BUILD_TIME) + "\"";
    json += "}";
    server.send(200, "application/json", json);
}
```

---

## Automated Version Management

### Using Python Script (version_bump.py)
```python
#!/usr/bin/env python3
import re
import sys

def bump_version(file_path, bump_type):
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Extract current version
    major = int(re.search(r'VERSION_MAJOR (\d+)', content).group(1))
    minor = int(re.search(r'VERSION_MINOR (\d+)', content).group(1))
    patch = int(re.search(r'VERSION_PATCH (\d+)', content).group(1))
    
    # Bump version
    if bump_type == 'major':
        major += 1
        minor = 0
        patch = 0
    elif bump_type == 'minor':
        minor += 1
        patch = 0
    elif bump_type == 'patch':
        patch += 1
    
    # Update content
    content = re.sub(r'VERSION_MAJOR \d+', f'VERSION_MAJOR {major}', content)
    content = re.sub(r'VERSION_MINOR \d+', f'VERSION_MINOR {minor}', content)
    content = re.sub(r'VERSION_PATCH \d+', f'VERSION_PATCH {patch}', content)
    content = re.sub(r'VERSION "\d+\.\d+\.\d+"', f'VERSION "{major}.{minor}.{patch}"', content)
    
    with open(file_path, 'w') as f:
        f.write(content)
    
    print(f"Version bumped to {major}.{minor}.{patch}")

if __name__ == '__main__':
    bump_version('iotboard.h', sys.argv[1])  # Usage: python version_bump.py major|minor|patch
```

---

## Summary

1. **Update versions** in `iotboard.h` when making changes
2. **Use semantic versioning** (MAJOR.MINOR.PATCH)
3. **Tag releases** in Git
4. **Maintain CHANGELOG.md**
5. **Use version checks** for backward compatibility
6. **Display version info** for debugging
7. **Track hardware versions** separately
8. **Document breaking changes** clearly

This system helps you maintain professional version control and makes debugging and support much easier!
