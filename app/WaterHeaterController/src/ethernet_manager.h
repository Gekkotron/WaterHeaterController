#pragma once

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "logger.h"

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetClient ethClient;

void ethernet_setup()
{
    // Fix proto 1
    pinMode(PB3, INPUT);
    pinMode(PB4, INPUT);
    pinMode(PB5, INPUT);

    //pinMode(ethernet_SPI_CS, OUTPUT);
    //digitalWrite(ethernet_SPI_CS, HIGH);
    
    Ethernet.init(ethernet_SPI_CS);
    if (Ethernet.begin(mac) == 0) {
        logger_println("DHCP failed");
    }

    delay(1500);

    logger_println("Setup completed");
#if defined(__STM32F1__) || defined(__STM32__)
    logger_println(Ethernet.localIP().toString().c_str());
#elif defined(__AVR__)
    char ip_str[16];
    IPAddress ip = Ethernet.localIP();
    sprintf(ip_str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    logger_println(ip_str);
#endif
}

void ethernet_loop()
{
    THROTTLE(30000);
    
    if (Ethernet.localIP() == INADDR_NONE || Ethernet.linkStatus() == Unknown)
    {
        logger_println("No IP");
        ethernet_setup();
    }
    else
    {
#if defined(__STM32F1__) || defined(__STM32__)
        logger_println(Ethernet.localIP().toString().c_str());
#elif defined(__AVR__)
        char ip_str[16];
        IPAddress ip = Ethernet.localIP();
        sprintf(ip_str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        logger_println(ip_str);
#endif
    }

    if (Ethernet.linkStatus() == LinkON)
    {
        logger_println("Link ON");
    }
    else if (Ethernet.linkStatus() == LinkOFF)
    {
        logger_println("Link OFF");
    }
    else if (Ethernet.linkStatus() == Unknown)
    {
        logger_println("Link Unknown");
    }
}