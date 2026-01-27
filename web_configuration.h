#ifndef WEB_CONFIGURATION_H
#define WEB_CONFIGURATION_H

#include <Arduino.h>
#include <WiFi.h>
#include <Ethernet.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Update.h>
#include "RTCManager.h"
#include "FilesystemManager.h"

// Prevent HTTP method enum conflicts between ESPAsyncWebServer and ESP32 WebServer
// Undefine the conflicting macros from ESPAsyncWebServer before other libraries include WebServer
#undef HTTP_GET
#undef HTTP_POST  
#undef HTTP_DELETE
#undef HTTP_PUT
#undef HTTP_PATCH
#undef HTTP_HEAD
#undef HTTP_OPTIONS

// External references
extern Preferences wifiPref;
extern Preferences ethernetPref;
extern Preferences mqttPref;
extern Preferences hmiPref;
extern RTCManager rtc;
extern FilesystemManager fsManagerFFat;

// Web server on port 80
AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");

// Authentication credentials (store in preferences in production)
String web_username = "admin";
String web_password = "admin123";
bool web_auth_enabled = true;

// Server state
bool webServerStarted = false;
bool filesystemMounted = false;

// Forward declarations
void setupWebServer();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
String getSystemStatusJSON();
String getNetworkStatusJSON();
void initFilesystem();

// Helper function to get Ethernet MAC as String
String getEthernetMACString() {
    uint8_t mac[6];
    Ethernet.MACAddress(mac);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

// Initialize filesystem
void initFilesystem() {
    if (filesystemMounted) return;
    
    if (!fsManagerFFat.isFilesystemMounted()) {
        if (!fsManagerFFat.init()) {
            Serial.println("[FS] Filesystem mount failed!");
            return;
        }
    }
    
    filesystemMounted = true;
    Serial.printf("[FS] %s mounted successfully\n", fsManagerFFat.getFilesystemName().c_str());
    Serial.printf("[FS] Total: %d bytes, Used: %d bytes, Free: %d bytes\n", 
                  fsManagerFFat.totalBytes(), fsManagerFFat.usedBytes(), fsManagerFFat.freeBytes());
}


// ==================== AUTHENTICATION ====================

bool checkAuthentication(AsyncWebServerRequest *request) {
    if (!web_auth_enabled) return true;
    
    if (!request->authenticate(web_username.c_str(), web_password.c_str())) {
        request->requestAuthentication();
        return false;
    }
    return true;
}


// ==================== HTML PAGES ====================

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IoT Device Configuration</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 10px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.1);
            overflow: hidden;
            display: block;
            visibility: visible;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        .header h1 { font-size: 28px; margin-bottom: 10px; }
        .header p { opacity: 0.9; font-size: 14px; }
        
        .tabs {
            display: flex;
            background: #f5f5f5;
            border-bottom: 2px solid #ddd;
            overflow-x: auto;
        }
        .tab {
            padding: 15px 25px;
            cursor: pointer;
            border: none;
            background: transparent;
            font-size: 14px;
            color: #666;
            transition: all 0.3s;
            white-space: nowrap;
        }
        .tab:hover { background: #e0e0e0; }
        .tab.active {
            background: white;
            color: #667eea;
            border-bottom: 3px solid #667eea;
        }
        
        .content { padding: 30px; }
        .tab-content { display: none; }
        .tab-content.active { display: block; animation: fadeIn 0.3s; }
        
        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(10px); }
            to { opacity: 1; transform: translateY(0); }
        }
        
        .card {
            background: white;
            border: 1px solid #e0e0e0;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
        }
        .card h3 {
            color: #333;
            margin-bottom: 15px;
            padding-bottom: 10px;
            border-bottom: 2px solid #667eea;
        }
        
        .form-group {
            margin-bottom: 20px;
        }
        .form-group label {
            display: block;
            color: #555;
            margin-bottom: 8px;
            font-weight: 500;
            font-size: 14px;
        }
        .form-group input, .form-group select {
            width: 100%;
            padding: 12px;
            border: 1px solid #ddd;
            border-radius: 6px;
            font-size: 14px;
            transition: border 0.3s;
        }
        .form-group input:focus, .form-group select:focus {
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        
        .btn {
            padding: 12px 30px;
            border: none;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
            margin-right: 10px;
            margin-top: 10px;
        }
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.3);
        }
        .btn-secondary {
            background: #6c757d;
            color: white;
        }
        .btn-danger {
            background: #dc3545;
            color: white;
        }
        
        .status {
            display: inline-block;
            padding: 6px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 600;
        }
        .status.connected {
            background: #d4edda;
            color: #155724;
        }
        .status.disconnected {
            background: #f8d7da;
            color: #721c24;
        }
        
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-top: 20px;
        }
        .info-item {
            padding: 15px;
            background: #f8f9fa;
            border-radius: 6px;
            border-left: 4px solid #667eea;
        }
        .info-item label {
            display: block;
            color: #666;
            font-size: 12px;
            margin-bottom: 5px;
        }
        .info-item .value {
            color: #333;
            font-size: 16px;
            font-weight: 600;
        }
        
        .alert {
            padding: 15px;
            border-radius: 6px;
            margin-bottom: 20px;
            display: none;
        }
        .alert.success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .alert.error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        
        .toggle {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }
        .toggle input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 34px;
        }
        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }
        input:checked + .slider {
            background-color: #667eea;
        }
        input:checked + .slider:before {
            transform: translateX(26px);
        }
        
        .file-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px;
            border-bottom: 1px solid #e0e0e0;
            transition: background 0.2s;
        }
        .file-item:hover {
            background: #f8f9fa;
        }
        .file-item:last-child {
            border-bottom: none;
        }
        .file-info {
            flex: 1;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .file-icon {
            font-size: 20px;
        }
        .file-name {
            font-weight: 500;
            color: #333;
        }
        .file-size {
            color: #666;
            font-size: 12px;
        }
        .file-actions {
            display: flex;
            gap: 5px;
        }
        .btn-small {
            padding: 6px 12px;
            font-size: 12px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            transition: all 0.2s;
        }
        .btn-download {
            background: #28a745;
            color: white;
        }
        .btn-delete {
            background: #dc3545;
            color: white;
        }
        .btn-small:hover {
            opacity: 0.8;
            transform: translateY(-1px);
        }
        
        .modal {
            display: none;
            position: fixed;
            z-index: 1000;
            left: 0;
            top: 0;
            width: 100%;
            height: 100%;
            background-color: rgba(0,0,0,0.5);
        }
        .modal.active {
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .modal-content {
            background: white;
            padding: 30px;
            border-radius: 10px;
            width: 90%;
            max-width: 800px;
            max-height: 90vh;
            overflow-y: auto;
            box-shadow: 0 10px 40px rgba(0,0,0,0.3);
        }
        .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
            padding-bottom: 15px;
            border-bottom: 2px solid #667eea;
        }
        .modal-close {
            font-size: 28px;
            font-weight: bold;
            color: #999;
            cursor: pointer;
            border: none;
            background: none;
        }
        .modal-close:hover {
            color: #333;
        }
        .file-editor {
            width: 100%;
            min-height: 400px;
            font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
            font-size: 14px;
            padding: 15px;
            border: 1px solid #ddd;
            border-radius: 6px;
            resize: vertical;
        }
        .breadcrumb {
            display: flex;
            align-items: center;
            gap: 5px;
            margin-bottom: 15px;
            padding: 10px;
            background: #f8f9fa;
            border-radius: 6px;
            flex-wrap: wrap;
        }
        .breadcrumb-item {
            color: #667eea;
            cursor: pointer;
            text-decoration: none;
            font-weight: 500;
        }
        .breadcrumb-item:hover {
            text-decoration: underline;
        }
        .breadcrumb-separator {
            color: #999;
        }
        
        @media (max-width: 768px) {
            .container { margin: 10px; }
            .content { padding: 15px; }
            .tabs { overflow-x: scroll; }
            .file-item {
                flex-direction: column;
                align-items: flex-start;
                gap: 10px;
            }
            .file-actions {
                width: 100%;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌐 IoT Device Configuration</h1>
            <p>Production Monitoring System v1.1.0</p>
        </div>
        
        <div class="tabs">
            <button class="tab active" onclick="showTab('system', this)">📊 System</button>
            <button class="tab" onclick="showTab('network', this)">🌐 Network</button>
            <button class="tab" onclick="showTab('mqtt', this)">📡 MQTT</button>
            <button class="tab" onclick="showTab('rtc', this)">🕐 RTC</button>
            <button class="tab" onclick="showTab('hmi', this)">🖥️ HMI</button>
            <button class="tab" onclick="showTab('files', this)">📁 Files</button>
            <button class="tab" onclick="showTab('firmware', this)">⬆️ Firmware</button>
            <button class="tab" onclick="showTab('settings', this)">⚙️ Settings</button>
        </div>
        
        <div class="content">
            <div id="alert" class="alert"></div>
            
            <!-- System Tab -->
            <div id="system" class="tab-content active">
                <div class="card">
                    <h3>System Status</h3>
                    <div class="info-grid">
                        <div class="info-item">
                            <label>Chip Model</label>
                            <div class="value" id="chipModel">-</div>
                        </div>
                        <div class="info-item">
                            <label>Free Heap</label>
                            <div class="value" id="freeHeap">-</div>
                        </div>
                        <div class="info-item">
                            <label>CPU Frequency</label>
                            <div class="value" id="cpuFreq">-</div>
                        </div>
                        <div class="info-item">
                            <label>Uptime</label>
                            <div class="value" id="uptime">-</div>
                        </div>
                    </div>
                </div>
                
                <div class="card">
                    <h3>Network Status</h3>
                    <div class="info-grid">
                        <div class="info-item">
                            <label>WiFi Status</label>
                            <div class="value" id="wifiStatus">-</div>
                        </div>
                        <div class="info-item">
                            <label>Ethernet Status</label>
                            <div class="value" id="ethStatus">-</div>
                        </div>
                        <div class="info-item">
                            <label>IP Address</label>
                            <div class="value" id="ipAddress">-</div>
                        </div>
                        <div class="info-item">
                            <label>MAC Address</label>
                            <div class="value" id="macAddress">-</div>
                        </div>
                    </div>
                </div>
                
                <div class="card">
                    <h3>Actions</h3>
                    <button class="btn btn-primary" onclick="refreshStatus()">🔄 Refresh Status</button>
                    <button class="btn btn-secondary" onclick="rebootDevice()">🔁 Reboot Device</button>
                    <button class="btn btn-danger" onclick="factoryReset()">⚠️ Factory Reset</button>
                </div>
            </div>
            
            <!-- Network Tab -->
            <div id="network" class="tab-content">
                <div class="card">
                    <h3>WiFi Configuration</h3>
                    <div class="form-group">
                        <label>WiFi Enabled</label>
                        <label class="toggle">
                            <input type="checkbox" id="wifiEnabled">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div class="form-group">
                        <label>SSID</label>
                        <input type="text" id="wifiSsid" placeholder="Enter WiFi SSID">
                    </div>
                    <div class="form-group">
                        <label>Password</label>
                        <input type="password" id="wifiPassword" placeholder="Enter WiFi password">
                    </div>
                    <button class="btn btn-primary" onclick="saveWiFiConfig()">💾 Save WiFi Config</button>
                    <button class="btn btn-secondary" onclick="scanWiFi()">🔍 Scan Networks</button>
                </div>
                
                <div class="card">
                    <h3>Ethernet Configuration</h3>
                    <div class="form-group">
                        <label>Ethernet Enabled</label>
                        <label class="toggle">
                            <input type="checkbox" id="ethEnabled">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div class="form-group">
                        <label>DHCP Mode</label>
                        <label class="toggle">
                            <input type="checkbox" id="ethDhcp" onchange="toggleStaticIP()">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div id="staticIpFields">
                        <div class="form-group">
                            <label>IP Address</label>
                            <input type="text" id="ethIp" placeholder="192.168.1.100">
                        </div>
                        <div class="form-group">
                            <label>Gateway</label>
                            <input type="text" id="ethGateway" placeholder="192.168.1.1">
                        </div>
                        <div class="form-group">
                            <label>Subnet Mask</label>
                            <input type="text" id="ethSubnet" placeholder="255.255.255.0">
                        </div>
                        <div class="form-group">
                            <label>DNS Server</label>
                            <input type="text" id="ethDns" placeholder="8.8.8.8">
                        </div>
                    </div>
                    <button class="btn btn-primary" onclick="saveEthernetConfig()">💾 Save Ethernet Config</button>
                </div>
            </div>
            
            <!-- MQTT Tab -->
            <div id="mqtt" class="tab-content">
                <div class="card">
                    <h3>MQTT Broker Configuration</h3>
                    <div class="form-group">
                        <label>Broker Host</label>
                        <input type="text" id="mqttHost" placeholder="mqtt.example.com">
                    </div>
                    <div class="form-group">
                        <label>Port</label>
                        <input type="number" id="mqttPort" placeholder="1883">
                    </div>
                    <div class="form-group">
                        <label>Username</label>
                        <input type="text" id="mqttUser" placeholder="Username (optional)">
                    </div>
                    <div class="form-group">
                        <label>Password</label>
                        <input type="password" id="mqttPass" placeholder="Password (optional)">
                    </div>
                    <button class="btn btn-primary" onclick="saveMQTTConfig()">💾 Save MQTT Config</button>
                </div>
                
                <div class="card">
                    <h3>Subtopic Configuration</h3>
                    <div class="form-group">
                        <label>Company Name</label>
                        <input type="text" id="subCompany" placeholder="premierseals">
                    </div>
                    <div class="form-group">
                        <label>Location</label>
                        <input type="text" id="subLocation" placeholder="chinchwad">
                    </div>
                    <div class="form-group">
                        <label>Department</label>
                        <input type="text" id="subDepartment" placeholder="molding">
                    </div>
                    <div class="form-group">
                        <label>Line</label>
                        <input type="text" id="subLine" placeholder="injection">
                    </div>
                    <div class="form-group">
                        <label>Machine Name</label>
                        <input type="text" id="subMachine" placeholder="M-101">
                    </div>
                    <button class="btn btn-primary" onclick="saveSubtopicConfig()">💾 Save Subtopic Config</button>
                </div>
            </div>
            
            <!-- RTC Tab -->
            <div id="rtc" class="tab-content">
                <div class="card">
                    <h3>Real-Time Clock</h3>
                    <div class="info-grid">
                        <div class="info-item">
                            <label>Current Date/Time</label>
                            <div class="value" id="rtcDateTime">-</div>
                        </div>
                        <div class="info-item">
                            <label>RTC Type</label>
                            <div class="value" id="rtcType">-</div>
                        </div>
                    </div>
                    
                    <div style="margin-top: 20px;">
                        <div class="form-group">
                            <label>Set Date/Time</label>
                            <input type="datetime-local" id="rtcInput">
                        </div>
                        <button class="btn btn-primary" onclick="setRTC()">🕐 Set RTC</button>
                        <button class="btn btn-secondary" onclick="syncRTCBrowser()">🌐 Sync with Browser</button>
                    </div>
                </div>
            </div>
            
            <!-- HMI Tab -->
            <div id="hmi" class="tab-content">
                <div class="card">
                    <h3>HMI Display Configuration</h3>
                    <div class="form-group">
                        <label>HMI Enabled</label>
                        <label class="toggle">
                            <input type="checkbox" id="hmiEnabled">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <p style="color: #666; font-size: 14px; margin-top: 10px;">
                        ⚠️ Changes require device reboot to take effect
                    </p>
                    <button class="btn btn-primary" onclick="saveHMIConfig()">💾 Save HMI Config</button>
                </div>
            </div>
            
            <!-- Files Tab -->
            <div id="files" class="tab-content">
                <div class="card">
                    <h3>File Manager</h3>
                    <div class="info-grid" style="grid-template-columns: 1fr;">
                        <div class="info-item">
                            <label>Filesystem</label>
                            <div class="value" id="fsType">SPIFFS</div>
                        </div>
                        <div class="info-item">
                            <label>Total Space</label>
                            <div class="value" id="fsTotal">-</div>
                        </div>
                        <div class="info-item">
                            <label>Used Space</label>
                            <div class="value" id="fsUsed">-</div>
                        </div>
                    </div>
                </div>
                
                <div class="card">
                    <h3>Upload File</h3>
                    <div class="form-group">
                        <label>Select File</label>
                        <input type="file" id="fileToUpload">
                    </div>
                    <div class="form-group">
                        <label>Upload Path (e.g., /config.json)</label>
                        <input type="text" id="uploadPath" placeholder="/filename.ext">
                    </div>
                    <div id="fileUploadProgress" style="display: none; margin: 20px 0;">
                        <div style="background: #f0f0f0; border-radius: 10px; overflow: hidden;">
                            <div id="fileProgressBar" style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); height: 30px; width: 0%; transition: width 0.3s; display: flex; align-items: center; justify-content: center; color: white; font-weight: 600;">
                                <span id="fileProgressText">0%</span>
                            </div>
                        </div>
                    </div>
                    <button class="btn btn-primary" onclick="uploadFile()">📤 Upload File</button>
                </div>
                
                <div class="card">
                    <h3>Files & Directories</h3>
                    <div class="breadcrumb" id="breadcrumb">
                        <span class="breadcrumb-item" onclick="navigateToFolder('/')">📁 Root</span>
                    </div>
                    <button class="btn btn-secondary" onclick="refreshFileList()" style="margin-bottom: 15px;">🔄 Refresh</button>
                    <div id="fileList" style="max-height: 400px; overflow-y: auto;">
                        <p style="color: #666;">Loading files...</p>
                    </div>
                </div>
            </div>
            
            <!-- File Editor Modal -->
            <div id="fileEditorModal" class="modal">
                <div class="modal-content">
                    <div class="modal-header">
                        <h3>📝 Edit File: <span id="editingFileName"></span></h3>
                        <button class="modal-close" onclick="closeFileEditor()">&times;</button>
                    </div>
                    <textarea id="fileEditor" class="file-editor" placeholder="File content will appear here..."></textarea>
                    <div style="margin-top: 20px; display: flex; gap: 10px;">
                        <button class="btn btn-primary" onclick="saveFileContent()">💾 Save</button>
                        <button class="btn btn-secondary" onclick="closeFileEditor()">❌ Cancel</button>
                    </div>
                </div>
            </div>
            
            <!-- Firmware Tab -->
            <div id="firmware" class="tab-content">
                <div class="card">
                    <h3>Firmware Update (OTA)</h3>
                    <div class="form-group">
                        <label>Select Firmware File (.bin)</label>
                        <input type="file" id="firmwareFile" accept=".bin">
                    </div>
                    <div id="uploadProgress" style="display: none; margin: 20px 0;">
                        <div style="background: #f0f0f0; border-radius: 10px; overflow: hidden;">
                            <div id="progressBar" style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); height: 30px; width: 0%; transition: width 0.3s; display: flex; align-items: center; justify-content: center; color: white; font-weight: 600;">
                                <span id="progressText">0%</span>
                            </div>
                        </div>
                    </div>
                    <button class="btn btn-primary" onclick="uploadFirmware()">⬆️ Upload Firmware</button>
                    <p style="color: #dc3545; font-size: 14px; margin-top: 15px;">
                        ⚠️ Device will reboot automatically after successful update. Do not power off!
                    </p>
                </div>
            </div>
            
            <!-- Settings Tab -->
            <div id="settings" class="tab-content">
                <div class="card">
                    <h3>Web Interface Settings</h3>
                    <div class="form-group">
                        <label>Username</label>
                        <input type="text" id="webUser" placeholder="admin">
                    </div>
                    <div class="form-group">
                        <label>Password</label>
                        <input type="password" id="webPass" placeholder="New password">
                    </div>
                    <div class="form-group">
                        <label>Authentication Enabled</label>
                        <label class="toggle">
                            <input type="checkbox" id="webAuthEnabled">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <button class="btn btn-primary" onclick="saveWebSettings()">💾 Save Settings</button>
                </div>
            </div>
        </div>
    </div>

    <script>
        console.log('Script loading started');
        let ws;
        
        // Initialize WebSocket connection
        function initWebSocket() {
            console.log('Initializing WebSocket...');
            ws = new WebSocket('ws://' + window.location.hostname + '/ws');
            
            ws.onopen = function() {
                console.log('WebSocket connected');
                refreshStatus();
            };
            
            ws.onmessage = function(event) {
                try {
                    const data = JSON.parse(event.data);
                    handleWebSocketData(data);
                } catch(e) {
                    console.error('WebSocket data parse error:', e);
                }
            };
            
            ws.onclose = function() {
                console.log('WebSocket disconnected');
                setTimeout(initWebSocket, 3000);
            };
        }
        
        function handleWebSocketData(data) {
            if (data.type === 'status') {
                updateSystemStatus(data);
            }
        }
        
        function updateSystemStatus(data) {
            if (data.system) {
                document.getElementById('chipModel').textContent = data.system.chip_model || '-';
                document.getElementById('freeHeap').textContent = (data.system.free_heap || 0) + ' bytes';
                document.getElementById('cpuFreq').textContent = (data.system.cpu_freq || 0) + ' MHz';
                document.getElementById('uptime').textContent = formatUptime(data.system.uptime || 0);
            }
            
            if (data.network) {
                document.getElementById('wifiStatus').textContent = data.network.wifi_status || '-';
                document.getElementById('ethStatus').textContent = data.network.eth_status || '-';
                document.getElementById('ipAddress').textContent = data.network.ip || '-';
                document.getElementById('macAddress').textContent = data.network.mac || '-';
            }
            
            if (data.rtc) {
                document.getElementById('rtcDateTime').textContent = data.rtc.datetime || '-';
                document.getElementById('rtcType').textContent = data.rtc.type || '-';
            }
        }
        
        function formatUptime(seconds) {
            const days = Math.floor(seconds / 86400);
            const hours = Math.floor((seconds % 86400) / 3600);
            const mins = Math.floor((seconds % 3600) / 60);
            return `${days}d ${hours}h ${mins}m`;
        }
        
        function showTab(tabName, element) {
            const tabs = document.querySelectorAll('.tab');
            const contents = document.querySelectorAll('.tab-content');
            
            tabs.forEach(tab => tab.classList.remove('active'));
            contents.forEach(content => content.classList.remove('active'));
            
            if (element) {
                element.classList.add('active');
            } else {
                // Find the tab button by matching onclick attribute
                document.querySelectorAll('.tab').forEach(tab => {
                    if (tab.getAttribute('onclick') && tab.getAttribute('onclick').includes(tabName)) {
                        tab.classList.add('active');
                    }
                });
            }
            document.getElementById(tabName).classList.add('active');
        }
        
        function showAlert(message, type = 'success') {
            const alert = document.getElementById('alert');
            alert.textContent = message;
            alert.className = 'alert ' + type;
            alert.style.display = 'block';
            
            setTimeout(() => {
                alert.style.display = 'none';
            }, 5000);
        }
        
        function toggleStaticIP() {
            const dhcp = document.getElementById('ethDhcp').checked;
            const staticFields = document.getElementById('staticIpFields');
            staticFields.style.display = dhcp ? 'none' : 'block';
        }
        
        // API Calls
        async function apiCall(endpoint, method = 'GET', data = null) {
            try {
                const options = {
                    method: method,
                    headers: { 'Content-Type': 'application/json' }
                };
                
                if (data) {
                    options.body = JSON.stringify(data);
                }
                
                const response = await fetch(endpoint, options);
                const result = await response.json();
                
                if (result.success) {
                    showAlert(result.message || 'Operation successful', 'success');
                } else {
                    showAlert(result.message || 'Operation failed', 'error');
                }
                
                return result;
            } catch (error) {
                showAlert('Network error: ' + error.message, 'error');
                return { success: false };
            }
        }
        
        function refreshStatus() {
            apiCall('/api/status').then(data => {
                if (data.success) updateSystemStatus(data);
            });
        }
        
        function rebootDevice() {
            if (confirm('Are you sure you want to reboot the device?')) {
                apiCall('/api/system/reboot', 'POST');
            }
        }
        
        function factoryReset() {
            if (confirm('⚠️ This will erase ALL settings! Are you sure?')) {
                if (confirm('Last chance! Really factory reset?')) {
                    apiCall('/api/system/factory', 'POST');
                }
            }
        }
        
        function saveWiFiConfig() {
            const data = {
                enabled: document.getElementById('wifiEnabled').checked,
                ssid: document.getElementById('wifiSsid').value,
                password: document.getElementById('wifiPassword').value
            };
            apiCall('/api/wifi/config', 'POST', data);
        }
        
        function saveEthernetConfig() {
            const data = {
                enabled: document.getElementById('ethEnabled').checked,
                dhcp: document.getElementById('ethDhcp').checked,
                ip: document.getElementById('ethIp').value,
                gateway: document.getElementById('ethGateway').value,
                subnet: document.getElementById('ethSubnet').value,
                dns: document.getElementById('ethDns').value
            };
            apiCall('/api/ethernet/config', 'POST', data);
        }
        
        function saveMQTTConfig() {
            const data = {
                host: document.getElementById('mqttHost').value,
                port: parseInt(document.getElementById('mqttPort').value),
                user: document.getElementById('mqttUser').value,
                password: document.getElementById('mqttPass').value
            };
            apiCall('/api/mqtt/config', 'POST', data);
        }
        
        function saveSubtopicConfig() {
            const data = {
                company: document.getElementById('subCompany').value,
                location: document.getElementById('subLocation').value,
                department: document.getElementById('subDepartment').value,
                line: document.getElementById('subLine').value,
                machine: document.getElementById('subMachine').value
            };
            apiCall('/api/subtopic/config', 'POST', data);
        }
        
        function saveHMIConfig() {
            const data = {
                enabled: document.getElementById('hmiEnabled').checked
            };
            apiCall('/api/hmi/config', 'POST', data);
        }
        
        function setRTC() {
            const datetime = document.getElementById('rtcInput').value;
            if (!datetime) {
                showAlert('Please select date/time', 'error');
                return;
            }
            apiCall('/api/rtc/set', 'POST', { datetime: datetime });
        }
        
        function syncRTCBrowser() {
            const now = new Date();
            const datetime = now.toISOString().slice(0, 19).replace('T', ' ');
            apiCall('/api/rtc/set', 'POST', { datetime: datetime });
        }
        
        function scanWiFi() {
            showAlert('Scanning WiFi networks...', 'success');
            apiCall('/api/wifi/scan', 'POST');
        }
        
        async function uploadFirmware() {
            const fileInput = document.getElementById('firmwareFile');
            const file = fileInput.files[0];
            
            if (!file) {
                showAlert('Please select a firmware file', 'error');
                return;
            }
            
            if (!file.name.endsWith('.bin')) {
                showAlert('Please select a .bin file', 'error');
                return;
            }
            
            if (!confirm('Start firmware update? Device will reboot after update.')) {
                return;
            }
            
            const formData = new FormData();
            formData.append('firmware', file);
            
            const progressDiv = document.getElementById('uploadProgress');
            const progressBar = document.getElementById('progressBar');
            const progressText = document.getElementById('progressText');
            
            progressDiv.style.display = 'block';
            
            try {
                const xhr = new XMLHttpRequest();
                
                xhr.upload.addEventListener('progress', (e) => {
                    if (e.lengthComputable) {
                        const percent = Math.round((e.loaded / e.total) * 100);
                        progressBar.style.width = percent + '%';
                        progressText.textContent = percent + '%';
                    }
                });
                
                xhr.addEventListener('load', () => {
                    if (xhr.status === 200) {
                        showAlert('Firmware uploaded successfully! Rebooting...', 'success');
                        setTimeout(() => {
                            window.location.reload();
                        }, 5000);
                    } else {
                        showAlert('Upload failed: ' + xhr.responseText, 'error');
                        progressDiv.style.display = 'none';
                    }
                });
                
                xhr.addEventListener('error', () => {
                    showAlert('Upload error occurred', 'error');
                    progressDiv.style.display = 'none';
                });
                
                xhr.open('POST', '/api/firmware/update');
                xhr.send(formData);
                
            } catch (error) {
                showAlert('Upload error: ' + error.message, 'error');
                progressDiv.style.display = 'none';
            }
        }
        
        function saveWebSettings() {
            const data = {
                username: document.getElementById('webUser').value,
                password: document.getElementById('webPass').value,
                auth_enabled: document.getElementById('webAuthEnabled').checked
            };
            apiCall('/api/web/settings', 'POST', data);
        }
        
        // File Management Functions
        let currentPath = '/';
        
        function navigateToFolder(folderName) {
            if (folderName === '/') {
                currentPath = '/';
            } else if (folderName === '..') {
                const parts = currentPath.split('/').filter(p => p);
                parts.pop();
                currentPath = '/' + parts.join('/');
                if (currentPath === '/') currentPath = '/';
            } else {
                if (currentPath === '/') {
                    currentPath = '/' + folderName;
                } else {
                    currentPath = currentPath + '/' + folderName;
                }
            }
            updateBreadcrumb();
            refreshFileList();
        }
        
        function updateBreadcrumb() {
            const breadcrumb = document.getElementById('breadcrumb');
            if (!breadcrumb) return;
            
            let html = '<span class="breadcrumb-item" onclick="navigateToFolder("/")">📁 Root</span>';
            
            if (currentPath !== '/') {
                const parts = currentPath.split('/').filter(p => p);
                let path = '';
                parts.forEach((part, index) => {
                    path += '/' + part;
                    const fullPath = path;
                    html += '<span class="breadcrumb-separator"> / </span>';
                    html += `<span class="breadcrumb-item" onclick="navigateToPath('${fullPath}')">${part}</span>`;
                });
            }
            
            breadcrumb.innerHTML = html;
        }
        
        function navigateToPath(path) {
            currentPath = path;
            updateBreadcrumb();
            refreshFileList();
        }
        
        async function refreshFileList() {
            const fileList = document.getElementById('fileList');
            if (!fileList) return; // Safety check
            
            try {
                const response = await fetch('/api/files/list?path=' + encodeURIComponent(currentPath));
                const data = await response.json();
                
                if (data.success) {
                    displayFiles(data.files);
                    const fsType = document.getElementById('fsType');
                    const fsTotal = document.getElementById('fsTotal');
                    const fsUsed = document.getElementById('fsUsed');
                    if (fsType && data.fsType) fsType.textContent = data.fsType;
                    if (fsTotal) fsTotal.textContent = formatBytes(data.total);
                    if (fsUsed) fsUsed.textContent = formatBytes(data.used) + ' (' + Math.round((data.used / data.total) * 100) + '%)';
                } else {
                    showAlert('Failed to load files', 'error');
                }
            } catch (error) {
                showAlert('Error loading files: ' + error.message, 'error');
            }
        }
        
        function displayFiles(files) {
            const fileList = document.getElementById('fileList');
            if (!fileList) return; // Safety check
            
            if (!files || files.length === 0) {
                fileList.innerHTML = '<p style="color: #666; padding: 20px; text-align: center;">No files found</p>';
                return;
            }
            
            let html = '';
            
            if (currentPath !== '/') {
                html += `
                    <div class="file-item" onclick="navigateToFolder('..')" style="cursor: pointer; background: #f0f0f0;">
                        <div class="file-info">
                            <span class="file-icon">📁</span>
                            <div>
                                <div class="file-name">..</div>
                                <div class="file-size">Parent Directory</div>
                            </div>
                        </div>
                        <div class="file-actions"></div>
                    </div>
                `;
            }
            
            files.forEach(file => {
                const icon = file.isDir ? '📁' : getFileIcon(file.name);
                const clickHandler = file.isDir ? `onclick="navigateToFolder('${file.name}')" style="cursor: pointer;"` : '';
                html += `
                    <div class="file-item">
                        <div class="file-info" ${clickHandler}>
                            <span class="file-icon">${icon}</span>
                            <div>
                                <div class="file-name">${file.name}</div>
                                <div class="file-size">${file.isDir ? 'Directory' : formatBytes(file.size)}</div>
                            </div>
                        </div>
                        <div class="file-actions">
                            ${!file.isDir ? `<button class="btn-small" style="background: #17a2b8; color: white;" onclick="editFile('${file.name}')">✏️ Edit</button>` : ''}
                            ${!file.isDir ? `<button class="btn-small btn-download" onclick="downloadFile('${file.name}')">⬇️ Download</button>` : ''}
                            <button class="btn-small btn-delete" onclick="deleteFile('${file.name}')">🗑️ Delete</button>
                        </div>
                    </div>
                `;
            });
            
            fileList.innerHTML = html;
        }
        
        function getFileIcon(filename) {
            const ext = filename.split('.').pop().toLowerCase();
            const icons = {
                'txt': '📄', 'json': '📋', 'xml': '📋', 'csv': '📊',
                'jpg': '🖼️', 'jpeg': '🖼️', 'png': '🖼️', 'gif': '🖼️',
                'pdf': '📕', 'doc': '📘', 'docx': '📘',
                'zip': '📦', 'rar': '📦', 'tar': '📦', 'gz': '📦',
                'bin': '⚙️', 'hex': '⚙️',
                'log': '📝', 'ini': '⚙️', 'conf': '⚙️', 'cfg': '⚙️'
            };
            return icons[ext] || '📄';
        }
        
        function formatBytes(bytes) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i];
        }
        
        async function uploadFile() {
            const fileInput = document.getElementById('fileToUpload');
            const pathInput = document.getElementById('uploadPath');
            const file = fileInput.files[0];
            
            if (!file) {
                showAlert('Please select a file', 'error');
                return;
            }
            
            let uploadPath = pathInput.value.trim();
            if (!uploadPath) {
                uploadPath = '/' + file.name;
            }
            if (!uploadPath.startsWith('/')) {
                uploadPath = '/' + uploadPath;
            }
            
            const formData = new FormData();
            formData.append('file', file);
            formData.append('path', uploadPath);
            
            const progressDiv = document.getElementById('fileUploadProgress');
            const progressBar = document.getElementById('fileProgressBar');
            const progressText = document.getElementById('fileProgressText');
            
            progressDiv.style.display = 'block';
            
            try {
                const xhr = new XMLHttpRequest();
                
                xhr.upload.addEventListener('progress', (e) => {
                    if (e.lengthComputable) {
                        const percent = Math.round((e.loaded / e.total) * 100);
                        progressBar.style.width = percent + '%';
                        progressText.textContent = percent + '%';
                    }
                });
                
                xhr.addEventListener('load', () => {
                    progressDiv.style.display = 'none';
                    if (xhr.status === 200) {
                        const response = JSON.parse(xhr.responseText);
                        showAlert(response.message || 'File uploaded successfully', 'success');
                        fileInput.value = '';
                        pathInput.value = '';
                        refreshFileList();
                    } else {
                        showAlert('Upload failed: ' + xhr.responseText, 'error');
                    }
                });
                
                xhr.addEventListener('error', () => {
                    progressDiv.style.display = 'none';
                    showAlert('Upload error occurred', 'error');
                });
                
                xhr.open('POST', '/api/files/upload');
                xhr.send(formData);
                
            } catch (error) {
                progressDiv.style.display = 'none';
                showAlert('Upload error: ' + error.message, 'error');
            }
        }
        
        function downloadFile(filename) {
            const fullPath = currentPath === '/' ? '/' + filename : currentPath + '/' + filename;
            window.location.href = '/api/files/download?path=' + encodeURIComponent(fullPath);
        }
        
        async function deleteFile(filename) {
            if (!confirm('Delete ' + filename + '?')) {
                return;
            }
            
            const fullPath = currentPath === '/' ? '/' + filename : currentPath + '/' + filename;
            
            try {
                const response = await fetch('/api/files/delete?path=' + encodeURIComponent(fullPath), {
                    method: 'DELETE'
                });
                const data = await response.json();
                
                if (data.success) {
                    showAlert('File deleted', 'success');
                    refreshFileList();
                } else {
                    showAlert(data.message || 'Delete failed', 'error');
                }
            } catch (error) {
                showAlert('Error deleting file: ' + error.message, 'error');
            }
        }
        
        async function editFile(filename) {
            const fullPath = currentPath === '/' ? '/' + filename : currentPath + '/' + filename;
            
            try {
                const response = await fetch('/api/files/read?path=' + encodeURIComponent(fullPath));
                const data = await response.json();
                
                if (data.success) {
                    document.getElementById('editingFileName').textContent = filename;
                    document.getElementById('fileEditor').value = data.content;
                    document.getElementById('fileEditor').dataset.filePath = fullPath;
                    document.getElementById('fileEditorModal').classList.add('active');
                } else {
                    showAlert(data.message || 'Failed to load file', 'error');
                }
            } catch (error) {
                showAlert('Error loading file: ' + error.message, 'error');
            }
        }
        
        function closeFileEditor() {
            document.getElementById('fileEditorModal').classList.remove('active');
            document.getElementById('fileEditor').value = '';
        }
        
        async function saveFileContent() {
            const editor = document.getElementById('fileEditor');
            const filePath = editor.dataset.filePath;
            const content = editor.value;
            
            try {
                const response = await fetch('/api/files/write', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ path: filePath, content: content })
                });
                const data = await response.json();
                
                if (data.success) {
                    showAlert('File saved successfully', 'success');
                    closeFileEditor();
                    refreshFileList();
                } else {
                    showAlert(data.message || 'Failed to save file', 'error');
                }
            } catch (error) {
                showAlert('Error saving file: ' + error.message, 'error');
            }
        }
        
        // Load configurations on page load
        async function loadConfigs() {
            // Load WiFi config
            const wifiConfig = await apiCall('/api/wifi/config');
            if (wifiConfig.success) {
                document.getElementById('wifiEnabled').checked = wifiConfig.enabled;
                document.getElementById('wifiSsid').value = wifiConfig.ssid || '';
            }
            
            // Load Ethernet config
            const ethConfig = await apiCall('/api/ethernet/config');
            if (ethConfig.success) {
                document.getElementById('ethEnabled').checked = ethConfig.enabled;
                document.getElementById('ethDhcp').checked = ethConfig.dhcp;
                document.getElementById('ethIp').value = ethConfig.ip || '';
                document.getElementById('ethGateway').value = ethConfig.gateway || '';
                document.getElementById('ethSubnet').value = ethConfig.subnet || '';
                document.getElementById('ethDns').value = ethConfig.dns || '';
                toggleStaticIP();
            }
            
            // Load MQTT config
            const mqttConfig = await apiCall('/api/mqtt/config');
            if (mqttConfig.success) {
                document.getElementById('mqttHost').value = mqttConfig.host || '';
                document.getElementById('mqttPort').value = mqttConfig.port || 1883;
                document.getElementById('mqttUser').value = mqttConfig.user || '';
            }
            
            // Load Subtopic config
            const subConfig = await apiCall('/api/subtopic/config');
            if (subConfig.success) {
                document.getElementById('subCompany').value = subConfig.company || '';
                document.getElementById('subLocation').value = subConfig.location || '';
                document.getElementById('subDepartment').value = subConfig.department || '';
                document.getElementById('subLine').value = subConfig.line || '';
                document.getElementById('subMachine').value = subConfig.machine || '';
            }
            
            // Load HMI config
            const hmiConfig = await apiCall('/api/hmi/config');
            if (hmiConfig.success) {
                document.getElementById('hmiEnabled').checked = hmiConfig.enabled;
            }
        }
        
        // Initialize on page load
        window.addEventListener('load', () => {
            console.log('Page loaded, initializing...');
            try {
                initWebSocket();
                loadConfigs();
                
                // Load file list only if the Files tab elements exist
                if (document.getElementById('fileList')) {
                    refreshFileList();
                }
                
                // Refresh status every 5 seconds
                setInterval(refreshStatus, 5000);
                console.log('Initialization complete');
            } catch (error) {
                console.error('Initialization error:', error);
                alert('Page initialization failed: ' + error.message);
            }
        });
        
        // Log when script finishes loading
        console.log('Script loaded successfully');
    </script>
</body>
</html>
)rawliteral";


// ==================== API HANDLERS ====================

void handleGetStatus(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    DynamicJsonDocument doc(2048);
    doc["success"] = true;
    
    // System info
    JsonObject system = doc.createNestedObject("system");
    system["chip_model"] = ESP.getChipModel();
    system["chip_cores"] = ESP.getChipCores();
    system["cpu_freq"] = ESP.getCpuFreqMHz();
    system["free_heap"] = ESP.getFreeHeap();
    system["flash_size"] = ESP.getFlashChipSize();
    system["uptime"] = millis() / 1000;
    
    // Network info
    JsonObject network = doc.createNestedObject("network");
    
    if (WiFi.status() == WL_CONNECTED) {
        network["wifi_status"] = "Connected";
        network["wifi_ssid"] = WiFi.SSID();
        network["wifi_ip"] = WiFi.localIP().toString();
        network["wifi_rssi"] = WiFi.RSSI();
    } else {
        network["wifi_status"] = "Disconnected";
    }
    
    if (Ethernet.linkStatus() == LinkON) {
        network["eth_status"] = "Connected";
        network["ip"] = Ethernet.localIP().toString();
        network["gateway"] = Ethernet.gatewayIP().toString();
        network["mac"] = getEthernetMACString();
    } else {
        network["eth_status"] = "Disconnected";
        if (WiFi.status() == WL_CONNECTED) {
            network["ip"] = WiFi.localIP().toString();
            network["mac"] = WiFi.macAddress();
        }
    }
    
    // RTC info
    JsonObject rtcObj = doc.createNestedObject("rtc");
    rtcObj["datetime"] = rtc.getDateTime();
    rtcObj["type"] = rtc.isExternalRTCAvailable() ? "External (DS3231)" : "Internal";
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}


void handleWiFiConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (request->method() == HTTP_GET) {
        // Get WiFi config
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["enabled"] = wifiPref.getBool("enabled", false);
        doc["ssid"] = wifiPref.getString("ssid", "");
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }
    else if (request->method() == HTTP_POST) {
        // Set WiFi config
        String body = request->arg("plain");
        DynamicJsonDocument doc(512);
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        wifiPref.end();
        wifiPref.begin("wifi", false);
        
        if (doc.containsKey("enabled")) {
            wifiPref.putBool("enabled", doc["enabled"]);
        }
        if (doc.containsKey("ssid")) {
            wifiPref.putString("ssid", doc["ssid"].as<String>());
        }
        if (doc.containsKey("password")) {
            wifiPref.putString("password", doc["password"].as<String>());
        }
        
        wifiPref.end();
        wifiPref.begin("wifi", true);
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"WiFi config saved\"}");
    }
}


void handleEthernetConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (request->method() == HTTP_GET) {
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["enabled"] = ethernetPref.getBool("enabled", true);
        doc["dhcp"] = ethernetPref.getBool("dhcp", true);
        doc["ip"] = ethernetPref.getString("ip", "");
        doc["gateway"] = ethernetPref.getString("gateway", "");
        doc["subnet"] = ethernetPref.getString("subnet", "");
        doc["dns"] = ethernetPref.getString("dns", "");
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }
    else if (request->method() == HTTP_POST) {
        String body = request->arg("plain");
        DynamicJsonDocument doc(512);
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        
        if (doc.containsKey("enabled")) ethernetPref.putBool("enabled", doc["enabled"]);
        if (doc.containsKey("dhcp")) ethernetPref.putBool("dhcp", doc["dhcp"]);
        if (doc.containsKey("ip")) ethernetPref.putString("ip", doc["ip"].as<String>());
        if (doc.containsKey("gateway")) ethernetPref.putString("gateway", doc["gateway"].as<String>());
        if (doc.containsKey("subnet")) ethernetPref.putString("subnet", doc["subnet"].as<String>());
        if (doc.containsKey("dns")) ethernetPref.putString("dns", doc["dns"].as<String>());
        
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Ethernet config saved. Reconnect to apply.\"}");
    }
}


void handleMQTTConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (request->method() == HTTP_GET) {
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["host"] = mqttPref.getString("host", "");
        doc["port"] = mqttPref.getInt("port", 1883);
        doc["user"] = mqttPref.getString("user", "");
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }
    else if (request->method() == HTTP_POST) {
        String body = request->arg("plain");
        DynamicJsonDocument doc(512);
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        mqttPref.end();
        mqttPref.begin("mqtt", false);
        
        if (doc.containsKey("host")) mqttPref.putString("host", doc["host"].as<String>());
        if (doc.containsKey("port")) mqttPref.putInt("port", doc["port"]);
        if (doc.containsKey("user")) mqttPref.putString("user", doc["user"].as<String>());
        if (doc.containsKey("password")) mqttPref.putString("password", doc["password"].as<String>());
        
        mqttPref.end();
        mqttPref.begin("mqtt", true);
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"MQTT config saved\"}");
    }
}


void handleSubtopicConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    Preferences subPref;
    subPref.begin("subtopic", request->method() == HTTP_POST ? false : true);
    
    if (request->method() == HTTP_GET) {
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["company"] = subPref.getString("company", "");
        doc["location"] = subPref.getString("location", "");
        doc["department"] = subPref.getString("department", "");
        doc["line"] = subPref.getString("line", "");
        doc["machine"] = subPref.getString("machine", "");
        
        String response;
        serializeJson(doc, response);
        subPref.end();
        request->send(200, "application/json", response);
    }
    else if (request->method() == HTTP_POST) {
        String body = request->arg("plain");
        DynamicJsonDocument doc(512);
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            subPref.end();
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        if (doc.containsKey("company")) subPref.putString("company", doc["company"].as<String>());
        if (doc.containsKey("location")) subPref.putString("location", doc["location"].as<String>());
        if (doc.containsKey("department")) subPref.putString("department", doc["department"].as<String>());
        if (doc.containsKey("line")) subPref.putString("line", doc["line"].as<String>());
        if (doc.containsKey("machine")) subPref.putString("machine", doc["machine"].as<String>());
        
        subPref.end();
        subPref.begin("subtopic", true);
        subPref.end();
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Subtopic config saved\"}");
    }
}


void handleHMIConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (request->method() == HTTP_GET) {
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["enabled"] = hmiPref.getBool("enabled", true);
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }
    else if (request->method() == HTTP_POST) {
        String body = request->arg("plain");
        DynamicJsonDocument doc(256);
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        hmiPref.end();
        hmiPref.begin("hmi", false);
        
        if (doc.containsKey("enabled")) {
            hmiPref.putBool("enabled", doc["enabled"]);
        }
        
        hmiPref.end();
        hmiPref.begin("hmi", true);
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"HMI config saved. Reboot required.\"}");
    }
}


void handleRTCSet(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    String body = request->arg("plain");
    DynamicJsonDocument doc(256);
    
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }
    
    String datetime = doc["datetime"].as<String>();
    
    // Parse: YYYY-MM-DD HH:MM:SS or YYYY-MM-DDTHH:MM:SS
    datetime.replace('T', ' ');
    
    int year, month, day, hour, minute, second;
    int parsed = sscanf(datetime.c_str(), "%d-%d-%d %d:%d:%d", 
                       &year, &month, &day, &hour, &minute, &second);
    
    if (parsed != 6) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid datetime format\"}");
        return;
    }
    
    rtc.setDateTime(day, month, year, hour, minute, second);
    
    request->send(200, "application/json", "{\"success\":true,\"message\":\"RTC updated\"}");
}


void handleSystemReboot(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Rebooting...\"}");
    delay(1000);
    ESP.restart();
}


void handleFactoryReset(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    // Clear all preferences
    wifiPref.end();
    wifiPref.begin("wifi", false);
    wifiPref.clear();
    wifiPref.end();
    
    ethernetPref.end();
    ethernetPref.begin("ethernet", false);
    ethernetPref.clear();
    ethernetPref.end();
    
    mqttPref.end();
    mqttPref.begin("mqtt", false);
    mqttPref.clear();
    mqttPref.end();
    
    hmiPref.end();
    hmiPref.begin("hmi", false);
    hmiPref.clear();
    hmiPref.end();
    
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Factory reset complete. Rebooting...\"}");
    delay(1000);
    ESP.restart();
}


void handleFirmwareUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!checkAuthentication(request)) {
        return;
    }
    
    if (!index) {
        Serial.println("[OTA] Update started");
        
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            return;
        }
    }
    
    if (Update.write(data, len) != len) {
        Update.printError(Serial);
        return;
    }
    
    if (final) {
        if (Update.end(true)) {
            Serial.println("[OTA] Update complete");
            request->send(200, "application/json", "{\"success\":true,\"message\":\"Update complete. Rebooting...\"}");
            delay(1000);
            ESP.restart();
        } else {
            Update.printError(Serial);
            request->send(500, "application/json", "{\"success\":false,\"message\":\"Update failed\"}");
        }
    }
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char*)data;
        
        // Handle WebSocket commands
        if (message == "ping") {
            ws.textAll("pong");
        }
        else if (message == "status") {
            String status = getSystemStatusJSON();
            ws.textAll(status);
        }
    }
}


void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] Client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}


String getSystemStatusJSON() {
    DynamicJsonDocument doc(1024);
    doc["type"] = "status";
    
    JsonObject system = doc.createNestedObject("system");
    system["chip_model"] = ESP.getChipModel();
    system["free_heap"] = ESP.getFreeHeap();
    system["cpu_freq"] = ESP.getCpuFreqMHz();
    system["uptime"] = millis() / 1000;
    
    JsonObject network = doc.createNestedObject("network");
    if (WiFi.status() == WL_CONNECTED) {
        network["wifi_status"] = "Connected";
        network["ip"] = WiFi.localIP().toString();
        network["mac"] = WiFi.macAddress();
    } else if (Ethernet.linkStatus() == LinkON) {
        network["eth_status"] = "Connected";
        network["ip"] = Ethernet.localIP().toString();
        network["mac"] = getEthernetMACString();
    }
    
    JsonObject rtcObj = doc.createNestedObject("rtc");
    rtcObj["datetime"] = rtc.getDateTime();
    rtcObj["type"] = rtc.isExternalRTCAvailable() ? "External" : "Internal";
    
    String output;
    serializeJson(doc, output);
    return output;
}


// ==================== FILE MANAGEMENT HANDLERS ====================

void handleFileList(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (!filesystemMounted || !fsManagerFFat.isFilesystemMounted()) {
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Filesystem not mounted\"}");
        return;
    }
    
    String path = request->hasArg("path") ? request->arg("path") : "/";
    if (!path.startsWith("/")) path = "/" + path;
    
    DynamicJsonDocument doc(4096);
    doc["success"] = true;
    doc["total"] = fsManagerFFat.totalBytes();
    doc["used"] = fsManagerFFat.usedBytes();
    doc["free"] = fsManagerFFat.freeBytes();
    doc["fsType"] = fsManagerFFat.getFilesystemName();
    doc["currentPath"] = path;
    
    JsonArray files = doc.createNestedArray("files");
    
    fs::FS *fs = fsManagerFFat.getActiveFilesystem();
    if (!fs) {
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Filesystem not available\"}");
        return;
    }
    
    File root = fs->open(path);
    if (!root) {
        request->send(404, "application/json", "{\"success\":false,\"message\":\"Directory not found\"}");
        return;
    }
    
    if (!root.isDirectory()) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Path is not a directory\"}");
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        JsonObject fileObj = files.createNestedObject();
        fileObj["name"] = String(file.name());
        fileObj["size"] = file.size();
        fileObj["isDir"] = file.isDirectory();
        file = root.openNextFile();
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}


void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!checkAuthentication(request)) return;
    
    if (!filesystemMounted || !fsManagerFFat.isFilesystemMounted()) {
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Filesystem not mounted\"}");
        return;
    }
    
    static File uploadFile;
    static String uploadPath;
    
    if (!index) {
        // Start of upload
        uploadPath = request->hasArg("path") ? request->arg("path") : "/" + filename;
        
        Serial.printf("[FS] Upload start: %s\n", uploadPath.c_str());
        
        if (fsManagerFFat.search(uploadPath)) {
            fsManagerFFat.deleteFile(uploadPath);
        }
        
        fs::FS *fs = fsManagerFFat.getActiveFilesystem();
        if (!fs) {
            request->send(500, "application/json", "{\"success\":false,\"message\":\"Filesystem not available\"}");
            return;
        }
        
        uploadFile = fs->open(uploadPath, FILE_WRITE);
        if (!uploadFile) {
            Serial.println("[FS] Failed to open file for writing");
            request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to create file\"}");
            return;
        }
    }
    
    if (uploadFile) {
        if (uploadFile.write(data, len) != len) {
            Serial.println("[FS] Write failed");
        }
    }
    
    if (final) {
        if (uploadFile) {
            uploadFile.close();
            Serial.printf("[FS] Upload complete: %s (%d bytes)\n", uploadPath.c_str(), index + len);
        }
        request->send(200, "application/json", "{\"success\":true,\"message\":\"File uploaded successfully\"}");
    }
}


void handleFileDownload(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (!filesystemMounted || !fsManagerFFat.isFilesystemMounted()) {
        request->send(500, "text/plain", "Filesystem not mounted");
        return;
    }
    
    if (!request->hasArg("path")) {
        request->send(400, "text/plain", "Missing path parameter");
        return;
    }
    
    String path = request->arg("path");
    if (!path.startsWith("/")) {
        path = "/" + path;
    }
    
    if (!fsManagerFFat.search(path)) {
        request->send(404, "text/plain", "File not found");
        return;
    }
    
    fs::FS *fs = fsManagerFFat.getActiveFilesystem();
    if (!fs) {
        request->send(500, "text/plain", "Filesystem not available");
        return;
    }
    
    File file = fs->open(path, FILE_READ);
    if (!file) {
        request->send(500, "text/plain", "Failed to open file");
        return;
    }
    
    String filename = path.substring(path.lastIndexOf('/') + 1);
    request->send(*fs, path, "application/octet-stream", true);
    
    Serial.printf("[FS] Download: %s\n", path.c_str());
}


void handleFileDelete(AsyncWebServerRequest *request) {
    Serial.println("[FS] handleFileDelete() called");
    
    if (!checkAuthentication(request)) {
        Serial.println("[FS] Authentication failed");
        return;
    }
    
    Serial.println("[FS] Authentication passed");
    
    if (!filesystemMounted || !fsManagerFFat.isFilesystemMounted()) {
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Filesystem not mounted\"}");
        return;
    }
    
    if (!request->hasArg("path")) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing path parameter\"}");
        return;
    }
    
    String path = request->arg("path");
    if (!path.startsWith("/")) {
        path = "/" + path;
    }
    
    Serial.printf("[FS] Delete request: %s\n", path.c_str());
    
    if (!fsManagerFFat.search(path)) {
        Serial.printf("[FS] Delete failed - not found: %s\n", path.c_str());
        request->send(404, "application/json", "{\"success\":false,\"message\":\"File not found\"}");
        return;
    }
    
    // Check if it's a directory
    fs::FS *fs = fsManagerFFat.getActiveFilesystem();
    if (fs) {
        File file = fs->open(path);
        if (file) {
            bool isDir = file.isDirectory();
            file.close();
            
            bool deleted = false;
            if (isDir) {
                Serial.printf("[FS] Deleting directory: %s\n", path.c_str());
                deleted = fsManagerFFat.deleteDir(path);
            } else {
                Serial.printf("[FS] Deleting file: %s\n", path.c_str());
                deleted = fsManagerFFat.deleteFile(path);
            }
            
            if (deleted) {
                Serial.printf("[FS] Deleted successfully: %s\n", path.c_str());
                request->send(200, "application/json", "{\"success\":true,\"message\":\"Deleted successfully\"}");
            } else {
                Serial.printf("[FS] Delete operation failed: %s\n", path.c_str());
                request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to delete\"}");
            }
            return;
        }
    }
    
    // Fallback: try file delete
    if (fsManagerFFat.deleteFile(path)) {
        Serial.printf("[FS] Deleted: %s\n", path.c_str());
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Deleted successfully\"}");
    } else {
        Serial.printf("[FS] Delete failed: %s\n", path.c_str());
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to delete\"}");
    }
}


void handleFileRead(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (!filesystemMounted || !fsManagerFFat.isFilesystemMounted()) {
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Filesystem not mounted\"}");
        return;
    }
    
    if (!request->hasArg("path")) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing path parameter\"}");
        return;
    }
    
    String path = request->arg("path");
    if (!path.startsWith("/")) {
        path = "/" + path;
    }
    
    String content = fsManagerFFat.readFile(path);
    
    if (content.length() == 0 && !fsManagerFFat.search(path)) {
        request->send(404, "application/json", "{\"success\":false,\"message\":\"File not found\"}");
        return;
    }
    
    DynamicJsonDocument doc(8192);
    doc["success"] = true;
    doc["content"] = content;
    doc["path"] = path;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
    
    Serial.printf("[FS] Read: %s (%d bytes)\n", path.c_str(), content.length());
}


void handleFileWrite(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (!filesystemMounted || !fsManagerFFat.isFilesystemMounted()) {
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Filesystem not mounted\"}");
        return;
    }
    
    String body = request->arg("plain");
    DynamicJsonDocument doc(8192);
    
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc.containsKey("path") || !doc.containsKey("content")) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing path or content\"}");
        return;
    }
    
    String path = doc["path"].as<String>();
    String content = doc["content"].as<String>();
    
    if (!path.startsWith("/")) {
        path = "/" + path;
    }
    
    if (fsManagerFFat.writeFile(path, content.c_str())) {
        Serial.printf("[FS] Written: %s (%d bytes)\n", path.c_str(), content.length());
        request->send(200, "application/json", "{\"success\":true,\"message\":\"File saved successfully\"}");
    } else {
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to write file\"}");
    }
}


// ==================== SETUP WEB SERVER ====================

void setupWebServer() {
    if (webServerStarted) return;
    
    // Serve main page
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!checkAuthentication(request)) return;
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html);
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    });
    
    // Initialize filesystem
    initFilesystem();
    
    // API Routes
    webServer.on("/api/status", HTTP_GET, handleGetStatus);
    webServer.on("/api/wifi/config", HTTP_ANY, handleWiFiConfig);
    webServer.on("/api/ethernet/config", HTTP_ANY, handleEthernetConfig);
    webServer.on("/api/mqtt/config", HTTP_ANY, handleMQTTConfig);
    webServer.on("/api/subtopic/config", HTTP_ANY, handleSubtopicConfig);
    webServer.on("/api/hmi/config", HTTP_ANY, handleHMIConfig);
    webServer.on("/api/rtc/set", HTTP_POST, handleRTCSet);
    webServer.on("/api/system/reboot", HTTP_POST, handleSystemReboot);
    webServer.on("/api/system/factory", HTTP_POST, handleFactoryReset);
    
    // File management routes
    webServer.on("/api/files/list", HTTP_GET, handleFileList);
    webServer.on("/api/files/read", HTTP_GET, handleFileRead);
    webServer.on("/api/files/write", HTTP_POST, handleFileWrite);
    webServer.on("/api/files/download", HTTP_GET, handleFileDownload);
    webServer.on("/api/files/delete", HTTP_ANY, handleFileDelete);
    
    // Firmware update handler
    webServer.on("/api/firmware/update", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200);
        },
        handleFirmwareUpdate
    );
    
    // File upload handler
    webServer.on("/api/files/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            request->send(200);
        },
        handleFileUpload
    );
    
    // WebSocket
    ws.onEvent(onWebSocketEvent);
    webServer.addHandler(&ws);
    
    // 404 handler
    webServer.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "application/json", "{\"success\":false,\"message\":\"Not found\"}");
    });
    
    // Enable CORS
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    webServer.begin();
    
    
    Serial.println("[Web] Server started");
    Serial.print("[Web] Access at: http://");
    
    if (Ethernet.linkStatus() == LinkON) {
        webServerStarted = true;
        Serial.println(Ethernet.localIP());
        Serial.printf("[Web] Ethernet MAC: %s\n", getEthernetMACString().c_str());
    } else if (WiFi.status() == WL_CONNECTED) {
        webServerStarted = true;
        Serial.println(WiFi.localIP());
        Serial.printf("[Web] WiFi IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        webServerStarted = false;
        Serial.println("(waiting for network)");
    }
}


// ==================== LOOP HANDLER ====================

void handleWebServer() {
    if (!webServerStarted) {
        // Only start if network is available
        if (Ethernet.linkStatus() == LinkON) {
            Serial.println("[Web] Ethernet link detected, starting server...");
            setupWebServer();
        } else if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[Web] WiFi connected, starting server...");
            setupWebServer();
        }
    }
    
    // Note: cleanupClients() not available in this AsyncWebSocket version
}


// ==================== BROADCAST STATUS ====================

void broadcastStatusToWebClients() {
    if (webServerStarted && ws.count() > 0) {
        String status = getSystemStatusJSON();
        ws.textAll(status);
    }
}

#endif // WEB_CONFIGURATION_H
