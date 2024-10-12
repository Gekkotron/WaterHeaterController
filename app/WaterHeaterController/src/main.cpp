#include <Arduino.h>
#include "mqtt.h"
#include "app.h"

#define THROTTLE(delay)                 \
  static unsigned long lastPublish = 0; \
  if (millis() - lastPublish < delay)   \
  {                                     \
    return;                             \
  }                                     \
  lastPublish = millis();

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Water Heater Controller");

  mqtt_setup();
  DS18B20_setup();
}

void publishData()
{
  THROTTLE(5000);
  mqtt_send(getDataTopic().c_str(), data());
}

void loop()
{
  mqtt_loop();
  publishData();
}
