#!/usr/bin/env python3
"""
Simple test client for ESP32 JSON OPC UA Server

Usage:
    python test_client.py 192.168.1.4 4840
"""

import socket
import json
import sys

def send_request(ip, port, request):
    """Send JSON request to ESP32 server"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((ip, port))
        
        # Send request
        sock.send((json.dumps(request) + "\n").encode())
        
        # Receive response
        response = sock.recv(4096).decode().strip()
        sock.close()
        
        return json.loads(response)
    except Exception as e:
        print(f"Error: {e}")
        return None

def test_server(ip, port):
    print("=" * 60)
    print(f"Testing ESP32 JSON OPC UA Server at {ip}:{port}")
    print("=" * 60)
    
    # Test 1: Get server info
    print("\n1. Getting server info...")
    response = send_request(ip, port, {"action": "info"})
    if response:
        print(f"   Server: {response.get('server')}")
        print(f"   URI: {response.get('uri')}")
        print(f"   Version: {response.get('version')}")
        print(f"   Nodes: {response.get('nodes')}")
    
    # Test 2: Browse all nodes
    print("\n2. Browsing all nodes...")
    response = send_request(ip, port, {"action": "browse"})
    if response and response.get('status') == 'success':
        nodes = response.get('nodes', [])
        print(f"   Found {len(nodes)} nodes:")
        for node in nodes:
            writable = "RW" if node.get('writable') else "RO"
            print(f"   - {node.get('nodeId'):30s} [{node.get('type'):8s}] ({writable}) = {node.get('value')}")
    
    # Test 3: Read specific node
    print("\n3. Reading temperature node...")
    response = send_request(ip, port, {"action": "read", "nodeId": "sensors.temperature"})
    if response:
        print(f"   Value: {response.get('value')} (Type: {response.get('type')})")
    
    # Test 4: Write to writable node
    print("\n4. Writing to relay1 node...")
    response = send_request(ip, port, {"action": "write", "nodeId": "control.relay1", "value": "true"})
    if response:
        print(f"   Status: {response.get('status')}")
        print(f"   Message: {response.get('message')}")
    
    # Test 5: Read back the written value
    print("\n5. Reading back relay1 value...")
    response = send_request(ip, port, {"action": "read", "nodeId": "control.relay1"})
    if response:
        print(f"   Value: {response.get('value')}")
    
    print("\n" + "=" * 60)
    print("Test completed!")
    print("=" * 60)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python test_client.py <IP_ADDRESS> [PORT]")
        print("Example: python test_client.py 192.168.1.4 4840")
        sys.exit(1)
    
    ip = sys.argv[1]
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 4840
    
    test_server(ip, port)
