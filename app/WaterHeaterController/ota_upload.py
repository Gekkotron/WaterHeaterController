#!/usr/bin/env python3
"""
OTA Firmware Upload Script
Usage: python3 ota_upload.py <device_ip> <firmware.hex>
"""

import sys
import os
import socket
import time

def upload_firmware_avr(ip_address, firmware_file, password="admin"):
    """
    Upload firmware to AVR device using ArduinoOTA protocol
    """
    print(f"Uploading {firmware_file} to {ip_address}...")
    
    # Use Arduino CLI or avrdude with network protocol
    # This requires the ArduinoOTA library to be configured on the device
    
    # For now, use a simple command
    cmd = f"arduino-cli upload -p network://{ip_address} -b arduino:avr:mega --input-file {firmware_file}"
    print(f"Command: {cmd}")
    print("Note: Ensure arduino-cli is installed and configured")
    
    os.system(cmd)

def upload_firmware_stm32(ip_address, firmware_file, port=8266):
    """
    Upload firmware to STM32 device using HTTP POST
    """
    print(f"Uploading {firmware_file} to {ip_address}:{port}...")
    
    try:
        # Read firmware file
        with open(firmware_file, 'rb') as f:
            firmware_data = f.read()
        
        # Create HTTP POST request
        request = f"POST /update HTTP/1.1\r\n"
        request += f"Host: {ip_address}\r\n"
        request += f"Content-Length: {len(firmware_data)}\r\n"
        request += f"Content-Type: application/octet-stream\r\n"
        request += "Connection: close\r\n"
        request += "\r\n"
        
        # Connect to device
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(30)
        sock.connect((ip_address, port))
        
        # Send header
        sock.send(request.encode())
        
        # Send firmware data in chunks
        chunk_size = 1024
        total_sent = 0
        
        for i in range(0, len(firmware_data), chunk_size):
            chunk = firmware_data[i:i+chunk_size]
            sock.send(chunk)
            total_sent += len(chunk)
            progress = (total_sent / len(firmware_data)) * 100
            print(f"Progress: {progress:.1f}% ({total_sent}/{len(firmware_data)} bytes)", end='\r')
        
        print("\nWaiting for response...")
        
        # Read response
        response = sock.recv(4096)
        print(f"Response: {response.decode('utf-8', errors='ignore')}")
        
        sock.close()
        print("Upload completed!")
        
    except Exception as e:
        print(f"Error: {e}")
        return False
    
    return True

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 ota_upload.py <device_ip> <firmware_file> [type]")
        print("  type: 'avr' or 'stm32' (default: auto-detect from file extension)")
        sys.exit(1)
    
    ip_address = sys.argv[1]
    firmware_file = sys.argv[2]
    
    if not os.path.exists(firmware_file):
        print(f"Error: Firmware file '{firmware_file}' not found")
        sys.exit(1)
    
    # Auto-detect type from file extension
    if len(sys.argv) > 3:
        device_type = sys.argv[3].lower()
    else:
        if firmware_file.endswith('.hex'):
            device_type = 'avr'
        elif firmware_file.endswith('.bin'):
            device_type = 'stm32'
        else:
            print("Warning: Could not detect device type, defaulting to STM32")
            device_type = 'stm32'
    
    print(f"Device type: {device_type}")
    
    if device_type == 'avr':
        upload_firmware_avr(ip_address, firmware_file)
    else:
        upload_firmware_stm32(ip_address, firmware_file)

if __name__ == "__main__":
    main()
