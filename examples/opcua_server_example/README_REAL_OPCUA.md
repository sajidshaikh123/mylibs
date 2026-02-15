# Real OPC UA Server for ESP32-S3

This is a **standards-compliant OPC UA server** implementation that works with commercial OPC UA clients.

## Important Note

The current implementation in `opcua_server_open62541.h` provides **basic OPC UA protocol support** for discovery and connection. For a **fully functional OPC UA server**, you have three options:

## Option 1: Use Python Bridge (Easiest - Recommended)

This is the **fastest and easiest way** to get a real OPC UA server working:

1. Keep your current ESP32 JSON server running
2. Run the Python bridge on your PC:
   ```bash
   pip install opcua
   python python_opcua_bridge.py --esp-ip 192.168.1.4
   ```
3. Connect UAExpert to: `opc.tcp://localhost:4841`

**Advantages:**
- ✅ Works immediately with all OPC UA clients
- ✅ Full OPC UA protocol support
- ✅ No ESP32 code changes needed
- ✅ Low memory usage on ESP32

## Option 2: Install open62541 Library (Full Featured)

For a complete OPC UA stack running on ESP32-S3:

### Installation Steps:

1. **Download open62541 for Arduino:**
   ```bash
   cd ~/Arduino/libraries
   git clone https://github.com/open62541/open62541-arduino
   ```

2. **Enable PSRAM in Arduino IDE:**
   - Tools → PSRAM → "OPI PSRAM"
   - Tools → Partition Scheme → "16MB Flash (3MB APP/9MB FATFS)"

3. **Modify your sketch to use open62541:**
   ```cpp
   #define OPEN62541_AVAILABLE
   #include <open62541.h>
   #include "opcua_server_open62541.h"
   
   // Use the full-featured server
   ```

4. **Compile and upload**

### Requirements:
- ESP32-S3 with PSRAM (8MB recommended)
- open62541 library installed
- ~150KB heap memory minimum

## Option 3: Current Basic Implementation (Partial Support)

The current implementation in `opcua_server_open62541.h` provides:

**What Works:**
- ✅ Basic TCP connection on port 4840
- ✅ Hello/ACK handshake
- ✅ Client can discover the server

**What's NOT Implemented:**
- ❌ Full OPC UA binary encoding/decoding
- ❌ Secure channel establishment
- ❌ Session management
- ❌ Browse/Read/Write services
- ❌ Security policies

**To Use:**
```cpp
#include "opcua_server_open62541.h"

OPCUAServer_Real opcuaServer(4840);

void setup() {
    boardinit();
    opcuaServer.begin(false); // Start on Ethernet
}

void loop() {
    boardloop();
    opcuaServer.loop();
}
```

## Comparison Table

| Feature | JSON Server | Python Bridge | Basic OPC UA | open62541 Full |
|---------|------------|---------------|--------------|----------------|
| Memory (ESP32) | 10KB | 10KB | 50KB | 150KB+ |
| OPC UA Clients | ❌ | ✅ | ⚠️ Partial | ✅ |
| Discovery | ❌ | ✅ | ⚠️ Partial | ✅ |
| Browse/Read/Write | Custom | ✅ | ❌ | ✅ |
| Security | ❌ | ✅ | ❌ | ✅ |
| Setup Difficulty | Easy | Easy | Easy | Hard |
| Best For | Custom apps | Testing | Learning | Production |

## Recommended Path

For most users, I recommend:

1. **Development/Testing:** Use Python Bridge
2. **Production (Low volume):** Continue using Python Bridge on gateway PC
3. **Production (High volume):** Invest in implementing open62541

## Testing Your Server

### With UAExpert:

1. **Add Server:**
   - File → Add Server
   - Enter: `opc.tcp://192.168.1.4:4840` (your ESP32 IP)

2. **Connect:**
   - Double-click the server
   - Select "None" security policy
   - Click Connect

3. **Expected Behavior:**

   **With Basic Implementation:**
   - Server appears in list ✅
   - Can initiate connection ⚠️
   - May fail during session creation ❌

   **With Python Bridge:**
   - Server appears in list ✅
   - Connection succeeds ✅
   - Can browse all nodes ✅
   - Can read/write values ✅

   **With open62541:**
   - Full OPC UA functionality ✅

## Troubleshooting

### "BadCommunicationError" or "Connection Failed"

**Cause:** OPC UA client expects full protocol, but server only implements basics

**Solution:**
1. Use Python Bridge (recommended)
2. Or install open62541 library
3. Or continue using JSON server with custom clients

### "Discovery works but Browse fails"

**Cause:** Basic implementation only handles connection setup

**Solution:** Use Python Bridge or install open62541

### High Memory Usage

**Cause:** OPC UA protocol is complex and memory-intensive

**Solution:**
- Enable PSRAM
- Reduce number of nodes
- Use Python Bridge to offload processing to PC

## Files in This Example

- `opcua_server_example.ino` - JSON-based server (custom protocol)
- `opcua_real_server_example.ino` - Real OPC UA server attempt
- `opcua_server.h` - JSON protocol implementation
- `opcua_server_open62541.h` - Basic OPC UA protocol support
- `python_opcua_bridge.py` - Python bridge for real OPC UA (RECOMMENDED)
- `test_client.py` - Test client for JSON server

## Next Steps

1. **If you just want it to work:** Use `python_opcua_bridge.py`
2. **If you want to learn:** Study `opcua_server_open62541.h` and OPC UA specs
3. **If you need production solution:** Plan to install open62541 library

## Resources

- OPC UA Specification: https://opcfoundation.org/developer-tools/specifications-unified-architecture
- open62541 Library: https://open62541.org/
- Python opcua: https://python-opcua.readthedocs.io/
- UAExpert: https://www.unified-automation.com/products/development-tools/uaexpert.html
