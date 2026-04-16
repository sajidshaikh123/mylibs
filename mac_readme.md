# MAC-Level MQTT Command Reference

All MAC-level commands are sent to the topic:

```
any_prefix/{DEVICE_MAC_ADDRESS}/{command}
```

Where `{DEVICE_MAC_ADDRESS}` is the WiFi MAC address of the device (e.g., `AA:BB:CC:DD:EE:FF`).

All responses are published to `{MAC}/{command}/status` (or similar as noted below).

---

## 1. Subtopic Configuration

Set the MQTT topic hierarchy for the device.

**Topic:** `…/{MAC}/subtopic`

**Response Topic:** `{MAC}/subtopic/status`

**Storage:** Preferences (NVS), namespace `"subtopic"`

| NVS Key      | Source Field    |
|--------------|-----------------|
| `company`    | `company_name`  |
| `location`   | `location`      |
| `department` | `department`    |
| `line`       | `line`          |
| `machine`    | `machinename`   |

### Payload (without line)

```json
{
  "company_name": "Embedsol",
  "location": "Ahmedabad",
  "department": "Production",
  "machinename": "CNC_01"
}
```

### Payload (with optional line)

```json
{
  "company_name": "Embedsol",
  "location": "Ahmedabad",
  "department": "Production",
  "line": "Line_A",
  "machinename": "CNC_01"
}
```

### Validation Rules

| Field          | Required | Max Length | Allowed Characters               |
|----------------|----------|------------|-----------------------------------|
| `company_name` | Yes      | 50         | Alphanumeric, `_`, `-`, space     |
| `location`     | Yes      | 50         | Alphanumeric, `_`, `-`, space     |
| `department`   | Yes      | 50         | Alphanumeric, `_`, `-`, space     |
| `line`         | No       | 50         | Alphanumeric, `_`, `-`, space     |
| `machinename`  | Yes      | 50         | Alphanumeric, `_`, `-`, space     |

All fields must be non-empty (except `line` which is optional).

### Success Response

```json
{ "success": true, "message": "Subtopic configuration updated successfully" }
```

### Error Response

```json
{ "success": false, "message": "Invalid subtopic configuration" }
```

---

## 2. Ethernet Configuration

Configure Ethernet network settings.

**Topic:** `…/{MAC}/ethernet`

**Response Topic:** `{MAC}/ethernet/status`

**Storage:** Preferences (NVS), namespace `"ethernet"`

| NVS Key    | Type   | Source                          |
|------------|--------|---------------------------------|
| `enabled`  | bool   | `enabled` (default: `true`)     |
| `dhcp`     | bool   | `dhcp`                          |
| `ip`       | String | Validated IP                    |
| `gateway`  | String | Validated gateway               |
| `subnet`   | String | Validated subnet                |
| `dns`      | String | Validated DNS                   |
| `speed`    | String | `speed` (if present)            |
| `duplex`   | String | `duplex` (if present)           |

### Payload — DHCP Mode

```json
{
  "enabled": true,
  "dhcp": true
}
```

### Payload — Static IP Mode

```json
{
  "enabled": true,
  "dhcp": false,
  "staticIp": "192.168.1.100",
  "staticGateway": "192.168.1.1",
  "staticSubnet": "255.255.255.0",
  "staticDns": "8.8.8.8",
  "speed": "100",
  "duplex": "full"
}
```

### Validation Rules

| Field           | Required         | Values / Format                     |
|-----------------|------------------|-------------------------------------|
| `dhcp`          | Yes              | `true` / `false`                    |
| `enabled`       | No (default `true`) | `true` / `false`                 |
| `staticIp`      | If `dhcp=false`  | Valid IPv4 (e.g., `192.168.1.100`)  |
| `staticGateway` | If `dhcp=false`  | Valid IPv4                          |
| `staticSubnet`  | If `dhcp=false`  | Valid IPv4                          |
| `staticDns`     | No               | Valid IPv4 (default: `8.8.8.8`)     |
| `speed`         | No               | `"auto"`, `"10"`, `"100"`          |
| `duplex`        | No               | `"auto"`, `"half"`, `"full"`       |

IP validation: `xxx.xxx.xxx.xxx` format, octets 0–255, no leading zeros.

### Success Response

```json
{ "success": true, "message": "Ethernet configuration saved to Preferences" }
```

### Error Response

```json
{ "success": false, "message": "Invalid ethernet configuration" }
```

---

## 3. WiFi Configuration

Configure WiFi network settings.

**Topic:** `…/{MAC}/wifi`

**Response Topic:** `{MAC}/wifi/status`

**Storage:** Preferences (NVS), namespace `"wifi"`

| NVS Key    | Type   | Source                          |
|------------|--------|---------------------------------|
| `enabled`  | bool   | `enabled` (default: `true`)     |
| `ssid`     | String | `ssid`                          |
| `password` | String | `password`                      |
| `dhcp`     | bool   | `dhcp` (default: `true`)        |
| `ip`       | String | `ip` (if static)                |
| `gateway`  | String | `gateway` (if static)           |
| `subnet`   | String | `subnet` (if static)            |
| `dns`      | String | `dns` (if static)               |

### Payload — DHCP Mode

```json
{
  "enabled": true,
  "ssid": "MyNetwork",
  "password": "12345678",
  "dhcp": true
}
```

### Payload — Static IP Mode

```json
{
  "enabled": true,
  "ssid": "MyNetwork",
  "password": "12345678",
  "dhcp": false,
  "ip": "192.168.1.25",
  "gateway": "192.168.1.1",
  "subnet": "255.255.255.0",
  "dns": "8.8.8.8"
}
```

### Validation Rules

| Field      | Required           | Constraints                         |
|------------|--------------------|-------------------------------------|
| `ssid`     | Yes                | 1–32 characters                     |
| `password` | No                 | 8–63 characters (or empty for open) |
| `enabled`  | No (default `true`)| `true` / `false`                    |
| `dhcp`     | No (default `true`)| `true` / `false`                    |
| `ip`       | If `dhcp=false`    | Valid IPv4                          |
| `gateway`  | If `dhcp=false`    | Valid IPv4                          |
| `subnet`   | If `dhcp=false`    | Valid IPv4                          |
| `dns`      | No                 | Valid IPv4 (default: `8.8.8.8`)     |

### Success Response

```json
{ "success": true, "message": "WiFi configuration saved to Preferences" }
```

### Error Response

```json
{ "success": false, "message": "Password must be at least 8 characters" }
```

---

## 4. RTC (Real-Time Clock)

Set the device date and time.

**Topic:** `…/{MAC}/rtc`

**Response Topic:** `{MAC}/rtc/set`

### Payload

```json
{
  "date": 15,
  "month": 4,
  "year": 2026,
  "hours": 14,
  "minutes": 30,
  "seconds": 0
}
```

### Fields

| Field     | Type   | Range       |
|-----------|--------|-------------|
| `date`    | uint8  | 1–31        |
| `month`   | uint8  | 1–12        |
| `year`    | uint16 | e.g., 2026  |
| `hours`   | uint8  | 0–23        |
| `minutes` | uint8  | 0–59        |
| `seconds` | uint8  | 0–59        |

### Success Response

```json
{ "success": true }
```

---

## 5. OTA Firmware Update

Trigger over-the-air firmware update. *(Currently disabled/commented out in code)*

**Topic:** `…/{MAC}/ota_update`

**Response Topic:** `{MAC}/ota/status`

### Payload

```json
{
  "url": "http://192.168.1.10/firmware.bin",
  "host": "192.168.1.10",
  "port": 80,
  "path": "/firmware.bin"
}
```

### Fields

| Field  | Required                  | Description                  |
|--------|---------------------------|------------------------------|
| `url`  | Yes (if host/path absent) | Full firmware URL            |
| `host` | Yes (if url absent)       | Server hostname or IP        |
| `port` | No (default: 80)         | Server port                  |
| `path` | Yes (if url absent)       | File path on server          |

Provide either `url` alone, or `host` + `path` + optional `port`.

### Response (during update)

```json
{
  "status": "success",
  "message": "Firmware update completed successfully",
  "timestamp": "2026-04-15 14:30:00",
  "device_mac": "AA:BB:CC:DD:EE:FF",
  "running_partition": "app0",
  "free_heap": 180000,
  "uptime_ms": 60000
}
```

---

## 6. System Info

Request device system information.

**Topic:** `…/{MAC}/system_info`

**Response Topic:** `{MAC}/system/info` (retained)

### Payload

Any payload (content ignored). Send empty string `""` or `{}`.

### Response

```json
{
  "timestamp": "2026-04-15 14:30:00",
  "device_mac": "AA:BB:CC:DD:EE:FF",
  "free_heap": 180000,
  "uptime_ms": 3600000,
  "chip_model": "ESP32-S3",
  "chip_revision": 0,
  "cpu_freq_mhz": 240,
  "flash_size": 16777216,
  "sketch_size": 1500000,
  "free_sketch_space": 6000000
}
```

---

## 7. Reboot

Remotely restart the device.

**Topic:** `…/{MAC}/reboot`

**Response Topic:** `{MAC}/reboot/status`

### Payload

Any payload (content ignored). Send empty string `""` or `{}`.

### Response

```
rebooting
```

Device restarts after 1 second delay.

---

## NVS Namespace Summary

| Namespace    | Used By    | Keys                                                        |
|--------------|------------|-------------------------------------------------------------|
| `subtopic`   | Subtopic   | `company`, `location`, `department`, `line`, `machine`      |
| `ethernet`   | Ethernet   | `enabled`, `dhcp`, `ip`, `gateway`, `subnet`, `dns`, `speed`, `duplex` |
| `wifi`       | WiFi       | `enabled`, `ssid`, `password`, `dhcp`, `ip`, `gateway`, `subnet`, `dns` |

---

## Common Error Responses

All handlers return JSON with `success` and `message` fields on error:

```json
{ "success": false, "message": "<error description>" }
```

| Error                        | Cause                                    |
|------------------------------|------------------------------------------|
| `Insufficient memory`        | Free heap < 5000 bytes (subtopic only)   |
| `JSON parsing failed: ...`   | Invalid JSON in payload                  |
| `Missing required field: X`  | Required field not present               |
| `Invalid X length`           | String too short or too long             |
| `Invalid characters in X`    | Disallowed characters in field           |
| `Invalid IP: x.x.x.x`       | Malformed IP address                     |
| `Password must be at least 8 characters` | WiFi password too short       |

---

## Quick Example

Configure a device with MAC `AA:BB:CC:DD:EE:FF`:

```
# Set subtopic
Topic:   any/AA:BB:CC:DD:EE:FF/subtopic
Payload: {"company_name":"Embedsol","location":"Ahmedabad","department":"Production","machinename":"CNC_01"}

# Set WiFi (DHCP)
Topic:   any/AA:BB:CC:DD:EE:FF/wifi
Payload: {"enabled":true,"ssid":"Factory_WiFi","password":"securepass123","dhcp":true}

# Set Ethernet (Static IP)
Topic:   any/AA:BB:CC:DD:EE:FF/ethernet
Payload: {"enabled":true,"dhcp":false,"staticIp":"192.168.1.100","staticGateway":"192.168.1.1","staticSubnet":"255.255.255.0","staticDns":"8.8.8.8"}

# Set RTC
Topic:   any/AA:BB:CC:DD:EE:FF/rtc
Payload: {"date":15,"month":4,"year":2026,"hours":14,"minutes":30,"seconds":0}

# Get system info
Topic:   any/AA:BB:CC:DD:EE:FF/system_info
Payload: {}

# Reboot
Topic:   any/AA:BB:CC:DD:EE:FF/reboot
Payload: {}
```
