# TCP Modbus W5500 Implementation

## Overview
This implementation uses the `modbus-esp8266` library which is compatible with:
- ESP32 WiFi
- W5500 SPI Ethernet (via Arduino Ethernet library)
- Any Arduino-compatible network interface

## Installation

1. Install the `modbus-esp8266` library:
   - Open Arduino IDE
   - Go to Tools → Manage Libraries
   - Search for "modbus-esp8266"
   - Install "ModbusIP ESP8266/ESP32" by Andre Sarmento Barbosa

2. The library is already integrated into your project via `tcp_modbus_w5500.h`

## Features

✅ Works with W5500 Ethernet (192.168.1.120)
✅ Works with ESP32 WiFi
✅ Auto-detects active network interface
✅ Supports all Modbus register types:
   - Holding Registers (Read/Write)
   - Input Registers (Read-only)
   - Coils (Read/Write bits)
   - Discrete Inputs (Read-only bits)

## Configuration Commands

```
tcpmodbus enable              # Enable Modbus TCP server
tcpmodbus disable             # Disable Modbus TCP server
tcpmodbus slaveid <1-247>     # Set Modbus slave/unit ID
tcpmodbus port <502>          # Set TCP port (default: 502)
tcpmodbus transport ethernet  # Use Ethernet (W5500)
tcpmodbus transport wifi      # Use WiFi
tcpmodbus transport auto      # Auto-detect (prefers Ethernet)
tcpmodbus status              # Show current status
tcpmodbus show                # Show saved configuration
```

## Quick Start

### Enable Modbus on Ethernet (W5500)

```
tcpmodbus enable
tcpmodbus transport ethernet
tcpmodbus slaveid 1
tcpmodbus port 502
```

Then reboot your device. The Modbus TCP server will start on your Ethernet IP (192.168.1.120:502).

### Enable Modbus on WiFi

```
tcpmodbus enable
tcpmodbus transport wifi
tcpmodbus slaveid 1
```

## Testing Connection

Use any Modbus TCP client to test:

### Example using mbpoll (Linux/Mac/Windows):
```bash
# Read holding register 0
mbpoll -a 1 -r 0 -c 1 -t 4 192.168.1.120

# Write holding register 0 with value 100
mbpoll -a 1 -r 0 -t 4:int 192.168.1.120 100
```

### Example using Python (pymodbus):
```python
from pymodbus.client import ModbusTcpClient

client = ModbusTcpClient('192.168.1.120', port=502)
client.connect()

# Read holding register 0
result = client.read_holding_registers(0, 1, slave=1)
print(f"Register 0 value: {result.registers[0]}")

# Write holding register 0
client.write_register(0, 1234, slave=1)

client.close()
```

## API Functions

### Holding Registers (40001-40100)
```cpp
tcpModbusSetHoldingRegister(address, value);
uint16_t value = tcpModbusGetHoldingRegister(address);
tcpModbusSetHoldingFloat(address, floatValue);  // Uses 2 registers
float value = tcpModbusGetHoldingFloat(address);
```

### Input Registers (30001-30100)
```cpp
tcpModbusSetInputRegister(address, value);
uint16_t value = tcpModbusGetInputRegister(address);
```

### Coils (00001-00064)
```cpp
tcpModbusSetCoil(address, true/false);
bool state = tcpModbusGetCoil(address);
```

### Discrete Inputs (10001-10064)
```cpp
tcpModbusSetDiscreteInput(address, true/false);
bool state = tcpModbusGetDiscreteInput(address);
```

## Register Map (Current Implementation)

| Type | Modbus Address | Count | Description |
|------|---------------|-------|-------------|
| Holding Registers | 40001-40100 | 100 | Read/Write |
| Input Registers | 30001-30100 | 100 | Read-only |
| Coils | 00001-00064 | 64 | Read/Write bits |
| Discrete Inputs | 10001-10064 | 64 | Read-only bits |

## Example: Counter in Holding Register 0

Your current code increments a counter in holding register 0 every 5 seconds:

```cpp
void loop() {
    boardloop();
    
    if (millis() - lastModbusUpdate >= 5000) {
        lastModbusUpdate = millis();
        counter = tcpModbusGetHoldingRegister(0);
        counter++;
        tcpModbusSetHoldingRegister(0, counter);
        Serial.printf("[Modbus] Holding Reg 0 = %d\n", counter);
    }
}
```

You can read this counter from any Modbus TCP client!

## Troubleshooting

### "No active network interface"
- Make sure either WiFi or Ethernet is connected
- Check WiFi credentials or Ethernet cable
- Verify IP address is assigned

### Connection refused from client
- Verify firewall settings on ESP32 (if any)
- Check that port 502 is not blocked
- Confirm slave ID matches in client and server
- Test with: `ping 192.168.1.120`

### Library not found error
- Install `modbus-esp8266` library from Arduino Library Manager
- Restart Arduino IDE after installation

## Advantages over ESP-IDF Modbus

✅ Works with W5500 SPI Ethernet
✅ Works with any Arduino Client object
✅ Simpler API
✅ No esp_netif dependency
✅ Better compatibility across platforms

## Migration Notes

If you were using the old `tcp_modbus.h`:
- All register access functions remain the same
- Configuration commands are slightly simplified
- Master/client mode removed (server/slave only)
- No breaking changes to your application code
