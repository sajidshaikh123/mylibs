# OPC UA Server Implementation Guide

## Current Implementation

The current `opcua_server.h` is a **simplified JSON-based TCP server**, NOT a real OPC UA protocol implementation.

### What it supports:
- ✅ Simple JSON commands over TCP (telnet, netcat, custom clients)
- ✅ Lightweight (low memory usage)
- ✅ Easy to use and understand
- ❌ NOT compatible with standard OPC UA clients (UAExpert, Prosys, etc.)

### JSON Protocol Commands:

```json
{"action":"browse"}
{"action":"info"}
{"action":"read","nodeId":"temperature"}
{"action":"write","nodeId":"relay1","value":"true"}
```

## Testing with Current Implementation

### Method 1: Telnet (Windows)
```powershell
telnet 192.168.1.4 4840
{"action":"browse"}
```

### Method 2: PowerShell
```powershell
$client = New-Object System.Net.Sockets.TcpClient("192.168.1.4", 4840)
$stream = $client.GetStream()
$writer = New-Object System.IO.StreamWriter($stream)
$reader = New-Object System.IO.StreamReader($stream)

$writer.WriteLine('{"action":"browse"}')
$writer.Flush()
$response = $reader.ReadLine()
Write-Host $response
$client.Close()
```

### Method 3: Python Client
```python
import socket
import json

def opcua_request(ip, port, action, nodeId=None, value=None):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((ip, port))
    
    request = {"action": action}
    if nodeId:
        request["nodeId"] = nodeId
    if value:
        request["value"] = value
    
    sock.send((json.dumps(request) + "\n").encode())
    response = sock.recv(4096).decode()
    sock.close()
    
    return json.loads(response)

# Usage
print(opcua_request("192.168.1.4", 4840, "browse"))
print(opcua_request("192.168.1.4", 4840, "read", nodeId="temperature"))
print(opcua_request("192.168.1.4", 4840, "write", nodeId="relay1", value="true"))
```

### Method 4: Node.js Client
```javascript
const net = require('net');

function opcuaRequest(ip, port, action, nodeId, value) {
    return new Promise((resolve, reject) => {
        const client = net.connect(port, ip, () => {
            const request = { action };
            if (nodeId) request.nodeId = nodeId;
            if (value) request.value = value;
            
            client.write(JSON.stringify(request) + '\n');
        });
        
        client.on('data', (data) => {
            resolve(JSON.parse(data.toString()));
            client.end();
        });
        
        client.on('error', reject);
    });
}

// Usage
(async () => {
    const nodes = await opcuaRequest('192.168.1.4', 4840, 'browse');
    console.log(nodes);
    
    const temp = await opcuaRequest('192.168.1.4', 4840, 'read', 'temperature');
    console.log(temp);
})();
```

## For Real OPC UA Client Support

To use standard OPC UA clients (UAExpert, Prosys OPC UA Browser), you need:

### Option 1: Install open62541 Library

1. Download open62541 for ESP32: https://github.com/open62541/open62541
2. Add to Arduino libraries folder
3. Use `opcua_server_open62541.h` instead

### Option 2: Python Bridge (Recommended for testing)

Create a bridge server on your PC:

```python
# opcua_bridge.py
from opcua import Server
import socket
import json
import threading

# Create real OPC UA server
server = Server()
server.set_endpoint("opc.tcp://0.0.0.0:4841")
server.set_server_name("ESP32 Bridge")

uri = "http://embedsol.com"
idx = server.register_namespace(uri)

objects = server.get_objects_node()

# Connect to ESP32 JSON server
esp_ip = "192.168.1.4"
esp_port = 4840

def get_esp_nodes():
    sock = socket.socket()
    sock.connect((esp_ip, esp_port))
    sock.send(b'{"action":"browse"}\n')
    response = json.loads(sock.recv(4096).decode())
    sock.close()
    return response

def read_esp_value(nodeId):
    sock = socket.socket()
    sock.connect((esp_ip, esp_port))
    request = {"action": "read", "nodeId": nodeId}
    sock.send((json.dumps(request) + "\n").encode())
    response = json.loads(sock.recv(4096).decode())
    sock.close()
    return response

# Create OPC UA variables from ESP32 nodes
esp_nodes = get_esp_nodes()
for node in esp_nodes['nodes']:
    var = objects.add_variable(idx, node['nodeId'], 0)
    var.set_writable()

server.start()
print("OPC UA Bridge running on opc.tcp://localhost:4841")
print("Connect your OPC UA client to this address")

try:
    while True:
        time.sleep(1)
finally:
    server.stop()
```

Install required library:
```bash
pip install opcua
```

Run the bridge:
```bash
python opcua_bridge.py
```

Now connect UAExpert to: `opc.tcp://localhost:4841`

## Comparison

| Feature | Current (JSON) | Real OPC UA (open62541) | Python Bridge |
|---------|---------------|------------------------|---------------|
| Memory Usage | Low (~10KB) | High (~100KB+) | Low (on PC) |
| Standard Clients | ❌ No | ✅ Yes | ✅ Yes |
| Easy to Use | ✅ Yes | ❌ Complex | ✅ Yes |
| Security | ❌ None | ✅ Full | ✅ Full |
| Discovery | ❌ No | ✅ Yes | ✅ Yes |
| Best For | Custom apps | Production | Testing |

## Recommendations

1. **For custom applications**: Use current JSON implementation - simple and lightweight
2. **For testing with OPC UA clients**: Use Python bridge
3. **For production with OPC UA**: Implement open62541 or use industrial gateway

## Troubleshooting

### Error: "BadCommunicationError"
- Means the OPC UA client can't understand the JSON protocol
- Use one of the solutions above

### Error: "Connection Refused"
- Check firewall settings
- Verify IP address and port
- Ensure server is running (check Serial monitor)

### Error: "Timeout"
- Network issue
- Try telnet first to test basic connectivity
