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
#include "MQTT_Lib.h"
#include "mbedtls/base64.h"
#include "esp_partition.h"

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
extern Preferences rs485ModbusPref;
extern Preferences serialportPref;
extern Preferences shiftPref;
extern Preferences settingsPref;
extern bool rs485ModbusEnabled;
extern bool serialportEnabled;
extern bool shiftEnabled;
extern uint8_t shiftNumShifts;
extern bool usbScannerEnabled;
extern String lastResetReason;
extern RTCManager rtc;
extern FilesystemManager fsManagerFFat;
extern MQTT_Lib mqtt_obj;

// Web server on port 80
AsyncWebServer webServer(80);
EthernetServer ethWebServer(80);
AsyncWebSocket ws("/ws");

// Authentication credentials (store in preferences in production)
String web_username = "admin";
String web_password = "admin123";
bool web_auth_enabled = true;

// Server state
bool webServerStarted = false;
bool ethWebServerStarted = false;

// Forward declarations
void setupWebServer();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
String getSystemStatusJSON();
String getNetworkStatusJSON();

// Ethernet web server forward declarations
void setupEthWebServer();
void handleEthWebClients();
void handleEthWeb();

// Helper function to get Ethernet MAC as String
String getEthernetMACString() {
    uint8_t mac[6];
    Ethernet.MACAddress(mac);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
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
        
        .page-body {
            display: flex;
            align-items: flex-start;
        }
        .tabs {
            display: flex;
            flex-direction: column;
            width: 160px;
            min-width: 160px;
            background: #f5f5f5;
            border-right: 2px solid #ddd;
            padding: 8px 0;
            position: sticky;
            top: 0;
            max-height: 100vh;
            overflow-y: auto;
        }
        .tab {
            padding: 12px 18px;
            cursor: pointer;
            border: none;
            background: transparent;
            font-size: 13px;
            color: #555;
            transition: all 0.2s;
            white-space: nowrap;
            text-align: left;
            border-left: 3px solid transparent;
        }
        .tab:hover { background: #e8e8e8; color: #333; }
        .tab.active {
            background: white;
            color: #667eea;
            border-left: 3px solid #667eea;
            font-weight: 600;
        }
        
        .content { padding: 30px; flex: 1; min-width: 0; }
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
            .page-body { flex-direction: column; }
            .tabs { flex-direction: row; width: 100%; min-width: unset; border-right: none; border-bottom: 2px solid #ddd; overflow-x: auto; overflow-y: hidden; padding: 0; position: static; max-height: none; }
            .tab { border-left: none; border-bottom: 3px solid transparent; }
            .tab.active { border-left: none; border-bottom: 3px solid #667eea; }
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
        
        <div class="page-body">
        <div class="tabs">
            <button class="tab active" onclick="showTab('system', this)">📊 System</button>
            <button class="tab" onclick="showTab('network', this)">🌐 Network</button>
            <button class="tab" onclick="showTab('mqtt', this)">📡 MQTT</button>
            <button class="tab" onclick="showTab('rtc', this)">🕐 RTC</button>
            <button class="tab" onclick="showTab('hmi', this)">🖥️ HMI</button>
            <button class="tab" onclick="showTab('rs485modbus', this)">🔌 RS485</button>
            <button class="tab" onclick="showTab('serialport', this)">🔗 Serial Port</button>
            <button class="tab" onclick="showTab('shifts', this)">🕒 Shifts</button>
            <button class="tab" onclick="showTab('io', this)">⚡ IO</button>
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
                    <button class="btn btn-danger" onclick="factoryReset()">&#9888;&#65039; Factory Reset</button>
                </div>

                <div class="card">
                    <h3>Reset Information</h3>
                    <div class="info-grid">
                        <div class="info-item">
                            <label>Last Reset Reason</label>
                            <div class="value" id="resetReason">-</div>
                        </div>
                        <div class="info-item">
                            <label>Total Boots</label>
                            <div class="value" id="totalBoots">-</div>
                        </div>
                    </div>
                    <table style="width:100%;margin-top:12px;border-collapse:collapse;font-size:14px;">
                        <thead>
                            <tr style="background:rgba(102,126,234,0.1);">
                                <th style="padding:6px 10px;text-align:left;">Reason</th>
                                <th style="padding:6px 10px;text-align:right;">Count</th>
                            </tr>
                        </thead>
                        <tbody id="resetCountersBody"></tbody>
                    </table>
                </div>

                <div class="card">
                    <h3>Partition Table</h3>
                    <div style="overflow-x:auto;">
                        <table style="width:100%;border-collapse:collapse;font-size:13px;font-family:monospace;">
                            <thead>
                                <tr style="background:rgba(102,126,234,0.1);">
                                    <th style="padding:6px 8px;text-align:left;">Label</th>
                                    <th style="padding:6px 8px;text-align:left;">Type</th>
                                    <th style="padding:6px 8px;text-align:left;">SubType</th>
                                    <th style="padding:6px 8px;text-align:right;">Address</th>
                                    <th style="padding:6px 8px;text-align:right;">Size</th>
                                </tr>
                            </thead>
                            <tbody id="partitionTableBody"></tbody>
                        </table>
                    </div>
                    <div style="margin-top:10px;font-size:13px;color:#888;" id="flashInfo">-</div>
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
                    <div class="form-group">
                        <label>DHCP Mode</label>
                        <label class="toggle">
                            <input type="checkbox" id="wifiDhcp" onchange="toggleWiFiStaticIP()" checked>
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div id="wifiStaticIpFields" style="display:none;">
                        <div class="form-group">
                            <label>IP Address</label>
                            <input type="text" id="wifiIp" placeholder="192.168.1.100">
                        </div>
                        <div class="form-group">
                            <label>Gateway</label>
                            <input type="text" id="wifiGateway" placeholder="192.168.1.1">
                        </div>
                        <div class="form-group">
                            <label>Subnet Mask</label>
                            <input type="text" id="wifiSubnet" placeholder="255.255.255.0">
                        </div>
                        <div class="form-group">
                            <label>DNS Server</label>
                            <input type="text" id="wifiDns" placeholder="8.8.8.8">
                        </div>
                    </div>
                    <div id="wifiLiveCard" style="background:#f8f9fa;border-radius:6px;padding:12px 14px;margin-bottom:16px;border-left:4px solid #ccc;">
                        <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap;">
                            <span style="font-weight:600;font-size:13px;color:#555;">Live Status</span>
                            <span id="wifiLiveBadge" class="status disconnected">Disconnected</span>
                            <button class="btn btn-secondary" style="padding:4px 12px;font-size:12px;margin:0;" onclick="loadNetworkStatus()">🔄 Refresh</button>
                        </div>
                        <div id="wifiLiveDetails" style="display:none;margin-top:10px;">
                            <div class="info-grid">
                                <div class="info-item"><label>IP Address</label><div class="value" id="wifiLiveIp">-</div></div>
                                <div class="info-item"><label>Subnet Mask</label><div class="value" id="wifiLiveSubnet">-</div></div>
                                <div class="info-item"><label>Gateway</label><div class="value" id="wifiLiveGw">-</div></div>
                                <div class="info-item"><label>Signal (RSSI)</label><div class="value" id="wifiLiveRssi">-</div></div>
                            </div>
                        </div>
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
                    <div id="ethLiveCard" style="background:#f8f9fa;border-radius:6px;padding:12px 14px;margin-bottom:16px;border-left:4px solid #ccc;">
                        <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap;">
                            <span style="font-weight:600;font-size:13px;color:#555;">Live Status</span>
                            <span id="ethLiveBadge" class="status disconnected">Disconnected</span>
                        </div>
                        <div id="ethLiveDetails" style="display:none;margin-top:10px;">
                            <div class="info-grid">
                                <div class="info-item"><label>IP Address</label><div class="value" id="ethLiveIp">-</div></div>
                                <div class="info-item"><label>Subnet Mask</label><div class="value" id="ethLiveSubnet">-</div></div>
                                <div class="info-item"><label>Gateway</label><div class="value" id="ethLiveGw">-</div></div>
                                <div class="info-item"><label>MAC Address</label><div class="value" id="ethLiveMac">-</div></div>
                            </div>
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
                        <label>Enable MQTT</label>
                        <label class="toggle">
                            <input type="checkbox" id="mqttEnabled">
                            <span class="slider"></span>
                        </label>
                    </div>
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
                    <div class="form-group">
                        <label>Transport</label>
                        <select id="mqttTransport">
                            <option value="auto">Auto (Ethernet preferred, fallback WiFi)</option>
                            <option value="wifi">WiFi only</option>
                            <option value="ethernet">Ethernet only</option>
                        </select>
                    </div>
                    <div id="mqttLiveCard" style="background:#f8f9fa;border-radius:6px;padding:12px 14px;margin-bottom:16px;border-left:4px solid #ccc;">
                        <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap;">
                            <span style="font-weight:600;font-size:13px;color:#555;">Live Status</span>
                            <span id="mqttLiveBadge" class="status disconnected">Disconnected</span>
                            <button class="btn btn-secondary" style="padding:4px 12px;font-size:12px;margin:0;" onclick="loadMqttStatus()">🔄 Refresh</button>
                        </div>
                        <div id="mqttLiveDetails" style="display:none;margin-top:10px;">
                            <div class="info-grid">
                                <div class="info-item"><label>Broker</label><div class="value" id="mqttLiveBroker">-</div></div>
                                <div class="info-item"><label>Port</label><div class="value" id="mqttLivePort">-</div></div>
                            </div>
                        </div>
                        <div id="mqttLiveReason" style="display:none;margin-top:8px;font-size:13px;color:#721c24;"></div>
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
            
            <!-- RS485 Modbus Tab -->
            <div id="rs485modbus" class="tab-content">
                <div class="card">
                    <h3>RS485 Modbus RTU Configuration</h3>
                    <div class="form-group">
                        <label>RS485 Modbus Enabled</label>
                        <label class="toggle">
                            <input type="checkbox" id="rs485ModbusEnabled">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div class="form-group">
                        <label>Status</label>
                        <div class="value" id="rs485ModbusRunning">-</div>
                    </div>
                    <div class="form-group">
                        <label>Baud Rate</label>
                        <select id="rs485Baudrate">
                            <option value="2400">2400</option>
                            <option value="4800">4800</option>
                            <option value="9600" selected>9600</option>
                            <option value="19200">19200</option>
                            <option value="38400">38400</option>
                            <option value="57600">57600</option>
                            <option value="115200">115200</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Data Bits</label>
                        <select id="rs485Databits">
                            <option value="5">5</option>
                            <option value="6">6</option>
                            <option value="7">7</option>
                            <option value="8" selected>8</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Parity</label>
                        <select id="rs485Parity">
                            <option value="N" selected>None</option>
                            <option value="E">Even</option>
                            <option value="O">Odd</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Stop Bits</label>
                        <select id="rs485Stopbits">
                            <option value="1" selected>1</option>
                            <option value="2">2</option>
                        </select>
                    </div>
                    <p style="color: #666; font-size: 14px; margin-top: 10px;">
                        ⚠️ Changes require device reboot to take effect
                    </p>
                    <button class="btn btn-primary" onclick="saveRS485ModbusConfig()">💾 Save RS485 Modbus Config</button>
                </div>
            </div>

            <!-- Serial Port (UART2) Tab -->
            <div id="serialport" class="tab-content">
                <div class="card">
                    <h3>Serial Port (UART2) Configuration</h3>
                    <p style="color:#e67e22;font-size:13px;margin-bottom:15px;">
                        ⚠️ UART2 is shared with RS485 Modbus RTU. Enable only one at a time.
                        Pins: TX=TX2_PIN, RX=RX2_PIN
                    </p>
                    <div class="form-group">
                        <label>Serial Port Enabled</label>
                        <label class="toggle">
                            <input type="checkbox" id="serialportEnabled">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div class="form-group">
                        <label>Running Status</label>
                        <div class="value" id="serialportRunning">-</div>
                    </div>
                    <div class="form-group">
                        <label>Baud Rate</label>
                        <select id="serialportBaudrate">
                            <option value="2400">2400</option>
                            <option value="4800">4800</option>
                            <option value="9600" selected>9600</option>
                            <option value="19200">19200</option>
                            <option value="38400">38400</option>
                            <option value="57600">57600</option>
                            <option value="115200">115200</option>
                            <option value="230400">230400</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Data Bits</label>
                        <select id="serialportDatabits">
                            <option value="5">5</option>
                            <option value="6">6</option>
                            <option value="7">7</option>
                            <option value="8" selected>8</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Parity</label>
                        <select id="serialportParity">
                            <option value="N" selected>None</option>
                            <option value="E">Even</option>
                            <option value="O">Odd</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Stop Bits</label>
                        <select id="serialportStopbits">
                            <option value="1" selected>1</option>
                            <option value="2">2</option>
                        </select>
                    </div>
                    <p style="color: #666; font-size: 14px; margin-top: 10px;">
                        ⚠️ Changes require device reboot to take effect
                    </p>
                    <button class="btn btn-primary" onclick="saveSerialPortConfig()">💾 Save Serial Port Config</button>
                </div>
            </div>

            <!-- Shifts Tab -->
            <div id="shifts" class="tab-content">
                <div class="card">
                    <h3>🕒 Shift Details Configuration</h3>
                    <div class="form-group">
                        <label>Shift Tracking Enabled</label>
                        <label class="toggle">
                            <input type="checkbox" id="shiftEnabled" onchange="onShiftEnableChange()">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div id="shift_main_fields">
                        <div class="form-group">
                            <label>Number of Shifts</label>
                            <select id="shiftNum" onchange="renderShiftForms()">
                                <option value="1">1 Shift</option>
                                <option value="2">2 Shifts</option>
                                <option value="3">3 Shifts</option>
                            </select>
                        </div>
                        <div id="shiftForms"></div>
                    </div>
                    <p style="color:#666;font-size:14px;margin-top:10px;">
                        ⚠️ Time format: HH:MM:SS &nbsp;|&nbsp; Leave break fields empty if unused.
                    </p>
                    <button class="btn btn-primary" onclick="saveShiftConfig()">💾 Save Shift Config</button>
                </div>
            </div>

            <!-- IO Tab -->
            <div id="io" class="tab-content">
                <div class="card">
                    <h3>IO Peripheral Configuration</h3>
                    <div class="form-group">
                        <label>Input Expander (PCF8574)</label>
                        <label class="toggle">
                            <input type="checkbox" id="ioInputEnabled">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div class="form-group">
                        <label>Output Expander (PCF8574)</label>
                        <label class="toggle">
                            <input type="checkbox" id="ioOutputEnabled">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div class="form-group">
                        <label>USB Scanner (ESP32-S3)</label>
                        <label class="toggle">
                            <input type="checkbox" id="ioUsbEnabled">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <p style="color: #888; font-size: 13px; margin-top: 10px;">&#9888;&#65039; Changes take effect after reboot.</p>
                    <button class="btn btn-primary" onclick="saveIOConfig()">&#128190; Save IO Config</button>
                </div>
            </div>

            <!-- Files Tab -->
            <div id="files" class="tab-content">
                <div class="card">
                    <h3>Filesystem Settings</h3>
                    <div class="form-group">
                        <label>Enable Filesystem (FFat)</label>
                        <label class="toggle">
                            <input type="checkbox" id="fsEnabled" onchange="document.getElementById('fileManagerSection').style.display = this.checked ? 'block' : 'none'">
                            <span class="slider"></span>
                        </label>
                    </div>
                    <div style="display: flex; gap: 10px; margin-top: 10px; flex-wrap: wrap;">
                        <button class="btn btn-primary" onclick="saveFilesystemConfig()">&#128190; Save</button>
                        <button class="btn btn-danger" onclick="formatFilesystem()">&#9888;&#65039; Format Filesystem</button>
                    </div>
                    <p style="color: #888; font-size: 13px; margin-top: 10px;">Enable/disable change takes effect after reboot. Format immediately erases all files!</p>
                </div>

                <div id="fileManagerSection" style="display:none;">

                <div class="card">
                    <h3>File Manager</h3>
                    <div style="display: flex; align-items: center; gap: 12px; flex-wrap: wrap;">
                        <span style="font-weight: 600; color: #667eea;" id="fsType">FFat</span>
                        <div style="flex: 1; min-width: 200px; background: #e9ecef; border-radius: 8px; overflow: hidden; height: 24px; position: relative;">
                            <div id="fsProgressBar" style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); height: 100%; width: 0%; transition: width 0.5s; border-radius: 8px;"></div>
                            <span id="fsProgressText" style="position: absolute; top: 0; left: 0; right: 0; bottom: 0; display: flex; align-items: center; justify-content: center; font-size: 12px; font-weight: 600; color: #333;">-</span>
                        </div>
                        <span style="font-size: 13px; color: #666; white-space: nowrap;"><span id="fsUsed">-</span> / <span id="fsTotal">-</span></span>
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

                </div><!-- end fileManagerSection -->
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
        </div><!-- end page-body -->
    </div>

    <script>
        console.log('Script loading started');
        let ws;
        let isAsyncServer = false;
        
        // Initialize WebSocket connection
        function initWebSocket() {
            console.log('Initializing WebSocket...');
            ws = new WebSocket('ws://' + window.location.hostname + '/ws');
            
            ws.onopen = function() {
                console.log('WebSocket connected');
                isAsyncServer = true;
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
                _applyNetworkLiveStatus(data.network);
            }
            
            if (data.rtc) {
                document.getElementById('rtcDateTime').textContent = data.rtc.datetime || '-';
                document.getElementById('rtcType').textContent = data.rtc.type || '-';
            }

            if (data.mqtt) _applyMqttLiveStatus(data.mqtt);

            if (data.reset) {
                document.getElementById('resetReason').textContent = data.reset.reason || '-';
                document.getElementById('totalBoots').textContent  = data.reset.total_boots || 0;
                const tbody = document.getElementById('resetCountersBody');
                if (tbody) {
                    tbody.innerHTML = '';
                    const labels = {poweron:'Power-on', sw:'Software', panic:'Panic', int_wdt:'Int WDT',
                                    task_wdt:'Task WDT', wdt:'Other WDT', brownout:'Brownout',
                                    ext:'External', deepsleep:'Deep Sleep', sdio:'SDIO', unknown:'Unknown'};
                    const counters = data.reset.counters || {};
                    Object.entries(labels).forEach(([k, v]) => {
                        const c = counters[k] || 0;
                        if (c > 0) {
                            tbody.innerHTML += `<tr style="border-bottom:1px solid #eee;"><td style="padding:5px 10px;">${v}</td><td style="padding:5px 10px;text-align:right;">${c}</td></tr>`;
                        }
                    });
                    if (tbody.innerHTML === '') {
                        tbody.innerHTML = '<tr><td colspan="2" style="padding:8px 10px;color:#888;">No data</td></tr>';
                    }
                }
            }

            if (data.partitions) {
                const tbody = document.getElementById('partitionTableBody');
                if (tbody) {
                    tbody.innerHTML = '';
                    data.partitions.forEach(p => {
                        const addr  = '0x' + (p.address >>> 0).toString(16).padStart(6, '0').toUpperCase();
                        const kb    = (p.size / 1024).toFixed(0);
                        const mb    = (p.size / 1048576);
                        const sz    = p.size >= 1048576
                            ? `${mb % 1 === 0 ? mb.toFixed(0) : mb.toFixed(2)} MB (${kb} KB)`
                            : `${kb} KB`;
                        tbody.innerHTML += `<tr style="border-bottom:1px solid #eee;">
                            <td style="padding:4px 8px;">${p.label}</td>
                            <td style="padding:4px 8px;">${p.type}</td>
                            <td style="padding:4px 8px;">${p.subtype}</td>
                            <td style="padding:4px 8px;text-align:right;">${addr}</td>
                            <td style="padding:4px 8px;text-align:right;">${sz}</td></tr>`;
                    });
                }
                const flashEl = document.getElementById('flashInfo');
                if (flashEl && data.flash_size) {
                    flashEl.textContent = `Flash: ${(data.flash_size/1024/1024).toFixed(0)} MB  |  Speed: ${data.flash_speed} MHz`;
                }
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
            
            // Auto-refresh when specific tabs open
            if (tabName === 'files')   { refreshFileList(); }
            if (tabName === 'network') { loadNetworkStatus(); }
            if (tabName === 'mqtt')    { loadMqttStatus(); }
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
            document.getElementById('staticIpFields').style.display = dhcp ? 'none' : 'block';
        }

        function toggleWiFiStaticIP() {
            const dhcp = document.getElementById('wifiDhcp').checked;
            document.getElementById('wifiStaticIpFields').style.display = dhcp ? 'none' : 'block';
        }

        function isValidIP(ip) {
            const parts = ip.split('.');
            if (parts.length !== 4) return false;
            return parts.every(p => {
                const n = parseInt(p, 10);
                return p !== '' && !isNaN(n) && n >= 0 && n <= 255 && String(n) === p;
            });
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
                const text = await response.text();
                let result;
                try {
                    result = JSON.parse(text);
                } catch (e) {
                    showAlert('Server error (HTTP ' + response.status + ')', 'error');
                    return { success: false };
                }
                
                if (result.success) {
                    if (result.message) showAlert(result.message, 'success');
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
            const dhcp = document.getElementById('wifiDhcp').checked;
            if (!dhcp) {
                const fields = [
                    { id: 'wifiIp', label: 'IP Address' },
                    { id: 'wifiGateway', label: 'Gateway' },
                    { id: 'wifiSubnet', label: 'Subnet Mask' },
                    { id: 'wifiDns', label: 'DNS Server' }
                ];
                for (const f of fields) {
                    const val = document.getElementById(f.id).value.trim();
                    if (!isValidIP(val)) {
                        showAlert('Invalid ' + f.label + ': "' + val + '" - use format 192.168.1.1', 'error');
                        document.getElementById(f.id).focus();
                        return;
                    }
                }
            }
            const data = {
                enabled: document.getElementById('wifiEnabled').checked,
                ssid: document.getElementById('wifiSsid').value,
                password: document.getElementById('wifiPassword').value,
                dhcp: dhcp,
                ip:      dhcp ? '' : document.getElementById('wifiIp').value.trim(),
                gateway: dhcp ? '' : document.getElementById('wifiGateway').value.trim(),
                subnet:  dhcp ? '' : document.getElementById('wifiSubnet').value.trim(),
                dns:     dhcp ? '' : document.getElementById('wifiDns').value.trim()
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
                enabled: document.getElementById('mqttEnabled').checked,
                server: document.getElementById('mqttHost').value,
                port: parseInt(document.getElementById('mqttPort').value),
                username: document.getElementById('mqttUser').value,
                password: document.getElementById('mqttPass').value,
                transport: document.getElementById('mqttTransport').value
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

        function saveRS485ModbusConfig() {
            const data = {
                enabled: document.getElementById('rs485ModbusEnabled').checked,
                baudrate: parseInt(document.getElementById('rs485Baudrate').value),
                databits: parseInt(document.getElementById('rs485Databits').value),
                parity: document.getElementById('rs485Parity').value,
                stopbits: parseInt(document.getElementById('rs485Stopbits').value)
            };
            apiCall('/api/rs485modbus/config', 'POST', data);
        }

        function saveSerialPortConfig() {
            const data = {
                enabled:  document.getElementById('serialportEnabled').checked,
                baudrate: parseInt(document.getElementById('serialportBaudrate').value),
                databits: parseInt(document.getElementById('serialportDatabits').value),
                parity:   document.getElementById('serialportParity').value,
                stopbits: parseInt(document.getElementById('serialportStopbits').value)
            };
            apiCall('/api/serialport/config', 'POST', data);
        }

        // ── Shift helpers ─────────────────────────────────────────────────────
        var _shiftData = { enabled:false, num_shifts:1, shifts:[
            {name:'Morning',  start:'06:00', end:'14:00', breaks:[{start:'00:00',end:'00:00'},{start:'00:00',end:'00:00'},{start:'00:00',end:'00:00'}]},
            {name:'Afternoon',start:'14:00', end:'22:00', breaks:[{start:'00:00',end:'00:00'},{start:'00:00',end:'00:00'},{start:'00:00',end:'00:00'}]},
            {name:'Night',    start:'22:00', end:'06:00', breaks:[{start:'00:00',end:'00:00'},{start:'00:00',end:'00:00'},{start:'00:00',end:'00:00'}]}
        ]};

        function onShiftEnableChange() {
            var en = document.getElementById('shiftEnabled').checked;
            document.getElementById('shift_main_fields').style.display = en ? 'block' : 'none';
        }

        function _shiftField(id, dflt) {
            var el = document.getElementById(id);
            return el ? el.value.trim() || dflt : dflt;
        }

        function renderShiftForms() {
            var n = parseInt(document.getElementById('shiftNum').value) || 1;
            var names = ['Morning','Afternoon','Night'];
            var html = '';
            for (var i = 0; i < n; i++) {
                var s = _shiftData.shifts[i] || {};
                var nm = s.name  || names[i];
                var st = s.start || '06:00';
                var en = s.end   || '14:00';
                html += '<div style="border:1px solid #e0e0e0;border-radius:8px;padding:16px;margin-top:14px">';
                html += '<div style="font-weight:600;margin-bottom:10px">Shift ' + (i+1) + '</div>';
                html += '<div class="form-group"><label>Name</label><input type="text" id="sn'+(i+1)+'_name" value="'+nm+'" maxlength="23" style="width:160px"></div>';
                html += '<div class="form-group"><label>Start Time</label><input type="time" id="sn'+(i+1)+'_start" value="'+st+'"></div>';
                html += '<div class="form-group"><label>End Time</label><input type="time" id="sn'+(i+1)+'_end" value="'+en+'"></div>';
                html += '<div style="font-size:13px;color:#555;margin:10px 0 6px">Breaks &mdash; set both to 00:00 if unused</div>';
                for (var m = 1; m <= 3; m++) {
                    var b = (s.breaks && s.breaks[m-1]) || {};
                    var bs = b.start||'00:00'; var be = b.end||'00:00';
                    html += '<div style="display:flex;gap:10px;align-items:center;margin-bottom:6px">';
                    html += '<span style="min-width:55px;font-size:13px">Break '+m+'</span>';
                    html += '<label style="font-size:12px;color:#888">Start</label><input type="time" id="sn'+(i+1)+'_b'+m+'s" value="'+bs+'">';
                    html += '<label style="font-size:12px;color:#888">End</label><input type="time" id="sn'+(i+1)+'_b'+m+'e" value="'+be+'">';
                    html += '</div>';
                }
                html += '</div>';
            }
            document.getElementById('shiftForms').innerHTML = html;
        }

        // Convert "HH:MM" to minutes-since-midnight for comparison
        function _toMin(t) {
            if (!t || t === '00:00') return -1; // -1 = "no break / disabled"
            var p = t.split(':');
            return parseInt(p[0]||0)*60 + parseInt(p[1]||0);
        }
        // Normalize overnight shift end: if end <= start, add 1440 (24h)
        function _shiftRange(st, en) {
            var s = _toMin(st); var e = _toMin(en);
            if (e <= s) e += 1440;
            return {s:s, e:e};
        }
        function _timeInRange(t, shiftS, shiftE) {
            var m = _toMin(t);
            if (m === -1) return true; // 00:00 = disabled, always valid
            if (shiftE > 1440) { // overnight
                return (m >= shiftS) || (m <= shiftE - 1440);
            }
            return m >= shiftS && m <= shiftE;
        }

        function saveShiftConfig() {
            var n = parseInt(document.getElementById('shiftNum').value) || 1;
            var errors = [];
            var shifts = [];

            for (var i = 1; i <= 3; i++) {
                var st  = _shiftField('sn'+i+'_start', '06:00');
                var en  = _shiftField('sn'+i+'_end',   '14:00');
                var nm  = _shiftField('sn'+i+'_name',  ['Morning','Afternoon','Night'][i-1]);
                var rng = _shiftRange(st, en);
                var brks = [];
                for (var m = 1; m <= 3; m++) {
                    var bs = _shiftField('sn'+i+'_b'+m+'s', '00:00');
                    var be = _shiftField('sn'+i+'_b'+m+'e', '00:00');
                    var bsMin = _toMin(bs); var beMin = _toMin(be);
                    // Only validate if this shift is within the active count
                    if (i <= n) {
                        var bsDis = (bsMin === -1); var beDis = (beMin === -1);
                        if (!bsDis || !beDis) {
                            // One side set but not the other
                            if (bsDis !== beDis) {
                                errors.push('Shift '+i+' Break '+m+': both start and end must be set (or both 00:00).');
                            } else {
                                if (!_timeInRange(bs, rng.s, rng.e))
                                    errors.push('Shift '+i+' Break '+m+' start ('+bs+') is outside shift time ('+st+' \u2013 '+en+').');
                                if (!_timeInRange(be, rng.s, rng.e))
                                    errors.push('Shift '+i+' Break '+m+' end ('+be+') is outside shift time ('+st+' \u2013 '+en+').');
                                if (bsMin !== -1 && beMin !== -1 && beMin <= bsMin)
                                    errors.push('Shift '+i+' Break '+m+': end time must be after start time.');
                            }
                        }
                    }
                    brks.push({ start: bs, end: be });
                }
                shifts.push({ name: nm, start: st, end: en, breaks: brks });
            }

            if (errors.length > 0) {
                showAlert('\u26a0 Validation failed:\n\u2022 ' + errors.join('\n\u2022 '), 'error');
                return;
            }

            apiCall('/api/shift/config', 'POST', {
                enabled:    document.getElementById('shiftEnabled').checked,
                num_shifts: n,
                shifts:     shifts
            });
        }

        function setRTC() {
            let datetime = document.getElementById('rtcInput').value;
            if (!datetime) {
                showAlert('Please select date/time', 'error');
                return;
            }
            // Normalize: replace T with space, strip AM/PM, ensure seconds
            datetime = datetime.replace('T', ' ').replace(/\s*(AM|PM)\s*/i, '');
            if (datetime.length === 16) datetime += ':00';
            apiCall('/api/rtc/set', 'POST', { datetime: datetime });
        }
        
        function syncRTCBrowser() {
            const now = new Date();
            const pad = n => String(n).padStart(2, '0');
            const datetime = now.getFullYear() + '-' + pad(now.getMonth()+1) + '-' + pad(now.getDate()) + ' ' + pad(now.getHours()) + ':' + pad(now.getMinutes()) + ':' + pad(now.getSeconds());
            apiCall('/api/rtc/set', 'POST', { datetime: datetime });
        }
        
        function scanWiFi() {
            showAlert('Scanning WiFi networks...', 'success');
            apiCall('/api/wifi/scan', 'POST');
        }

        function loadNetworkStatus() {
            apiCall('/api/status').then(data => {
                if (data && data.network) _applyNetworkLiveStatus(data.network);
            });
        }

        function loadMqttStatus() {
            apiCall('/api/status').then(data => {
                if (data && data.mqtt) _applyMqttLiveStatus(data.mqtt);
            });
        }

        function _applyMqttLiveStatus(mqtt) {
            var badge   = document.getElementById('mqttLiveBadge');
            var details = document.getElementById('mqttLiveDetails');
            var card    = document.getElementById('mqttLiveCard');
            var reason  = document.getElementById('mqttLiveReason');
            if (!badge) return;
            var conn = (mqtt.status === 'Connected');
            badge.textContent = mqtt.status || 'Unknown';
            badge.className   = 'status ' + (conn ? 'connected' : 'disconnected');
            if (card)    card.style.borderLeftColor = conn ? '#28a745' : '#dc3545';
            if (details) details.style.display      = conn ? 'block'  : 'none';
            if (reason)  reason.style.display       = (!conn && mqtt.reason) ? 'block' : 'none';
            if (conn && details) {
                document.getElementById('mqttLiveBroker').textContent = mqtt.broker || '-';
                document.getElementById('mqttLivePort').textContent   = mqtt.port   || '-';
            }
            if (!conn && reason && mqtt.reason) {
                reason.textContent = 'Reason: ' + mqtt.reason;
            }
        }

        function _applyNetworkLiveStatus(net) {
            // --- WiFi ---
            var wifiBadge   = document.getElementById('wifiLiveBadge');
            var wifiDetails = document.getElementById('wifiLiveDetails');
            var wifiCard    = document.getElementById('wifiLiveCard');
            if (wifiBadge) {
                var wConn = (net.wifi_status === 'Connected');
                wifiBadge.textContent  = net.wifi_status || 'Unknown';
                wifiBadge.className    = 'status ' + (wConn ? 'connected' : 'disconnected');
                if (wifiCard)    wifiCard.style.borderLeftColor = wConn ? '#28a745' : '#dc3545';
                if (wifiDetails) wifiDetails.style.display      = wConn ? 'block'  : 'none';
                if (wConn && wifiDetails) {
                    document.getElementById('wifiLiveIp').textContent     = net.wifi_ip      || '-';
                    document.getElementById('wifiLiveSubnet').textContent  = net.wifi_subnet  || '-';
                    document.getElementById('wifiLiveGw').textContent      = net.wifi_gateway || '-';
                    document.getElementById('wifiLiveRssi').textContent    = (net.wifi_rssi != null) ? (net.wifi_rssi + ' dBm') : '-';
                }
            }
            // --- Ethernet ---
            var ethBadge   = document.getElementById('ethLiveBadge');
            var ethDetails = document.getElementById('ethLiveDetails');
            var ethCard    = document.getElementById('ethLiveCard');
            if (ethBadge) {
                var eConn = (net.eth_status === 'Connected');
                ethBadge.textContent  = net.eth_status || 'Unknown';
                ethBadge.className    = 'status ' + (eConn ? 'connected' : 'disconnected');
                if (ethCard)    ethCard.style.borderLeftColor = eConn ? '#28a745' : '#dc3545';
                if (ethDetails) ethDetails.style.display      = eConn ? 'block'  : 'none';
                if (eConn && ethDetails) {
                    document.getElementById('ethLiveIp').textContent      = net.eth_ip      || '-';
                    document.getElementById('ethLiveSubnet').textContent   = net.eth_subnet  || '-';
                    document.getElementById('ethLiveGw').textContent       = net.eth_gateway || '-';
                    document.getElementById('ethLiveMac').textContent      = net.mac         || '-';
                }
            }
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
                if (isAsyncServer) {
                    xhr.send(formData);
                } else {
                    xhr.setRequestHeader('Content-Type', 'application/octet-stream');
                    xhr.send(file);
                }
                
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

        function saveFilesystemConfig() {
            const data = { fs_enabled: document.getElementById('fsEnabled').checked };
            apiCall('/api/filesystem/config', 'POST', data);
        }

        async function formatFilesystem() {
            if (!confirm('WARNING: This will permanently erase ALL files on the filesystem!\nThis cannot be undone. Continue?')) return;
            apiCall('/api/filesystem/format', 'POST');
        }

        function saveIOConfig() {
            const data = {
                input_enabled:  document.getElementById('ioInputEnabled').checked,
                output_enabled: document.getElementById('ioOutputEnabled').checked,
                usb_enabled:    document.getElementById('ioUsbEnabled').checked
            };
            apiCall('/api/io/config', 'POST', data);
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
                    const fsProgressBar = document.getElementById('fsProgressBar');
                    const fsProgressText = document.getElementById('fsProgressText');
                    if (fsType && data.fsType) fsType.textContent = data.fsType;
                    if (fsTotal) fsTotal.textContent = formatBytes(data.total);
                    if (fsUsed) fsUsed.textContent = formatBytes(data.used);
                    const pct = data.total > 0 ? Math.round((data.used / data.total) * 100) : 0;
                    if (fsProgressBar) fsProgressBar.style.width = pct + '%';
                    if (fsProgressText) fsProgressText.textContent = pct + '% used';
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
            
            if (!files || files.length === 0) {
                html += '<p style="color: #666; padding: 20px; text-align: center;">No files found</p>';
            } else {
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
            }
            
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
                if (isAsyncServer) {
                    xhr.send(formData);
                } else {
                    const reader = new FileReader();
                    reader.onload = function() {
                        const base64 = btoa(new Uint8Array(reader.result).reduce((data, byte) => data + String.fromCharCode(byte), ''));
                        xhr.setRequestHeader('Content-Type', 'application/json');
                        xhr.send(JSON.stringify({ path: uploadPath, data: base64, size: file.size }));
                    };
                    reader.readAsArrayBuffer(file);
                }
                
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
                document.getElementById('wifiDhcp').checked = wifiConfig.dhcp !== false;
                document.getElementById('wifiIp').value = wifiConfig.ip || '';
                document.getElementById('wifiGateway').value = wifiConfig.gateway || '';
                document.getElementById('wifiSubnet').value = wifiConfig.subnet || '';
                document.getElementById('wifiDns').value = wifiConfig.dns || '';
                toggleWiFiStaticIP();
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
                document.getElementById('mqttEnabled').checked = mqttConfig.enabled || false;
                document.getElementById('mqttHost').value = mqttConfig.server || '';
                document.getElementById('mqttPort').value = mqttConfig.port || 1883;
                document.getElementById('mqttUser').value = mqttConfig.username || '';
                document.getElementById('mqttTransport').value = mqttConfig.transport || 'auto';
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

            // Load RS485 Modbus config
            const rs485Config = await apiCall('/api/rs485modbus/config');
            if (rs485Config.success) {
                document.getElementById('rs485ModbusEnabled').checked = rs485Config.enabled;
                document.getElementById('rs485ModbusRunning').textContent = rs485Config.running ? 'Running' : 'Stopped';
                document.getElementById('rs485Baudrate').value = rs485Config.baudrate || 9600;
                document.getElementById('rs485Databits').value = rs485Config.databits || 8;
                document.getElementById('rs485Parity').value = rs485Config.parity || 'N';
                document.getElementById('rs485Stopbits').value = rs485Config.stopbits || 1;
            }

            // Load Serial Port config
            const spConfig = await apiCall('/api/serialport/config');
            if (spConfig.success) {
                document.getElementById('serialportEnabled').checked = spConfig.enabled;
                document.getElementById('serialportRunning').textContent = spConfig.running ? 'Running' : 'Stopped';
                document.getElementById('serialportBaudrate').value = spConfig.baudrate || 9600;
                document.getElementById('serialportDatabits').value = spConfig.databits || 8;
                document.getElementById('serialportParity').value   = spConfig.parity   || 'N';
                document.getElementById('serialportStopbits').value = spConfig.stopbits || 1;
            }

            // Load Shift config
            const shiftConfig = await apiCall('/api/shift/config');
            if (shiftConfig.success) {
                _shiftData = shiftConfig;
                document.getElementById('shiftEnabled').checked = !!shiftConfig.enabled;
                document.getElementById('shiftNum').value = shiftConfig.num_shifts || 1;
                onShiftEnableChange();
                renderShiftForms();
            }

            // Load IO config
            const ioConfig = await apiCall('/api/io/config');
            if (ioConfig.success) {
                document.getElementById('ioInputEnabled').checked  = ioConfig.input_enabled;
                document.getElementById('ioOutputEnabled').checked = ioConfig.output_enabled;
                document.getElementById('ioUsbEnabled').checked    = ioConfig.usb_enabled;
            }

            // Load Filesystem config
            const fsConfig = await apiCall('/api/filesystem/config');
            if (fsConfig.success) {
                document.getElementById('fsEnabled').checked = fsConfig.fs_enabled;
                document.getElementById('fileManagerSection').style.display = fsConfig.fs_enabled ? 'block' : 'none';
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

// Body handler: buffers raw POST body into request->_tempObject.
// Must be registered as the 5th parameter of webServer.on() for each route
// that accepts application/json POST bodies.
void handleJsonBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        if (request->_tempObject) free(request->_tempObject);
        request->_tempObject = malloc(total + 1);
    }
    if (request->_tempObject) {
        memcpy((uint8_t*)request->_tempObject + index, data, len);
        if (index + len == total)
            ((char*)request->_tempObject)[total] = '\0';
    }
}

// Returns the buffered POST body (populated by handleJsonBody above).
inline String getRequestBody(AsyncWebServerRequest *request) {
    if (request->_tempObject != nullptr) {
        return String((char*)request->_tempObject);
    }
    return request->arg("plain");
}

// Appends reset counters and partition table to any JSON doc (reused by WiFi & Ethernet status handlers)
void addResetPartitionInfo(DynamicJsonDocument &doc) {
    JsonObject resetObj = doc.createNestedObject("reset");
    resetObj["reason"] = lastResetReason;
    {
        Preferences rp;
        rp.begin("reset_cnt", true);
        resetObj["total_boots"] = rp.getUInt("total", 0);
        JsonObject cnt = resetObj.createNestedObject("counters");
        cnt["poweron"]   = rp.getUInt("poweron",   0);
        cnt["sw"]        = rp.getUInt("sw",        0);
        cnt["panic"]     = rp.getUInt("panic",     0);
        cnt["int_wdt"]   = rp.getUInt("int_wdt",   0);
        cnt["task_wdt"]  = rp.getUInt("task_wdt",  0);
        cnt["wdt"]       = rp.getUInt("wdt",       0);
        cnt["brownout"]  = rp.getUInt("brownout",  0);
        cnt["ext"]       = rp.getUInt("ext",       0);
        cnt["deepsleep"] = rp.getUInt("deepsleep", 0);
        cnt["sdio"]      = rp.getUInt("sdio",      0);
        cnt["unknown"]   = rp.getUInt("unknown",   0);
        rp.end();
    }
    JsonArray parts = doc.createNestedArray("partitions");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (it != NULL) {
        const esp_partition_t *p = esp_partition_get(it);
        JsonObject po = parts.createNestedObject();
        po["label"] = p->label;
        if      (p->type == ESP_PARTITION_TYPE_APP)  po["type"] = "app";
        else if (p->type == ESP_PARTITION_TYPE_DATA) po["type"] = "data";
        else                                         po["type"] = "other";
        char sub[12];
        if (p->type == ESP_PARTITION_TYPE_APP) {
            if      (p->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) snprintf(sub, sizeof(sub), "factory");
            else if (p->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0)   snprintf(sub, sizeof(sub), "ota_0");
            else if (p->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1)   snprintf(sub, sizeof(sub), "ota_1");
            else                                                       snprintf(sub, sizeof(sub), "0x%02x", p->subtype);
        } else if (p->type == ESP_PARTITION_TYPE_DATA) {
            if      (p->subtype == ESP_PARTITION_SUBTYPE_DATA_NVS)    snprintf(sub, sizeof(sub), "nvs");
            else if (p->subtype == ESP_PARTITION_SUBTYPE_DATA_OTA)    snprintf(sub, sizeof(sub), "ota");
            else if (p->subtype == ESP_PARTITION_SUBTYPE_DATA_FAT)    snprintf(sub, sizeof(sub), "fat");
            else if (p->subtype == ESP_PARTITION_SUBTYPE_DATA_SPIFFS) snprintf(sub, sizeof(sub), "spiffs");
            else                                                       snprintf(sub, sizeof(sub), "0x%02x", p->subtype);
        } else {
            snprintf(sub, sizeof(sub), "0x%02x", p->subtype);
        }
        po["subtype"] = sub;
        po["address"] = p->address;
        po["size"]    = p->size;
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
    doc["flash_size"]  = ESP.getFlashChipSize();
    doc["flash_speed"] = ESP.getFlashChipSpeed() / 1000000;
}

void handleGetStatus(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    DynamicJsonDocument doc(4096);
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
        network["wifi_status"]  = "Connected";
        network["wifi_ssid"]    = WiFi.SSID();
        network["wifi_ip"]      = WiFi.localIP().toString();
        network["wifi_subnet"]  = WiFi.subnetMask().toString();
        network["wifi_gateway"] = WiFi.gatewayIP().toString();
        network["wifi_rssi"]    = WiFi.RSSI();
    } else {
        network["wifi_status"] = "Disconnected";
    }
    
    if (Ethernet.linkStatus() == LinkON) {
        network["eth_status"]  = "Connected";
        network["eth_ip"]      = Ethernet.localIP().toString();
        network["eth_subnet"]  = Ethernet.subnetMask().toString();
        network["eth_gateway"] = Ethernet.gatewayIP().toString();
        network["ip"]          = Ethernet.localIP().toString();
        network["gateway"]     = Ethernet.gatewayIP().toString();
        network["mac"]         = getEthernetMACString();
    } else {
        network["eth_status"] = "Disconnected";
        if (WiFi.status() == WL_CONNECTED) {
            network["ip"]  = WiFi.localIP().toString();
            network["mac"] = WiFi.macAddress();
        }
    }
    
    // RTC info
    JsonObject rtcObj = doc.createNestedObject("rtc");
    rtcObj["datetime"] = rtc.getDateTime();
    rtcObj["type"] = rtc.isExternalRTCAvailable() ? "External (DS3231)" : "Internal";

    // MQTT status
    JsonObject mqttSt = doc.createNestedObject("mqtt");
    if (mqtt_obj.connected()) {
        mqttSt["status"] = "Connected";
        mqttSt["broker"] = mqttPref.getString("server", "");
        mqttSt["port"]   = mqttPref.getInt("port", 1883);
    } else {
        mqttSt["status"] = "Disconnected";
        const int mst = mqtt_obj.state();
        const char* mReason;
        switch (mst) {
            case -4: mReason = "Connection Timeout";  break;
            case -3: mReason = "Connection Lost";     break;
            case -2: mReason = "Connect Failed";      break;
            case -1: mReason = "Disconnected";        break;
            case  1: mReason = "Bad Protocol";        break;
            case  2: mReason = "Bad Client ID";       break;
            case  3: mReason = "Server Unavailable";  break;
            case  4: mReason = "Bad Credentials";     break;
            case  5: mReason = "Unauthorized";        break;
            default: mReason = "Unknown";             break;
        }
        mqttSt["reason"] = mReason;
    }

    addResetPartitionInfo(doc);

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
        doc["ssid"]    = wifiPref.getString("ssid", "");
        doc["dhcp"]    = wifiPref.getBool("dhcp", true);
        doc["ip"]      = wifiPref.getString("ip", "");
        doc["gateway"] = wifiPref.getString("gateway", "");
        doc["subnet"]  = wifiPref.getString("subnet", "");
        doc["dns"]     = wifiPref.getString("dns", "");
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }
    else if (strcmp(request->methodToString(), "POST") == 0) {
        // Set WiFi config
        String body = getRequestBody(request);
        DynamicJsonDocument doc(512);
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        wifiPref.end();
        wifiPref.begin("wifi", false);
        
        if (doc.containsKey("enabled"))  wifiPref.putBool("enabled",   doc["enabled"]);
        if (doc.containsKey("ssid"))     wifiPref.putString("ssid",    doc["ssid"].as<String>());
        if (doc.containsKey("password")) wifiPref.putString("password", doc["password"].as<String>());
        if (doc.containsKey("dhcp"))     wifiPref.putBool("dhcp",      doc["dhcp"]);
        if (doc.containsKey("ip"))       wifiPref.putString("ip",       doc["ip"].as<String>());
        if (doc.containsKey("gateway"))  wifiPref.putString("gateway",  doc["gateway"].as<String>());
        if (doc.containsKey("subnet"))   wifiPref.putString("subnet",   doc["subnet"].as<String>());
        if (doc.containsKey("dns"))      wifiPref.putString("dns",      doc["dns"].as<String>());
        
        wifiPref.end();
        wifiPref.begin("wifi", true);
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"WiFi config saved\"}");
    }
}


void handleEthernetConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (request->method() == HTTP_GET) {
        // Do NOT call begin()/end() here — ethernetPref is kept open by the application
        // (same pattern as handleWiFiConfig). Calling begin() on an already-open instance
        // fails silently and causes all reads to return their default values.
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["enabled"] = ethernetPref.getBool("enabled", false);
        doc["dhcp"]    = ethernetPref.getBool("dhcp",    true);
        doc["ip"]      = ethernetPref.getString("ip",      "");
        doc["gateway"] = ethernetPref.getString("gateway", "");
        doc["subnet"]  = ethernetPref.getString("subnet",  "");
        doc["dns"]     = ethernetPref.getString("dns",     "");
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }
    else if (strcmp(request->methodToString(), "POST") == 0) {
        String body = getRequestBody(request);
        DynamicJsonDocument doc(512);
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        // Close read-only handle, open read-write, write, close, reopen read-only
        // (same pattern as handleWiFiConfig POST)
        ethernetPref.end();
        ethernetPref.begin("ethernet", false);
        
        if (doc.containsKey("enabled")) ethernetPref.putBool("enabled",   doc["enabled"]);
        if (doc.containsKey("dhcp"))    ethernetPref.putBool("dhcp",      doc["dhcp"]);
        if (doc.containsKey("ip"))      ethernetPref.putString("ip",      doc["ip"].as<String>());
        if (doc.containsKey("gateway")) ethernetPref.putString("gateway", doc["gateway"].as<String>());
        if (doc.containsKey("subnet"))  ethernetPref.putString("subnet",  doc["subnet"].as<String>());
        if (doc.containsKey("dns"))     ethernetPref.putString("dns",     doc["dns"].as<String>());
        
        ethernetPref.end();
        ethernetPref.begin("ethernet", true);   // reopen read-only for subsequent reads
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Ethernet config saved. Reconnect to apply.\"}");
    }
}


void handleMQTTConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;

    if (request->method() == HTTP_GET) {
        mqttPref.begin("mqtt", true);
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["enabled"] = mqttPref.getBool("enabled", false);
        doc["server"] = mqttPref.getString("server", "");
        doc["port"] = mqttPref.getUShort("port", 1883);
        doc["username"] = mqttPref.getString("username", "");
        doc["transport"] = mqttPref.getString("transport", "auto");
        mqttPref.end();
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }
    else if (strcmp(request->methodToString(), "POST") == 0) {
        String body = getRequestBody(request);
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        mqttPref.begin("mqtt", false);
        if (doc.containsKey("enabled")) mqttPref.putBool("enabled", doc["enabled"].as<bool>());
        if (doc.containsKey("server")) mqttPref.putString("server", doc["server"].as<String>());
        if (doc.containsKey("port")) mqttPref.putUShort("port", doc["port"].as<uint16_t>());
        if (doc.containsKey("username")) mqttPref.putString("username", doc["username"].as<String>());
        if (doc.containsKey("password")) mqttPref.putString("password", doc["password"].as<String>());
        if (doc.containsKey("transport")) mqttPref.putString("transport", doc["transport"].as<String>());
        mqttPref.end();
        request->send(200, "application/json", "{\"success\":true,\"message\":\"MQTT config saved\"}");
    }
}


void handleSubtopicConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    Preferences subPref;
    subPref.begin("subtopics", strcmp(request->methodToString(), "POST") == 0 ? false : true);
    
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
    else if (strcmp(request->methodToString(), "POST") == 0) {
        String body = getRequestBody(request);
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
    else if (strcmp(request->methodToString(), "POST") == 0) {
        String body = getRequestBody(request);
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


void handleRS485ModbusConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (request->method() == HTTP_GET) {
        DynamicJsonDocument doc(512);
        rs485ModbusPref.begin("rs485modbus", true);
        doc["success"] = true;
        doc["enabled"] = rs485ModbusPref.getBool("modbus_enabled", false);
        doc["running"] = rs485ModbusEnabled;
        doc["baudrate"] = rs485ModbusPref.getULong("baudrate", 9600);
        uint32_t cfg = rs485ModbusPref.getULong("config", SERIAL_8N1);
        rs485ModbusPref.end();
        uint8_t bits; char parity; uint8_t stop;
        decodeSerialConfig(cfg, bits, parity, stop);
        doc["databits"] = bits;
        char parityStr[2] = {parity, '\0'};
        doc["parity"] = parityStr;
        doc["stopbits"] = stop;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }
    else if (strcmp(request->methodToString(), "POST") == 0) {
        String body = getRequestBody(request);
        DynamicJsonDocument doc(512);
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        rs485ModbusPref.begin("rs485modbus", false);
        
        if (doc.containsKey("enabled")) {
            rs485ModbusPref.putBool("modbus_enabled", doc["enabled"]);
        }
        if (doc.containsKey("baudrate")) {
            rs485ModbusPref.putULong("baudrate", doc["baudrate"].as<uint32_t>());
        }
        if (doc.containsKey("databits") || doc.containsKey("parity") || doc.containsKey("stopbits")) {
            uint32_t cfg = rs485ModbusPref.getULong("config", SERIAL_8N1);
            uint8_t bits; char parity; uint8_t stop;
            decodeSerialConfig(cfg, bits, parity, stop);
            if (doc.containsKey("databits")) bits = doc["databits"].as<uint8_t>();
            if (doc.containsKey("parity")) {
                String p = doc["parity"].as<String>();
                if (p.length() > 0) parity = p.charAt(0);
            }
            if (doc.containsKey("stopbits")) stop = doc["stopbits"].as<uint8_t>();
            uint32_t newCfg = buildSerialConfig(bits, parity, stop);
            rs485ModbusPref.putULong("config", newCfg);
        }
        
        rs485ModbusPref.end();
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"RS485 Modbus config saved. Reboot required.\"}");
    }
}


void handleSerialPortConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;

    if (request->method() == HTTP_GET) {
        DynamicJsonDocument doc(512);
        serialportPref.begin(SERIALPORT_PREF_NS, true);
        doc["success"] = true;
        doc["enabled"] = serialportPref.getBool("enabled", false);
        doc["running"] = serialPortIsRunning();
        doc["baudrate"] = serialportPref.getULong("baudrate", 9600);
        uint32_t cfg = serialportPref.getULong("config", SERIAL_8N1);
        serialportPref.end();
        uint8_t bits; char parity; uint8_t stop;
        serialport_decodeConfig(cfg, bits, parity, stop);
        doc["databits"] = bits;
        char parityStr[2] = {parity, '\0'};
        doc["parity"] = parityStr;
        doc["stopbits"] = stop;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }
    else if (strcmp(request->methodToString(), "POST") == 0) {
        String body = getRequestBody(request);
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }

        serialportPref.begin(SERIALPORT_PREF_NS, false);
        if (doc.containsKey("enabled"))  serialportPref.putBool("enabled",   doc["enabled"]);
        if (doc.containsKey("baudrate")) serialportPref.putULong("baudrate",  doc["baudrate"].as<uint32_t>());
        if (doc.containsKey("databits") || doc.containsKey("parity") || doc.containsKey("stopbits")) {
            uint32_t cfg = serialportPref.getULong("config", SERIAL_8N1);
            uint8_t bits; char parity; uint8_t stop;
            serialport_decodeConfig(cfg, bits, parity, stop);
            if (doc.containsKey("databits")) bits = doc["databits"].as<uint8_t>();
            if (doc.containsKey("parity")) {
                String p = doc["parity"].as<String>();
                if (p.length() > 0) parity = p.charAt(0);
            }
            if (doc.containsKey("stopbits")) stop = doc["stopbits"].as<uint8_t>();
            serialportPref.putULong("config", serialport_buildConfig(bits, parity, stop));
        }
        serialportPref.end();
        serialportPref.begin(SERIALPORT_PREF_NS, true);

        request->send(200, "application/json", "{\"success\":true,\"message\":\"Serial Port config saved. Reboot required.\"}");
    }
}

// ── GET|POST /api/shift/config ────────────────────────────────────────────────
void handleShiftConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;

    if (request->method() == HTTP_GET) {
        request->send(200, "application/json", shiftDetailsToJson());
    }
    else if (strcmp(request->methodToString(), "POST") == 0) {
        String body = getRequestBody(request);
        if (shiftDetailsFromJson(body)) {
            request->send(200, "application/json",
                          "{\"success\":true,\"message\":\"Shift config saved.\"}");
        } else {
            request->send(400, "application/json",
                          "{\"success\":false,\"message\":\"Invalid JSON or no changes.\"}");
        }
    }
}


void handleRTCSet(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    String body = getRequestBody(request);
    DynamicJsonDocument doc(256);
    
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }
    
    String datetime = doc["datetime"].as<String>();
    
    // Parse: YYYY-MM-DD HH:MM:SS or YYYY-MM-DDTHH:MM:SS or YYYY-MM-DDTHH:MM
    datetime.replace('T', ' ');
    
    int year, month, day, hour, minute, second = 0;
    int parsed = sscanf(datetime.c_str(), "%d-%d-%d %d:%d:%d", 
                       &year, &month, &day, &hour, &minute, &second);
    
    if (parsed < 5) {
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid datetime format\"}");
        return;
    }
    
    rtc.setDateTime(day, month, year, hour, minute, second);
    HMI.Time_Stamp(day, month, year - 2000, hour, minute, second);
    
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
    
    ethernetPref.begin("ethernet", false);
    ethernetPref.clear();
    ethernetPref.end();
    
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
        } else {
            Update.printError(Serial);
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
    DynamicJsonDocument doc(4096);
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

    // MQTT status
    JsonObject mqttSt = doc.createNestedObject("mqtt");
    if (mqtt_obj.connected()) {
        mqttSt["status"] = "Connected";
        mqttSt["broker"] = mqttPref.getString("server", "");
        mqttSt["port"]   = mqttPref.getInt("port", 1883);
    } else {
        mqttSt["status"] = "Disconnected";
        const int mst = mqtt_obj.state();
        const char* mReason;
        switch (mst) {
            case -4: mReason = "Connection Timeout";  break;
            case -3: mReason = "Connection Lost";     break;
            case -2: mReason = "Connect Failed";      break;
            case -1: mReason = "Disconnected";        break;
            case  1: mReason = "Bad Protocol";        break;
            case  2: mReason = "Bad Client ID";       break;
            case  3: mReason = "Server Unavailable";  break;
            case  4: mReason = "Bad Credentials";     break;
            case  5: mReason = "Unauthorized";        break;
            default: mReason = "Unknown";             break;
        }
        mqttSt["reason"] = mReason;
    }

    addResetPartitionInfo(doc);

    String output;
    serializeJson(doc, output);
    return output;
}


// ==================== FILE MANAGEMENT HANDLERS ====================

void handleFileList(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (!fsManagerFFat.isFilesystemMounted()) {
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
        // file.name() may return full path on newer ESP32 cores — strip path prefix
        String fname = String(file.name());
        int lastSlash = fname.lastIndexOf('/');
        if (lastSlash >= 0) fname = fname.substring(lastSlash + 1);
        fileObj["name"] = fname;
        fileObj["size"] = file.size();
        fileObj["isDir"] = file.isDirectory();
        file = root.openNextFile();
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}


static bool fileUploadSuccess = false;

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    static File uploadFile;
    static String uploadPath;
    
    if (!index) {
        fileUploadSuccess = false;
        
        if (!fsManagerFFat.isFilesystemMounted()) {
            Serial.println("[FS] Upload rejected: filesystem not mounted");
            return;
        }
        
        // Start of upload
        uploadPath = request->hasArg("path") ? request->arg("path") : "/" + filename;
        
        Serial.printf("[FS] Upload start: %s\n", uploadPath.c_str());
        
        if (fsManagerFFat.search(uploadPath)) {
            fsManagerFFat.deleteFile(uploadPath);
        }
        
        fs::FS *fs = fsManagerFFat.getActiveFilesystem();
        if (!fs) {
            Serial.println("[FS] Filesystem not available");
            return;
        }
        
        uploadFile = fs->open(uploadPath, FILE_WRITE);
        if (!uploadFile) {
            Serial.println("[FS] Failed to open file for writing");
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
            fileUploadSuccess = true;
        }
    }
}


void handleFileDownload(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (!fsManagerFFat.isFilesystemMounted()) {
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
    
    if (!fsManagerFFat.isFilesystemMounted()) {
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
    
    if (!fsManagerFFat.isFilesystemMounted()) {
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
    
    size_t docSize = max((size_t)1024, content.length() * 2 + 512);
    DynamicJsonDocument doc(docSize);
    doc["success"] = true;
    doc["content"] = content;
    doc["path"] = path;
    
    // Free content memory before serializing
    content = String();
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
    
    Serial.printf("[FS] Read: %s (%d bytes)\n", path.c_str(), doc["content"].as<String>().length());
}


String _fileWriteBody = "";

void handleFileWriteBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        _fileWriteBody = "";
        _fileWriteBody.reserve(total);
    }
    for (size_t i = 0; i < len; i++) {
        _fileWriteBody += (char)data[i];
    }
}

void handleFileWrite(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    
    if (!fsManagerFFat.isFilesystemMounted()) {
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Filesystem not mounted\"}");
        _fileWriteBody = "";
        return;
    }
    
    String body = _fileWriteBody;
    _fileWriteBody = "";
    size_t docSize = max((size_t)1024, body.length() * 2 + 512);
    DynamicJsonDocument doc(docSize);
    
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


// ==================== IO & FILESYSTEM & WEB SETTINGS HANDLERS ====================

void handleIOConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    if (request->method() == HTTP_GET) {
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["input_enabled"]  = settingsPref.getBool("input_enabled",  false);
        doc["output_enabled"] = settingsPref.getBool("output_enabled", false);
        doc["usb_enabled"]    = settingsPref.getBool("usb_enabled",    false);
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    } else if (strcmp(request->methodToString(), "POST") == 0) {
        String body = getRequestBody(request);
        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, body)) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        settingsPref.end();
        settingsPref.begin("settings", false);
        if (doc.containsKey("input_enabled"))  settingsPref.putBool("input_enabled",  doc["input_enabled"]);
        if (doc.containsKey("output_enabled")) settingsPref.putBool("output_enabled", doc["output_enabled"]);
        if (doc.containsKey("usb_enabled"))    settingsPref.putBool("usb_enabled",    doc["usb_enabled"]);
        settingsPref.end();
        settingsPref.begin("settings", true);
        request->send(200, "application/json", "{\"success\":true,\"message\":\"IO config saved. Reboot to apply.\"}");
    }
}

void handleFilesystemConfig(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    if (request->method() == HTTP_GET) {
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["fs_enabled"] = settingsPref.getBool("fs_enabled", false);
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    } else if (strcmp(request->methodToString(), "POST") == 0) {
        String body = getRequestBody(request);
        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, body)) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        settingsPref.end();
        settingsPref.begin("settings", false);
        if (doc.containsKey("fs_enabled")) settingsPref.putBool("fs_enabled", doc["fs_enabled"]);
        settingsPref.end();
        settingsPref.begin("settings", true);
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Filesystem config saved. Reboot to apply.\"}");
    }
}

void handleFilesystemFormat(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    bool ok = fsManagerFFat.format();
    if (ok) {
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Filesystem formatted. Reboot to remount.\"}");
    } else {
        request->send(500, "application/json", "{\"success\":false,\"message\":\"Format failed\"}");
    }
}

void handleWebSettings(AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;
    if (request->method() == HTTP_GET) {
        DynamicJsonDocument doc(256);
        doc["success"]      = true;
        doc["username"]     = web_username;
        doc["auth_enabled"] = web_auth_enabled;
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    } else if (strcmp(request->methodToString(), "POST") == 0) {
        String body = getRequestBody(request);
        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, body)) {
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        if (doc.containsKey("username")) web_username = doc["username"].as<String>();
        if (doc.containsKey("password") && doc["password"].as<String>().length() > 0)
            web_password = doc["password"].as<String>();
        if (doc.containsKey("auth_enabled")) web_auth_enabled = doc["auth_enabled"];
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Web settings saved\"}");
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
    // initFilesystem();
    
    // API Routes
    webServer.on("/api/status", HTTP_GET, handleGetStatus);
    webServer.on("/api/wifi/config", HTTP_ANY, handleWiFiConfig, NULL, handleJsonBody);
    webServer.on("/api/ethernet/config", HTTP_ANY, handleEthernetConfig, NULL, handleJsonBody);
    webServer.on("/api/mqtt/config", HTTP_ANY, handleMQTTConfig, NULL, handleJsonBody);
    webServer.on("/api/subtopic/config", HTTP_ANY, handleSubtopicConfig, NULL, handleJsonBody);
    webServer.on("/api/hmi/config", HTTP_ANY, handleHMIConfig, NULL, handleJsonBody);
    webServer.on("/api/rs485modbus/config", HTTP_ANY, handleRS485ModbusConfig, NULL, handleJsonBody);
    webServer.on("/api/serialport/config", HTTP_ANY, handleSerialPortConfig, NULL, handleJsonBody);
    webServer.on("/api/shift/config",      HTTP_ANY, handleShiftConfig,      NULL, handleJsonBody);
    webServer.on("/api/rtc/set", HTTP_POST, handleRTCSet, NULL, handleJsonBody);
    webServer.on("/api/system/reboot", HTTP_POST, handleSystemReboot);
    webServer.on("/api/system/factory", HTTP_POST, handleFactoryReset);
    
    // File management routes
    webServer.on("/api/files/list", HTTP_GET, handleFileList);
    webServer.on("/api/files/read", HTTP_GET, handleFileRead);
    webServer.on("/api/files/write", HTTP_POST, handleFileWrite, NULL, handleFileWriteBody);
    webServer.on("/api/files/download", HTTP_GET, handleFileDownload);
    webServer.on("/api/files/delete", HTTP_ANY, handleFileDelete);
    webServer.on("/api/io/config", HTTP_ANY, handleIOConfig, NULL, handleJsonBody);
    webServer.on("/api/filesystem/config", HTTP_ANY, handleFilesystemConfig, NULL, handleJsonBody);
    webServer.on("/api/filesystem/format", HTTP_POST, handleFilesystemFormat);
    webServer.on("/api/web/settings", HTTP_ANY, handleWebSettings, NULL, handleJsonBody);
    
    // Firmware update handler
    webServer.on("/api/firmware/update", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            if (Update.hasError()) {
                request->send(500, "application/json", "{\"success\":false,\"message\":\"Update failed\"}");
            } else {
                request->send(200, "application/json", "{\"success\":true,\"message\":\"Update complete. Rebooting...\"}");
                delay(1000);
                ESP.restart();
            }
        },
        handleFirmwareUpdate
    );
    
    // File upload handler
    webServer.on("/api/files/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            if (fileUploadSuccess) {
                request->send(200, "application/json", "{\"success\":true,\"message\":\"File uploaded successfully\"}");
            } else {
                request->send(500, "application/json", "{\"success\":false,\"message\":\"Upload failed\"}");
            }
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


// ==================================================================================
// ==================== ETHERNET WEB SERVER (Classic Ethernet.h) ====================
// ==================================================================================

// Helper: Send HTTP response header
void ethSendHeader(EthernetClient &client, int code, const char* contentType, size_t contentLength = 0) {
    client.print("HTTP/1.1 ");
    client.print(code);
    switch (code) {
        case 200: client.println(" OK"); break;
        case 400: client.println(" Bad Request"); break;
        case 401: client.println(" Unauthorized"); break;
        case 404: client.println(" Not Found"); break;
        case 500: client.println(" Internal Server Error"); break;
        default:  client.println(" OK"); break;
    }
    client.print("Content-Type: ");
    client.println(contentType);
    client.println("Access-Control-Allow-Origin: *");
    client.println("Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS");
    client.println("Access-Control-Allow-Headers: Content-Type, Authorization");
    if (contentLength > 0) {
        client.print("Content-Length: ");
        client.println(contentLength);
    }
    client.println("Connection: close");
    if (code == 401) {
        client.println("WWW-Authenticate: Basic realm=\"Login Required\"");
    }
    client.println(); // End of headers
}

// Helper: Send JSON response
void ethSendJSON(EthernetClient &client, int code, const String &json) {
    ethSendHeader(client, code, "application/json", json.length());
    size_t sent = 0;
    while (sent < json.length()) {
        size_t chunk = min((size_t)1024, json.length() - sent);
        client.write((const uint8_t*)(json.c_str() + sent), chunk);
        sent += chunk;
    }
}

// Helper: Send large HTML page in chunks
void ethSendHTMLPage(EthernetClient &client, const char* html) {
    size_t len = strlen_P(html);
    ethSendHeader(client, 200, "text/html", len);
    size_t sent = 0;
    const size_t chunkSize = 1024;
    while (sent < len) {
        size_t toSend = min(chunkSize, len - sent);
        client.write((const uint8_t*)(html + sent), toSend);
        sent += toSend;
        delay(1); // Yield for W5500 buffer
    }
}

// Helper: Check Basic Auth
bool checkEthAuth(const String &authHeader) {
    if (!web_auth_enabled) return true;
    if (authHeader.length() == 0) return false;
    if (!authHeader.startsWith("Basic ")) return false;
    
    String encoded = authHeader.substring(6);
    encoded.trim();
    
    // Decode base64 using mbedtls
    unsigned char decoded[128];
    size_t decodedLen = 0;
    int ret = mbedtls_base64_decode(decoded, sizeof(decoded) - 1, &decodedLen,
                                     (const unsigned char*)encoded.c_str(), encoded.length());
    if (ret != 0) return false;
    decoded[decodedLen] = 0;
    
    String credentials = String((char*)decoded);
    String expected = web_username + ":" + web_password;
    return (credentials == expected);
}

// Helper: Extract query parameter value
String getEthQueryParam(const String &query, const String &key) {
    int start = query.indexOf(key + "=");
    if (start < 0) return "";
    start += key.length() + 1;
    int end = query.indexOf('&', start);
    if (end < 0) end = query.length();
    return query.substring(start, end);
}

// Helper: URL decode
String urlDecode(const String &input) {
    String decoded = "";
    for (unsigned int i = 0; i < input.length(); i++) {
        if (input[i] == '%' && i + 2 < input.length()) {
            char hex[3] = { input[i + 1], input[i + 2], 0 };
            decoded += (char)strtol(hex, NULL, 16);
            i += 2;
        } else if (input[i] == '+') {
            decoded += ' ';
        } else {
            decoded += input[i];
        }
    }
    return decoded;
}

// ==================== ETHERNET WEB SERVER SETUP ====================

void setupEthWebServer() {
    if (ethWebServerStarted) return;
    ethWebServer.begin();
    ethWebServerStarted = true;
    Serial.printf("[EthWeb] Server started on port 80 at %s\n", Ethernet.localIP().toString().c_str());
}

// ==================== ETHERNET WEB CLIENT HANDLER ====================

void handleEthWebClients() {
    if (!ethWebServerStarted) return;
    
    EthernetClient client = ethWebServer.available();
    if (!client) return;
    
    // Wait for data with timeout
    unsigned long timeout = millis();
    while (!client.available()) {
        if (millis() - timeout > 3000) { client.stop(); return; }
        delay(1);
    }
    
    // Read request line: "GET /path HTTP/1.1"
    String requestLine = client.readStringUntil('\n');
    requestLine.trim();
    
    int sp1 = requestLine.indexOf(' ');
    int sp2 = requestLine.indexOf(' ', sp1 + 1);
    if (sp1 < 0 || sp2 < 0) { client.stop(); return; }
    
    String method = requestLine.substring(0, sp1);
    String fullPath = requestLine.substring(sp1 + 1, sp2);
    
    // Separate path and query string
    String path = fullPath;
    String query = "";
    int qIdx = fullPath.indexOf('?');
    if (qIdx >= 0) {
        path = fullPath.substring(0, qIdx);
        query = fullPath.substring(qIdx + 1);
    }
    
    // Read headers
    String authHeader = "";
    int contentLength = 0;
    while (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) break; // End of headers
        if (line.startsWith("Authorization: ")) authHeader = line.substring(15);
        else if (line.startsWith("Content-Length: ")) contentLength = line.substring(16).toInt();
    }
    
    // Read body for POST requests
    String body = "";
    if (contentLength > 0 && contentLength < 8192) {
        unsigned long bodyTimeout = millis();
        while ((int)body.length() < contentLength) {
            if (client.available()) {
                body += (char)client.read();
                bodyTimeout = millis();
            } else if (millis() - bodyTimeout > 2000) {
                break;
            } else {
                delay(1);
            }
        }
    }
    
    // Handle CORS preflight
    if (method == "OPTIONS") {
        ethSendHeader(client, 200, "text/plain", 0);
        client.stop();
        return;
    }
    
    // Auth check
    if (!checkEthAuth(authHeader)) {
        ethSendHeader(client, 401, "text/plain", 12);
        client.print("Unauthorized");
        client.stop();
        return;
    }
    
    Serial.printf("[EthWeb] %s %s\n", method.c_str(), path.c_str());
    
    // ==================== ROUTE HANDLING ====================
    
    // --- Serve main page ---
    if (path == "/" && method == "GET") {
        ethSendHTMLPage(client, index_html);
    }
    
    // --- GET /api/status ---
    else if (path == "/api/status" && method == "GET") {
        DynamicJsonDocument doc(4096);
        doc["success"] = true;
        
        JsonObject system = doc.createNestedObject("system");
        system["chip_model"] = ESP.getChipModel();
        system["chip_cores"] = ESP.getChipCores();
        system["cpu_freq"] = ESP.getCpuFreqMHz();
        system["free_heap"] = ESP.getFreeHeap();
        system["flash_size"] = ESP.getFlashChipSize();
        system["uptime"] = millis() / 1000;
        
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
        
        JsonObject rtcObj = doc.createNestedObject("rtc");
        rtcObj["datetime"] = rtc.getDateTime();
        rtcObj["type"] = rtc.isExternalRTCAvailable() ? "External (DS3231)" : "Internal";

        addResetPartitionInfo(doc);

        String response;
        serializeJson(doc, response);
        ethSendJSON(client, 200, response);
    }
    
    // --- GET /api/wifi/config ---
    else if (path == "/api/wifi/config" && method == "GET") {
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["enabled"] = wifiPref.getBool("enabled", false);
        doc["ssid"]    = wifiPref.getString("ssid", "");
        doc["dhcp"]    = wifiPref.getBool("dhcp", true);
        doc["ip"]      = wifiPref.getString("ip", "");
        doc["gateway"] = wifiPref.getString("gateway", "");
        doc["subnet"]  = wifiPref.getString("subnet", "");
        doc["dns"]     = wifiPref.getString("dns", "");
        
        String response;
        serializeJson(doc, response);
        ethSendJSON(client, 200, response);
    }
    
    // --- POST /api/wifi/config ---
    else if (path == "/api/wifi/config" && method == "POST") {
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            wifiPref.end();
            wifiPref.begin("wifi", false);
            if (doc.containsKey("enabled"))  wifiPref.putBool("enabled",   doc["enabled"]);
            if (doc.containsKey("ssid"))     wifiPref.putString("ssid",    doc["ssid"].as<String>());
            if (doc.containsKey("password")) wifiPref.putString("password", doc["password"].as<String>());
            if (doc.containsKey("dhcp"))     wifiPref.putBool("dhcp",      doc["dhcp"]);
            if (doc.containsKey("ip"))       wifiPref.putString("ip",       doc["ip"].as<String>());
            if (doc.containsKey("gateway"))  wifiPref.putString("gateway",  doc["gateway"].as<String>());
            if (doc.containsKey("subnet"))   wifiPref.putString("subnet",   doc["subnet"].as<String>());
            if (doc.containsKey("dns"))      wifiPref.putString("dns",      doc["dns"].as<String>());
            wifiPref.end();
            wifiPref.begin("wifi", true);
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"WiFi config saved\"}");
        }
    }
    
    // --- GET /api/ethernet/config ---
    else if (path == "/api/ethernet/config" && method == "GET") {
        ethernetPref.begin("ethernet", true);
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["enabled"] = ethernetPref.getBool("enabled", true);
        doc["dhcp"] = ethernetPref.getBool("dhcp", true);
        doc["ip"] = ethernetPref.getString("ip", "");
        doc["gateway"] = ethernetPref.getString("gateway", "");
        doc["subnet"] = ethernetPref.getString("subnet", "");
        doc["dns"] = ethernetPref.getString("dns", "");
        ethernetPref.end();
        
        String response;
        serializeJson(doc, response);
        ethSendJSON(client, 200, response);
    }
    
    // --- POST /api/ethernet/config ---
    else if (path == "/api/ethernet/config" && method == "POST") {
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            ethernetPref.begin("ethernet", false);
            if (doc.containsKey("enabled")) ethernetPref.putBool("enabled", doc["enabled"]);
            if (doc.containsKey("dhcp")) ethernetPref.putBool("dhcp", doc["dhcp"]);
            if (doc.containsKey("ip")) ethernetPref.putString("ip", doc["ip"].as<String>());
            if (doc.containsKey("gateway")) ethernetPref.putString("gateway", doc["gateway"].as<String>());
            if (doc.containsKey("subnet")) ethernetPref.putString("subnet", doc["subnet"].as<String>());
            if (doc.containsKey("dns")) ethernetPref.putString("dns", doc["dns"].as<String>());
            ethernetPref.end();
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"Ethernet config saved. Reconnect to apply.\"}");
        }
    }
    
    // --- GET /api/mqtt/config ---
    else if (path == "/api/mqtt/config" && method == "GET") {
        mqttPref.begin("mqtt", true);
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["server"] = mqttPref.getString("server", "");
        doc["port"] = mqttPref.getUShort("port", 1883);
        doc["username"] = mqttPref.getString("username", "");
        doc["transport"] = mqttPref.getString("transport", "auto");
        mqttPref.end();
        
        String response;
        serializeJson(doc, response);
        ethSendJSON(client, 200, response);
    }
    
    // --- POST /api/mqtt/config ---
    else if (path == "/api/mqtt/config" && method == "POST") {
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            mqttPref.begin("mqtt", false);
            if (doc.containsKey("server")) mqttPref.putString("server", doc["server"].as<String>());
            if (doc.containsKey("port")) mqttPref.putUShort("port", doc["port"].as<uint16_t>());
            if (doc.containsKey("username")) mqttPref.putString("username", doc["username"].as<String>());
            if (doc.containsKey("password")) mqttPref.putString("password", doc["password"].as<String>());
            if (doc.containsKey("transport")) mqttPref.putString("transport", doc["transport"].as<String>());
            mqttPref.end();
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"MQTT config saved\"}");
        }
    }
    
    // --- GET /api/subtopic/config ---
    else if (path == "/api/subtopic/config" && method == "GET") {
        Preferences subPref;
        subPref.begin("subtopics", true);
        DynamicJsonDocument doc(512);
        doc["success"] = true;
        doc["company"] = subPref.getString("company", "");
        doc["location"] = subPref.getString("location", "");
        doc["department"] = subPref.getString("department", "");
        doc["line"] = subPref.getString("line", "");
        doc["machine"] = subPref.getString("machine", "");
        subPref.end();
        
        String response;
        serializeJson(doc, response);
        ethSendJSON(client, 200, response);
    }
    
    // --- POST /api/subtopic/config ---
    else if (path == "/api/subtopic/config" && method == "POST") {
        Preferences subPref;
        subPref.begin("subtopics", false);
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            subPref.end();
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            if (doc.containsKey("company")) subPref.putString("company", doc["company"].as<String>());
            if (doc.containsKey("location")) subPref.putString("location", doc["location"].as<String>());
            if (doc.containsKey("department")) subPref.putString("department", doc["department"].as<String>());
            if (doc.containsKey("line")) subPref.putString("line", doc["line"].as<String>());
            if (doc.containsKey("machine")) subPref.putString("machine", doc["machine"].as<String>());
            subPref.end();
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"Subtopic config saved\"}");
        }
    }
    
    // --- GET /api/hmi/config ---
    else if (path == "/api/hmi/config" && method == "GET") {
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["enabled"] = hmiPref.getBool("enabled", true);
        
        String response;
        serializeJson(doc, response);
        ethSendJSON(client, 200, response);
    }
    
    // --- POST /api/hmi/config ---
    else if (path == "/api/hmi/config" && method == "POST") {
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            hmiPref.end();
            hmiPref.begin("hmi", false);
            if (doc.containsKey("enabled")) hmiPref.putBool("enabled", doc["enabled"]);
            hmiPref.end();
            hmiPref.begin("hmi", true);
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"HMI config saved. Reboot required.\"}");
        }
    }
    
    // --- GET /api/rs485modbus/config ---
    else if (path == "/api/rs485modbus/config" && method == "GET") {
        DynamicJsonDocument doc(512);
        rs485ModbusPref.begin("rs485modbus", true);
        doc["success"] = true;
        doc["enabled"] = rs485ModbusPref.getBool("modbus_enabled", false);
        doc["running"] = rs485ModbusEnabled;
        doc["baudrate"] = rs485ModbusPref.getULong("baudrate", 9600);
        uint32_t cfg = rs485ModbusPref.getULong("config", SERIAL_8N1);
        rs485ModbusPref.end();
        uint8_t bits; char parity; uint8_t stop;
        decodeSerialConfig(cfg, bits, parity, stop);
        doc["databits"] = bits;
        char parityStr[2] = {parity, '\0'};
        doc["parity"] = parityStr;
        doc["stopbits"] = stop;
        
        String response;
        serializeJson(doc, response);
        ethSendJSON(client, 200, response);
    }
    
    // --- POST /api/rs485modbus/config ---
    else if (path == "/api/rs485modbus/config" && method == "POST") {
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            rs485ModbusPref.begin("rs485modbus", false);
            if (doc.containsKey("enabled")) rs485ModbusPref.putBool("modbus_enabled", doc["enabled"]);
            if (doc.containsKey("baudrate")) rs485ModbusPref.putULong("baudrate", doc["baudrate"].as<uint32_t>());
            if (doc.containsKey("databits") || doc.containsKey("parity") || doc.containsKey("stopbits")) {
                uint32_t cfg = rs485ModbusPref.getULong("config", SERIAL_8N1);
                uint8_t bits; char parity; uint8_t stop;
                decodeSerialConfig(cfg, bits, parity, stop);
                if (doc.containsKey("databits")) bits = doc["databits"].as<uint8_t>();
                if (doc.containsKey("parity")) {
                    String p = doc["parity"].as<String>();
                    if (p.length() > 0) parity = p.charAt(0);
                }
                if (doc.containsKey("stopbits")) stop = doc["stopbits"].as<uint8_t>();
                uint32_t newCfg = buildSerialConfig(bits, parity, stop);
                rs485ModbusPref.putULong("config", newCfg);
            }
            rs485ModbusPref.end();
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"RS485 Modbus config saved. Reboot required.\"}");
        }
    }

    // --- GET /api/serialport/config ---
    else if (path == "/api/serialport/config" && method == "GET") {
        DynamicJsonDocument doc(512);
        serialportPref.begin(SERIALPORT_PREF_NS, true);
        doc["success"] = true;
        doc["enabled"] = serialportPref.getBool("enabled", false);
        doc["running"] = serialPortIsRunning();
        doc["baudrate"] = serialportPref.getULong("baudrate", 9600);
        uint32_t cfg = serialportPref.getULong("config", SERIAL_8N1);
        serialportPref.end();
        uint8_t bits; char parity; uint8_t stop;
        serialport_decodeConfig(cfg, bits, parity, stop);
        doc["databits"] = bits;
        char parityStr[2] = {parity, '\0'};
        doc["parity"] = parityStr;
        doc["stopbits"] = stop;
        String response;
        serializeJson(doc, response);
        ethSendJSON(client, 200, response);
    }

    // --- POST /api/serialport/config ---
    else if (path == "/api/serialport/config" && method == "POST") {
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            serialportPref.begin(SERIALPORT_PREF_NS, false);
            if (doc.containsKey("enabled"))  serialportPref.putBool("enabled",  doc["enabled"]);
            if (doc.containsKey("baudrate")) serialportPref.putULong("baudrate", doc["baudrate"].as<uint32_t>());
            if (doc.containsKey("databits") || doc.containsKey("parity") || doc.containsKey("stopbits")) {
                uint32_t cfg = serialportPref.getULong("config", SERIAL_8N1);
                uint8_t bits; char parity; uint8_t stop;
                serialport_decodeConfig(cfg, bits, parity, stop);
                if (doc.containsKey("databits")) bits = doc["databits"].as<uint8_t>();
                if (doc.containsKey("parity")) {
                    String p = doc["parity"].as<String>();
                    if (p.length() > 0) parity = p.charAt(0);
                }
                if (doc.containsKey("stopbits")) stop = doc["stopbits"].as<uint8_t>();
                serialportPref.putULong("config", serialport_buildConfig(bits, parity, stop));
            }
            serialportPref.end();
            serialportPref.begin(SERIALPORT_PREF_NS, true);
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"Serial Port config saved. Reboot required.\"}");
        }
    }

    // --- GET /api/shift/config ---
    else if (path == "/api/shift/config" && method == "GET") {
        ethSendJSON(client, 200, shiftDetailsToJson());
    }

    // --- POST /api/shift/config ---
    else if (path == "/api/shift/config" && method == "POST") {
        if (shiftDetailsFromJson(body)) {
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"Shift config saved.\"}");
        } else {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON or no changes.\"}");
        }
    }

    // --- POST /api/rtc/set ---
    else if (path == "/api/rtc/set" && method == "POST") {
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            String datetime = doc["datetime"].as<String>();
            datetime.replace('T', ' ');
            int year, month, day, hour, minute, second = 0;
            int parsed = sscanf(datetime.c_str(), "%d-%d-%d %d:%d:%d",
                                &year, &month, &day, &hour, &minute, &second);
            if (parsed < 5) {
                ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid datetime format\"}");
            } else {
                rtc.setDateTime(day, month, year, hour, minute, second);
                HMI.Time_Stamp(day, month, year - 2000, hour, minute, second);
                ethSendJSON(client, 200, "{\"success\":true,\"message\":\"RTC updated\"}");
            }
        }
    }
    
    // --- POST /api/firmware/update ---
    else if (path == "/api/firmware/update" && method == "POST") {
        if (contentLength <= 0) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"No firmware data\"}");
        } else {
            Serial.printf("[EthWeb] OTA update start, size: %d\n", contentLength);
            if (!Update.begin(contentLength)) {
                Update.printError(Serial);
                ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Update begin failed\"}");
            } else {
                uint8_t buf[512];
                size_t remaining = contentLength;
                bool success = true;
                while (remaining > 0) {
                    size_t toRead = min((size_t)512, remaining);
                    size_t bytesRead = 0;
                    unsigned long chunkTimeout = millis();
                    while (bytesRead < toRead) {
                        if (client.available()) {
                            buf[bytesRead++] = client.read();
                            chunkTimeout = millis();
                        } else if (millis() - chunkTimeout > 10000) {
                            success = false;
                            break;
                        } else {
                            delay(1);
                        }
                    }
                    if (!success) break;
                    if (Update.write(buf, bytesRead) != bytesRead) {
                        success = false;
                        Update.printError(Serial);
                        break;
                    }
                    remaining -= bytesRead;
                }
                if (success && Update.end(true)) {
                    Serial.println("[EthWeb] OTA update complete");
                    ethSendJSON(client, 200, "{\"success\":true,\"message\":\"Update complete. Rebooting...\"}");
                    client.stop();
                    delay(1000);
                    ESP.restart();
                } else {
                    Update.printError(Serial);
                    Update.end();
                    ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Update failed\"}");
                }
            }
        }
    }
    
    // --- POST /api/system/reboot ---
    else if (path == "/api/system/reboot" && method == "POST") {
        ethSendJSON(client, 200, "{\"success\":true,\"message\":\"Rebooting...\"}");
        client.stop();
        delay(1000);
        ESP.restart();
    }
    
    // --- POST /api/system/factory ---
    else if (path == "/api/system/factory" && method == "POST") {
        wifiPref.end();
        wifiPref.begin("wifi", false);
        wifiPref.clear();
        wifiPref.end();
        
        ethernetPref.begin("ethernet", false);
        ethernetPref.clear();
        ethernetPref.end();
        
        mqttPref.begin("mqtt", false);
        mqttPref.clear();
        mqttPref.end();
        
        hmiPref.end();
        hmiPref.begin("hmi", false);
        hmiPref.clear();
        hmiPref.end();
        
        ethSendJSON(client, 200, "{\"success\":true,\"message\":\"Factory reset complete. Rebooting...\"}");
        client.stop();
        delay(1000);
        ESP.restart();
    }
    
    // --- GET /api/files/list ---
    else if (path == "/api/files/list" && method == "GET") {
        if (!fsManagerFFat.isFilesystemMounted()) {
            ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Filesystem not mounted\"}");
        } else {
            String filePath = urlDecode(getEthQueryParam(query, "path"));
            if (filePath.length() == 0) filePath = "/";
            if (!filePath.startsWith("/")) filePath = "/" + filePath;
            
            DynamicJsonDocument doc(4096);
            doc["success"] = true;
            doc["total"] = fsManagerFFat.totalBytes();
            doc["used"] = fsManagerFFat.usedBytes();
            doc["free"] = fsManagerFFat.freeBytes();
            doc["fsType"] = fsManagerFFat.getFilesystemName();
            doc["currentPath"] = filePath;
            
            JsonArray files = doc.createNestedArray("files");
            fs::FS *fs = fsManagerFFat.getActiveFilesystem();
            if (fs) {
                File root = fs->open(filePath);
                if (root && root.isDirectory()) {
                    File file = root.openNextFile();
                    while (file) {
                        JsonObject fileObj = files.createNestedObject();
                        // file.name() may return full path on newer ESP32 cores — strip path prefix
                        String fname = String(file.name());
                        int lastSlash = fname.lastIndexOf('/');
                        if (lastSlash >= 0) fname = fname.substring(lastSlash + 1);
                        fileObj["name"] = fname;
                        fileObj["size"] = file.size();
                        fileObj["isDir"] = file.isDirectory();
                        file = root.openNextFile();
                    }
                }
            }
            
            String response;
            serializeJson(doc, response);
            ethSendJSON(client, 200, response);
        }
    }
    
    // --- GET /api/files/read ---
    else if (path == "/api/files/read" && method == "GET") {
        if (!fsManagerFFat.isFilesystemMounted()) {
            ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Filesystem not mounted\"}");
        } else {
            String filePath = urlDecode(getEthQueryParam(query, "path"));
            if (filePath.length() == 0) {
                ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Missing path parameter\"}");
            } else {
                if (!filePath.startsWith("/")) filePath = "/" + filePath;
                String content = fsManagerFFat.readFile(filePath);
                if (content.length() == 0 && !fsManagerFFat.search(filePath)) {
                    ethSendJSON(client, 404, "{\"success\":false,\"message\":\"File not found\"}");
                } else {
                    size_t docSize = max((size_t)1024, content.length() * 2 + 512);
                    DynamicJsonDocument doc(docSize);
                    doc["success"] = true;
                    doc["content"] = content;
                    doc["path"] = filePath;
                    content = String();
                    String response;
                    serializeJson(doc, response);
                    ethSendJSON(client, 200, response);
                }
            }
        }
    }
    
    // --- POST /api/files/write ---
    else if (path == "/api/files/write" && method == "POST") {
        if (!fsManagerFFat.isFilesystemMounted()) {
            ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Filesystem not mounted\"}");
        } else {
            size_t docSize = max((size_t)1024, body.length() * 2 + 512);
            DynamicJsonDocument doc(docSize);
            DeserializationError error = deserializeJson(doc, body);
            if (error) {
                ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
            } else if (!doc.containsKey("path") || !doc.containsKey("content")) {
                ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Missing path or content\"}");
            } else {
                String filePath = doc["path"].as<String>();
                String content = doc["content"].as<String>();
                if (!filePath.startsWith("/")) filePath = "/" + filePath;
                if (fsManagerFFat.writeFile(filePath, content.c_str())) {
                    ethSendJSON(client, 200, "{\"success\":true,\"message\":\"File saved successfully\"}");
                } else {
                    ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Failed to write file\"}");
                }
            }
        }
    }
    
    // --- POST /api/files/upload ---
    else if (path == "/api/files/upload" && method == "POST") {
        if (!fsManagerFFat.isFilesystemMounted()) {
            ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Filesystem not mounted\"}");
        } else {
            // For large uploads, body may not have been read yet (contentLength >= 8192)
            if (body.length() == 0 && contentLength > 0) {
                body.reserve(min(contentLength, 65536));
                unsigned long bodyTimeout = millis();
                while ((int)body.length() < contentLength && (int)body.length() < 65536) {
                    if (client.available()) {
                        body += (char)client.read();
                        bodyTimeout = millis();
                    } else if (millis() - bodyTimeout > 5000) {
                        break;
                    } else {
                        delay(1);
                    }
                }
            }
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, body);
            if (error || !doc.containsKey("path") || !doc.containsKey("data")) {
                ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid upload data\"}");
            } else {
                String filePath = doc["path"].as<String>();
                if (!filePath.startsWith("/")) filePath = "/" + filePath;
                
                // Decode base64 data and write to file
                String b64Data = doc["data"].as<String>();
                doc.clear();  // Free JSON memory
                
                size_t decodedLen = 0;
                unsigned char *decoded = (unsigned char*)malloc(b64Data.length());
                if (!decoded) {
                    ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Out of memory\"}");
                } else {
                    int ret = mbedtls_base64_decode(decoded, b64Data.length(), &decodedLen,
                                                    (const unsigned char*)b64Data.c_str(), b64Data.length());
                    b64Data = "";  // Free base64 string memory
                    
                    if (ret != 0) {
                        free(decoded);
                        ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Base64 decode failed\"}");
                    } else {
                        fs::FS *fs = fsManagerFFat.getActiveFilesystem();
                        if (!fs) {
                            free(decoded);
                            ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Filesystem not available\"}");
                        } else {
                            if (fsManagerFFat.search(filePath)) {
                                fsManagerFFat.deleteFile(filePath);
                            }
                            File file = fs->open(filePath, FILE_WRITE);
                            if (!file) {
                                free(decoded);
                                ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Failed to create file\"}");
                            } else {
                                file.write(decoded, decodedLen);
                                file.close();
                                free(decoded);
                                Serial.printf("[EthWeb] File uploaded: %s (%d bytes)\n", filePath.c_str(), decodedLen);
                                ethSendJSON(client, 200, "{\"success\":true,\"message\":\"File uploaded successfully\"}");
                            }
                        }
                    }
                }
            }
        }
    }
    
    // --- GET /api/files/download ---
    else if (path == "/api/files/download" && method == "GET") {
        String filePath = urlDecode(getEthQueryParam(query, "path"));
        if (filePath.length() == 0) {
            ethSendHeader(client, 400, "text/plain", 22);
            client.print("Missing path parameter");
        } else {
            if (!filePath.startsWith("/")) filePath = "/" + filePath;
            if (!fsManagerFFat.isFilesystemMounted() || !fsManagerFFat.search(filePath)) {
                ethSendHeader(client, 404, "text/plain", 14);
                client.print("File not found");
            } else {
                fs::FS *fs = fsManagerFFat.getActiveFilesystem();
                if (!fs) {
                    ethSendHeader(client, 500, "text/plain", 24);
                    client.print("Filesystem not available");
                } else {
                    File file = fs->open(filePath, FILE_READ);
                    if (!file) {
                        ethSendHeader(client, 500, "text/plain", 19);
                        client.print("Failed to open file");
                    } else {
                        size_t fileSize = file.size();
                        String filename = filePath.substring(filePath.lastIndexOf('/') + 1);
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-Type: application/octet-stream");
                        client.print("Content-Disposition: attachment; filename=\"");
                        client.print(filename);
                        client.println("\"");
                        client.print("Content-Length: ");
                        client.println(fileSize);
                        client.println("Connection: close");
                        client.println();
                        
                        uint8_t buf[512];
                        while (file.available()) {
                            size_t bytesRead = file.read(buf, sizeof(buf));
                            client.write(buf, bytesRead);
                            delay(1);
                        }
                        file.close();
                    }
                }
            }
        }
    }
    
    // --- /api/files/delete ---
    else if (path == "/api/files/delete" && (method == "POST" || method == "DELETE" || method == "GET")) {
        String filePath = "";
        // Check query param first, then body
        if (query.length() > 0) {
            filePath = urlDecode(getEthQueryParam(query, "path"));
        }
        if (filePath.length() == 0 && body.length() > 0) {
            DynamicJsonDocument doc(256);
            if (!deserializeJson(doc, body) && doc.containsKey("path")) {
                filePath = doc["path"].as<String>();
            }
        }
        
        if (filePath.length() == 0) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Missing path parameter\"}");
        } else {
            if (!filePath.startsWith("/")) filePath = "/" + filePath;
            if (!fsManagerFFat.isFilesystemMounted() || !fsManagerFFat.search(filePath)) {
                ethSendJSON(client, 404, "{\"success\":false,\"message\":\"File not found\"}");
            } else {
                fs::FS *fs = fsManagerFFat.getActiveFilesystem();
                bool deleted = false;
                if (fs) {
                    File file = fs->open(filePath);
                    if (file) {
                        bool isDir = file.isDirectory();
                        file.close();
                        deleted = isDir ? fsManagerFFat.deleteDir(filePath) : fsManagerFFat.deleteFile(filePath);
                    }
                }
                if (!deleted) deleted = fsManagerFFat.deleteFile(filePath);
                
                if (deleted) {
                    ethSendJSON(client, 200, "{\"success\":true,\"message\":\"Deleted successfully\"}");
                } else {
                    ethSendJSON(client, 500, "{\"success\":false,\"message\":\"Failed to delete\"}");
                }
            }
        }
    }
    
    // --- GET/POST /api/io/config ---
    else if (path == "/api/io/config" && method == "GET") {
        DynamicJsonDocument doc(256);
        doc["success"] = true;
        doc["input_enabled"]  = settingsPref.getBool("input_enabled",  false);
        doc["output_enabled"] = settingsPref.getBool("output_enabled", false);
        doc["usb_enabled"]    = settingsPref.getBool("usb_enabled",    false);
        String resp; serializeJson(doc, resp);
        ethSendJSON(client, 200, resp);
    }
    else if (path == "/api/io/config" && method == "POST") {
        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, body)) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            settingsPref.end();
            settingsPref.begin("settings", false);
            if (doc.containsKey("input_enabled"))  settingsPref.putBool("input_enabled",  doc["input_enabled"]);
            if (doc.containsKey("output_enabled")) settingsPref.putBool("output_enabled", doc["output_enabled"]);
            if (doc.containsKey("usb_enabled"))    settingsPref.putBool("usb_enabled",    doc["usb_enabled"]);
            settingsPref.end();
            settingsPref.begin("settings", true);
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"IO config saved. Reboot to apply.\"}");
        }
    }
    // --- GET/POST /api/filesystem/config ---
    else if (path == "/api/filesystem/config" && method == "GET") {
        DynamicJsonDocument doc(256);
        doc["success"]    = true;
        doc["fs_enabled"] = settingsPref.getBool("fs_enabled", false);
        String resp; serializeJson(doc, resp);
        ethSendJSON(client, 200, resp);
    }
    else if (path == "/api/filesystem/config" && method == "POST") {
        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, body)) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            settingsPref.end();
            settingsPref.begin("settings", false);
            if (doc.containsKey("fs_enabled")) settingsPref.putBool("fs_enabled", doc["fs_enabled"]);
            settingsPref.end();
            settingsPref.begin("settings", true);
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"Filesystem config saved. Reboot to apply.\"}");
        }
    }
    // --- POST /api/filesystem/format ---
    else if (path == "/api/filesystem/format" && method == "POST") {
        bool ok = fsManagerFFat.format();
        ethSendJSON(client, ok ? 200 : 500,
            ok ? "{\"success\":true,\"message\":\"Filesystem formatted. Reboot to remount.\"}"
               : "{\"success\":false,\"message\":\"Format failed\"}");
    }
    // --- GET/POST /api/web/settings ---
    else if (path == "/api/web/settings" && method == "GET") {
        DynamicJsonDocument doc(256);
        doc["success"]      = true;
        doc["username"]     = web_username;
        doc["auth_enabled"] = web_auth_enabled;
        String resp; serializeJson(doc, resp);
        ethSendJSON(client, 200, resp);
    }
    else if (path == "/api/web/settings" && method == "POST") {
        DynamicJsonDocument doc(256);
        if (deserializeJson(doc, body)) {
            ethSendJSON(client, 400, "{\"success\":false,\"message\":\"Invalid JSON\"}");
        } else {
            if (doc.containsKey("username")) web_username = doc["username"].as<String>();
            if (doc.containsKey("password") && doc["password"].as<String>().length() > 0)
                web_password = doc["password"].as<String>();
            if (doc.containsKey("auth_enabled")) web_auth_enabled = doc["auth_enabled"];
            ethSendJSON(client, 200, "{\"success\":true,\"message\":\"Web settings saved\"}");
        }
    }
    // --- 404 Not Found ---
    else {
        ethSendJSON(client, 404, "{\"success\":false,\"message\":\"Not found\"}");
    }
    
    delay(1);
    client.stop();
}


// ==================== ETHERNET WEB LOOP HANDLER ====================

void handleEthWeb() {
    if (!ethWebServerStarted) {
        if (Ethernet.linkStatus() == LinkON) {
            Serial.println("[EthWeb] Ethernet link detected, starting Ethernet HTTP server...");
            setupEthWebServer();
        }
    }
    handleEthWebClients();
}

#endif // WEB_CONFIGURATION_H
