#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <PubSubClient.h>

#include <ArduinoJson.h>

// Mqtt topic
const char *mqttTopic = "water_heater";

byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xAA};
IPAddress server(192, 168, 1, 100);

String getDataTopic()
{
    return String(mqttTopic) + "/data";
}

String getStatusTopic()
{
    return String(mqttTopic) + "/status";
}

void callback(char *topic, byte *payload, unsigned int length)
{
    // handle message arrived
}

EthernetClient ethClient;
PubSubClient client;

long lastReconnectAttempt = 0;

void mqtt_send(String topic, JsonDocument doc)
{
    char buffer[1024];
    size_t n = serializeJson(doc, buffer);
    client.publish(topic.c_str(), buffer, n);
}

boolean reconnect()
{
    if (client.connect("WaterHeaterController"))
    {
        JsonDocument doc;

        mqtt_send(getStatusTopic().c_str(), doc);
        // ... and resubscribe
        // client.subscribe("inTopic");
    }
    return client.connected();
}

void mqtt_setup()
{

    Ethernet.init(10);
    Ethernet.begin(mac, 2000);
    delay(1500);
    lastReconnectAttempt = 0;

    Serial.println("Setup completed");
    Serial.println(Ethernet.localIP());

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
        if (now - lastReconnectAttempt > 5000)
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