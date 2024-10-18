#include <Arduino.h>
#include "mqtt.h"
#include "app.h"
#include "config.h"

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
  Serial.println("Data published");
}

void loop()
{
  mqtt_loop();
  publishData();
  app_loop();
}
