#include <Arduino.h>
#include "mqtt.h"
#include "app.h"
#include "config.h"
#include "ethernet_manager.h"
#include "temp.h"

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Water Heater Controller");

  ethernet_setup();
  mqtt_setup();
  app_setup();
  DS18B20_setup();
}

void publishData()
{
  THROTTLE(2000);
  mqtt_send(getDataTopic(), createJsonData());
  Serial.println("Data published");
}

void functionPointer(int power)
{
  Serial.println("Power: " + String(power));
  setTriacPower(0, power);
}

void loop()
{
  ethernet_loop();
  mqtt_loop();
  publishData();
  app_loop();
}
