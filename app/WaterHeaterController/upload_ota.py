"""
Custom OTA Upload Script for PlatformIO
Uploads firmware to device over network via HTTP
"""

Import("env")
import socket
import sys
import os

def upload_via_http(source, target, env):
    """
    Upload firmware file to device via HTTP POST
    """
    firmware_path = str(source[0])
    upload_port = env.get("UPLOAD_PORT", "192.168.1.92:8266")
    
    # Parse host and port
    if ':' in upload_port:
        host, port = upload_port.split(':')
        port = int(port)
    else:
        host = upload_port
        port = 8266
    
    print(f"Uploading {firmware_path} to {host}:{port}...")
    
    # Check device status first
    print("Checking device compatibility...")
    try:
        check_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        check_sock.settimeout(5)
        check_sock.connect((host, port))
        
        status_request = f"GET /status HTTP/1.1\r\nHost: {host}\r\nConnection: close\r\n\r\n"
        check_sock.send(status_request.encode())
        
        status_response = b""
        try:
            while True:
                data = check_sock.recv(1024)
                if not data:
                    break
                status_response += data
        except socket.timeout:
            pass
        
        check_sock.close()
        status_str = status_response.decode('utf-8', errors='ignore')
        
        # Check if device supports OTA
        if "AVR OTA requires bootloader" in status_str or "WaterHeaterController AVR" in status_str:
            print("\n✗ ERROR: This device is AVR and does not support OTA")
            print("AVR devices require bootloader modifications for OTA")
            print("Please use serial upload instead:")
            print("  pio run -t upload -e atmega2560")
            return 1
            
        print("Device supports OTA - proceeding...")
        
    except Exception as e:
        print(f"Warning: Could not check device status: {e}")
        print("Continuing anyway...")
    
    # Read firmware file
    try:
        with open(firmware_path, 'rb') as f:
            firmware_data = f.read()
    except Exception as e:
        print(f"Error reading firmware file: {e}")
        return 1
    
    print(f"Firmware size: {len(firmware_data)} bytes")
    
    # Create HTTP POST request
    request = f"POST /update HTTP/1.1\r\n"
    request += f"Host: {host}\r\n"
    request += f"Content-Length: {len(firmware_data)}\r\n"
    request += "Content-Type: application/octet-stream\r\n"
    request += "Connection: close\r\n"
    request += "\r\n"
    
    try:
        # Connect to device
        print(f"Connecting to {host}:{port}...")
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(30)
        sock.connect((host, port))
        print("Connected!")
        
        # Send HTTP header
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
        response = b""
        sock.settimeout(5)
        try:
            while True:
                data = sock.recv(1024)
                if not data:
                    break
                response += data
        except socket.timeout:
            pass
        
        response_str = response.decode('utf-8', errors='ignore')
        print(f"\nServer response:")
        print(response_str[:500])  # Print first 500 chars
        
        sock.close()
        
        if "200 OK" in response_str or "Update complete" in response_str:
            print("\n✓ Upload successful!")
            print("Device will reboot automatically...")
            return 0
        else:
            print("\n✗ Upload may have failed - check device")
            return 1
        
    except socket.timeout:
        print("\n✗ Connection timeout - check device IP and network")
        return 1
    except ConnectionRefusedError:
        print(f"\n✗ Connection refused - is OTA server running on {host}:{port}?")
        print("Make sure the device is powered on and connected to network")
        return 1
    except Exception as e:
        print(f"\n✗ Upload error: {e}")
        return 1

# Override the upload command
env.Replace(UPLOADCMD=upload_via_http)
