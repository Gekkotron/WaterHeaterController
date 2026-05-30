#pragma once

#include "ethernet_manager.h"
#include <PubSubClient.h>

#include <ArduinoJson.h>
#include "config.h"

#include "logger.h"

#if defined(__AVR__)
#include <avr/wdt.h>
#endif

// Power callback
extern void updatePower(uint8_t channel, uint8_t power);

bool reconnect();

// Comms watchdog: if no MQTT message is published successfully within this
// window, the network stack (W5100) is assumed dead and we force a reset to
// re-initialise it. Guards against W5100 lockups that the hardware watchdog
// can't catch (loop() keeps running, so the WDT is fed forever).
#define NETWORK_TIMEOUT 300000UL // 5 minutes
unsigned long lastSuccessfulComm = 0;

// Mqtt topic
const char *mqttTopic = "water-heater";
IPAddress server(192, 168, 1, 91);

// Uptime
unsigned long startTime = millis();

char buffer[110];

/***************************************************************/
/*                                                             */
/*                      MQTT - TOPIC                           */
/*                                                             */
/***************************************************************/
char *getDataTopic()
{
    sprintf(buffer, "%s/data", mqttTopic);
    return buffer;
}

char *getStatusTopic()
{
    sprintf(buffer, "%s/status", mqttTopic);
    return buffer;
}

char *getControlTopic()
{
    sprintf(buffer, "%s/control", mqttTopic);
    return buffer;
}

char *getHeartbeatTopic()
{
    sprintf(buffer, "%s/heartbeat", mqttTopic);
    return buffer;
}

/***************************************************************/
/*                                                             */
/*                    MQTT - Callback                          */
/*                                                             */
/***************************************************************/

void callback(char *topic, byte *payload, unsigned int length)
{
    logger_println(String("Message arrived [" + String(topic) + "]").c_str());

    // Json
    JsonDocument doc;
    deserializeJson(doc, payload, length);

    if(doc.containsKey("power0"))
        updatePower(0, doc["power0"]);

    if(doc.containsKey("power1"))
        updatePower(1, doc["power1"]);

    if(doc.containsKey("power2"))
        updatePower(2, doc["power2"]);
}

PubSubClient client;

/***************************************************************/
/*                                                             */
/*                        MQTT - Send                          */
/*                                                             */
/***************************************************************/

void mqtt_send(String topic, JsonDocument doc)
{
    char buffer[1024];
    size_t n = serializeJson(doc, buffer);
    if (!client.publish(topic.c_str(), buffer, n))
    {
        logger_println("Failed to send message");
    }
    else
    {
        logger_println(("Data published - " + topic).c_str());
        lastSuccessfulComm = millis(); // feed the comms watchdog
    }
}

void publishData()
{
    THROTTLE(4000);
    mqtt_send(getDataTopic(), createJsonData());
}

void publishHeartbeat()
{
    THROTTLE(1000); // Publish every 1 second
    
    JsonDocument doc;
    
    #if defined(__STM32F1__) || defined(__STM32__)
    doc["millis"] = HAL_GetTick();
    #elif defined(__AVR__)
    doc["millis"] = millis();
    #endif
    
    doc["uptime_minutes"] = (millis() - startTime) / 60000;
    
    mqtt_send(getHeartbeatTopic(), doc);
}

/***************************************************************/
/*                                                             */
/*                     MQTT - Reconnect                        */
/*                                                             */
/***************************************************************/

long lastReconnectAttempt = 0;

bool reconnect()
{
    logger_println("Attempting MQTT connection...");
    if (client.connect("WaterHeaterController"))
    {
        JsonDocument doc = getResetCause();
        mqtt_send(getStatusTopic(), doc);
        client.subscribe(getControlTopic(), 0);
    }
    return client.connected();
}

/***************************************************************/
/*                                                             */
/*                         MQTT - Setup                        */
/*                                                             */
/***************************************************************/

/***************************************************************/
/*                                                             */
/*                   MQTT - Comms watchdog                     */
/*                                                             */
/***************************************************************/

void forceReset()
{
#if defined(__AVR__)
    wdt_enable(WDTO_15MS);
    while (1) {} // let the hardware watchdog reset the MCU
#elif defined(__STM32F1__) || defined(__STM32__)
    NVIC_SystemReset();
#endif
}

void comms_watchdog_check()
{
    if (millis() - lastSuccessfulComm > NETWORK_TIMEOUT)
    {
        logger_println("Comms watchdog: no successful publish, resetting");
        forceReset();
    }
}

void mqtt_setup()
{
    lastReconnectAttempt = 0;
    lastSuccessfulComm = millis(); // start the comms watchdog window

    delay(1500);

    client.setClient(ethClient);
    client.setServer(server, 1883);
    client.setCallback(callback);
    client.setBufferSize(1024);
}

void mqtt_loop()
{
    if (!client.connected())
    {
        long now = millis();
        if (now - lastReconnectAttempt > 20000)
        {
            logger_println("Client disconnected");
            lastReconnectAttempt = now;
            // Attempt to reconnect
            if (reconnect())
            {
                lastReconnectAttempt = 0;
            }
        }
    }
    else
    {
        // Client connected
        client.loop();
    }
}