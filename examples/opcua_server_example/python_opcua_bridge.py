#!/usr/bin/env python3
"""
OPC UA Bridge for ESP32 JSON Server

This script creates a real OPC UA server that bridges to the ESP32 JSON server.
Standard OPC UA clients (UAExpert, Prosys) can connect to this bridge.

Installation:
    pip install opcua asyncio

Usage:
    python python_opcua_bridge.py --esp-ip 192.168.1.4 --esp-port 4840 --opcua-port 4841
"""

import socket
import json
import time
import argparse
from opcua import Server, ua

class ESP32Bridge:
    def __init__(self, esp_ip, esp_port, opcua_port=4841):
        self.esp_ip = esp_ip
        self.esp_port = esp_port
        self.opcua_port = opcua_port
        
        # Create OPC UA Server
        self.server = Server()
        self.server.set_endpoint(f"opc.tcp://0.0.0.0:{opcua_port}")
        self.server.set_server_name("ESP32 OPC UA Bridge")
        
        # Setup namespace
        uri = "http://embedsol.com/esp32"
        self.idx = self.server.register_namespace(uri)
        self.objects = self.server.get_objects_node()
        
        # Store node mappings
        self.node_map = {}
        
    def esp_request(self, action, nodeId=None, value=None):
        """Send request to ESP32 JSON server"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((self.esp_ip, self.esp_port))
            
            request = {"action": action}
            if nodeId:
                request["nodeId"] = nodeId
            if value is not None:
                request["value"] = str(value)
            
            sock.send((json.dumps(request) + "\n").encode())
            response = sock.recv(4096).decode().strip()
            sock.close()
            
            return json.loads(response)
        except Exception as e:
            print(f"Error communicating with ESP32: {e}")
            return None
    
    def create_nodes(self):
        """Create OPC UA nodes from ESP32 nodes"""
        print("Fetching nodes from ESP32...")
        response = self.esp_request("browse")
        
        if not response or response.get("status") != "success":
            print("Failed to get nodes from ESP32")
            return False
        
        nodes = response.get("nodes", [])
        print(f"Found {len(nodes)} nodes")
        
        for node in nodes:
            nodeId = node.get("nodeId")
            name = node.get("name", nodeId)
            node_type = node.get("type")
            writable = node.get("writable", False)
            
            try:
                # Create folder structure based on node ID
                parts = nodeId.split(".")
                parent = self.objects
                
                if len(parts) > 1:
                    folder_name = parts[0]
                    # Try to get or create folder
                    try:
                        parent = self.objects.get_child([f"{self.idx}:{folder_name}"])
                    except:
                        parent = self.objects.add_folder(self.idx, folder_name)
                
                # Determine OPC UA data type
                if node_type == "bool":
                    var = parent.add_variable(self.idx, nodeId, False)
                elif node_type == "int32":
                    var = parent.add_variable(self.idx, nodeId, 0)
                elif node_type == "float":
                    var = parent.add_variable(self.idx, nodeId, 0.0)
                elif node_type == "double":
                    var = parent.add_variable(self.idx, nodeId, 0.0)
                elif node_type == "string":
                    var = parent.add_variable(self.idx, nodeId, "")
                else:
                    var = parent.add_variable(self.idx, nodeId, "")
                
                if writable:
                    var.set_writable()
                
                self.node_map[nodeId] = {
                    'variable': var,
                    'type': node_type,
                    'writable': writable
                }
                
                print(f"  ✓ Created: {nodeId} ({node_type})")
                
            except Exception as e:
                print(f"  ✗ Failed to create {nodeId}: {e}")
        
        return True
    
    def update_node_values(self):
        """Read values from ESP32 and update OPC UA nodes"""
        for nodeId, node_info in self.node_map.items():
            try:
                response = self.esp_request("read", nodeId=nodeId)
                if response and response.get("status") != "error":
                    value = response.get("value")
                    
                    # Convert value based on type
                    node_type = node_info['type']
                    if node_type == "bool":
                        value = value in [True, "true", "1", 1]
                    elif node_type == "int32":
                        value = int(value)
                    elif node_type in ["float", "double"]:
                        value = float(value)
                    
                    node_info['variable'].set_value(value)
            except Exception as e:
                pass  # Silently ignore read errors
    
    def handle_writes(self):
        """Check for writes from OPC UA clients and forward to ESP32"""
        # Note: In a full implementation, you'd need to set up write callbacks
        # This is a simplified version that polls for changes
        pass
    
    def start(self):
        """Start the bridge server"""
        print("=" * 60)
        print("ESP32 OPC UA Bridge")
        print("=" * 60)
        print(f"ESP32 JSON Server: {self.esp_ip}:{self.esp_port}")
        print(f"OPC UA Server: opc.tcp://0.0.0.0:{self.opcua_port}")
        print("=" * 60)
        
        # Test connection to ESP32
        print("\nTesting connection to ESP32...")
        response = self.esp_request("info")
        if response:
            print(f"✓ Connected to: {response.get('server', 'Unknown')}")
            print(f"  Version: {response.get('version', 'Unknown')}")
            print(f"  Nodes: {response.get('nodes', 0)}")
        else:
            print("✗ Failed to connect to ESP32!")
            print("  Please check IP address and ensure ESP32 is running")
            return False
        
        # Create nodes
        if not self.create_nodes():
            return False
        
        # Start OPC UA server
        self.server.start()
        print(f"\n✓ OPC UA Bridge started!")
        print(f"\nConnect your OPC UA client to:")
        print(f"  opc.tcp://localhost:{self.opcua_port}")
        print(f"  or")
        print(f"  opc.tcp://<this-pc-ip>:{self.opcua_port}")
        print("\nPress Ctrl+C to stop\n")
        
        try:
            while True:
                self.update_node_values()
                time.sleep(1)  # Update every second
        except KeyboardInterrupt:
            print("\nStopping bridge...")
        finally:
            self.server.stop()
            print("Bridge stopped")
        
        return True

def main():
    parser = argparse.ArgumentParser(description='OPC UA Bridge for ESP32 JSON Server')
    parser.add_argument('--esp-ip', default='192.168.1.4', 
                        help='ESP32 IP address (default: 192.168.1.4)')
    parser.add_argument('--esp-port', type=int, default=4840,
                        help='ESP32 JSON server port (default: 4840)')
    parser.add_argument('--opcua-port', type=int, default=4841,
                        help='OPC UA bridge port (default: 4841)')
    
    args = parser.parse_args()
    
    bridge = ESP32Bridge(args.esp_ip, args.esp_port, args.opcua_port)
    bridge.start()

if __name__ == "__main__":
    main()
