#pragma once

#include "ethernet_manager.h"
#include <PubSubClient.h>

#include <ArduinoJson.h>
#include "config.h"

// Power callback
extern void updatePower(uint8_t channel, uint8_t power);

bool reconnect();

// Mqtt topic
const char *mqttTopic = "water_heater";
IPAddress server(192, 168, 1, 90);

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

/***************************************************************/
/*                                                             */
/*                    MQTT - Callback                          */
/*                                                             */
/***************************************************************/

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.println("Message arrived [" + String(topic) + "]");

    // Json
    JsonDocument doc;
    deserializeJson(doc, payload, length);

    updatePower(0, doc["power0"]);
    updatePower(1, doc["power1"]);
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
        Serial.println("Failed to send message");
    }
    else
    {
        Serial.println("Data published - " + topic);
    }
}

void publishData()
{
    THROTTLE(2000);
    mqtt_send(getDataTopic(), createJsonData());
}

/***************************************************************/
/*                                                             */
/*                     MQTT - Reconnect                        */
/*                                                             */
/***************************************************************/

long lastReconnectAttempt = 0;

bool reconnect()
{
    Serial.println("Attempting MQTT connection...");
    if (client.connect("WaterHeaterController"))
    {
        JsonDocument doc = getResetCause();
        unsigned long elapsedMillis = millis() - startTime;

        // Convert milliseconds to minutes
        unsigned long elapsedMinutes = elapsedMillis / 60000;

        doc["uptime"] = elapsedMinutes;

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

void mqtt_setup()
{
    lastReconnectAttempt = 0;

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
            Serial.println("Client disconnected");
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