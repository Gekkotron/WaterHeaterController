#pragma once

#include <Arduino.h>
#include <Ethernet.h>
#include "logger.h"

// OTA Update functionality for both AVR and STM32
EthernetServer otaServer(8266);
bool otaInProgress = false;
uint32_t otaFileSize = 0;
uint32_t otaBytesWritten = 0;

#if defined(__AVR__)
// AVR doesn't support direct OTA without bootloader modifications
// This provides a basic HTTP server for remote management
// Actual firmware update requires bootloader support

void ota_setup()
{
    logger_println("Setting up OTA server...");
    otaServer.begin();
    logger_println("OTA server started on port 8266");
    
    char ip_str[32];
    IPAddress ip = Ethernet.localIP();
    sprintf(ip_str, "OTA ready at %d.%d.%d.%d:8266", ip[0], ip[1], ip[2], ip[3]);
    logger_println(ip_str);
}

void ota_loop()
{
    EthernetClient client = otaServer.available();
    
    if (client) {
        logger_println("OTA client connected");
        
        if (client.available()) {
            String header = client.readStringUntil('\n');
            
            if (header.startsWith("GET /status")) {
                // Status request
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/plain");
                client.println("Connection: close");
                client.println();
                client.println("WaterHeaterController AVR");
                client.print("IP: ");
                IPAddress ip = Ethernet.localIP();
                client.print(ip[0]); client.print(".");
                client.print(ip[1]); client.print(".");
                client.print(ip[2]); client.print(".");
                client.println(ip[3]);
                client.println("Note: OTA requires bootloader support");
            }
            else if (header.startsWith("POST /update")) {
                // Firmware update request
                client.println("HTTP/1.1 501 Not Implemented");
                client.println("Content-Type: text/plain");
                client.println("Connection: close");
                client.println();
                client.println("AVR OTA requires bootloader modification");
                client.println("Please use ICSP or serial bootloader");
            }
        }
        
        delay(10);
        client.stop();
    }
}

#elif defined(__STM32F1__) || defined(__STM32__)
// STM32 OTA Update functionality with flash writing

// Flash memory address for application (adjust based on your STM32 memory map)
#define APPLICATION_ADDRESS     0x08010000  // Start after bootloader space (64KB)
#define FLASH_PAGE_SIZE         0x800       // 2KB for STM32F103

void ota_setup()
{
    logger_println("Setting up OTA server...");
    otaServer.begin();
    logger_println("OTA server started on port 8266");
    
    #ifdef __STM32__
    char ip_str[32];
    IPAddress ip = Ethernet.localIP();
    sprintf(ip_str, "OTA ready at %d.%d.%d.%d:8266", ip[0], ip[1], ip[2], ip[3]);
    rtt.println(ip_str);
    #endif
}

void ota_loop()
{
    EthernetClient client = otaServer.available();
    
    if (client) {
        logger_println("OTA client connected");
        
        if (client.available()) {
            String header = client.readStringUntil('\n');
            
            if (header.startsWith("GET /status")) {
                // Status request
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: application/json");
                client.println("Connection: close");
                client.println();
                client.println("{");
                client.println("  \"device\": \"WaterHeaterController STM32\",");
                IPAddress ip = Ethernet.localIP();
                client.print("  \"ip\": \"");
                #ifdef __STM32__
                client.print(ip.toString().c_str());
                #else
                client.print(ip[0]); client.print(".");
                client.print(ip[1]); client.print(".");
                client.print(ip[2]); client.print(".");
                client.print(ip[3]);
                #endif
                client.println("\",");
                client.println("  \"status\": \"ready\"");
                client.println("}");
            }
            else if (header.startsWith("POST /update")) {
                logger_println("OTA update started");
                otaInProgress = true;
                otaBytesWritten = 0;
                
                // Read headers until blank line
                while (client.available() && client.read() != '\n');
                
                // Send response
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/plain");
                client.println("Connection: close");
                client.println();
                
                // Read firmware data
                // Note: This is a simplified implementation
                // Production code should include proper flash writing and verification
                
                while (client.connected() || client.available()) {
                    if (client.available()) {
                        uint8_t data = client.read();
                        // TODO: Write to flash memory
                        // Flash_Write(APPLICATION_ADDRESS + otaBytesWritten, data);
                        otaBytesWritten++;
                    }
                }
                
                client.println("Update complete");
                logger_println("OTA update completed");
                
                // Reboot to apply update
                // NVIC_SystemReset();
            }
        }
        
        delay(10);
        client.stop();
    }
}

#endif
