#pragma once

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xAA};
EthernetClient ethClient;

void ethernet_setup()
{
    Ethernet.init(10);
    Ethernet.begin(mac, 2000);

    delay(1500);

    Serial.println("Setup completed");
    Serial.println(Ethernet.localIP());
}
void ethernet_loop()
{
    if (Ethernet.localIP() == INADDR_NONE || Ethernet.linkStatus() == Unknown)
    {
        Serial.println("No IP");
        ethernet_setup();
    }
}