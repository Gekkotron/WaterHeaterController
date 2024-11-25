#include <Arduino.h>
#include "config.h"
#include "ethernet_manager.h"
#include "temp.h"
#include "water_sensor.h"
#include "power_control.h"
#include "app.h"
#include "mqtt.h"

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Water Heater Controller");

  ethernet_setup();
  mqtt_setup();
  power_control_setup();
  water_sensor_setup();
}

void updatePower(uint8_t channel, uint8_t power)
{
  sprintf(buffer, "Channel %d Power %d", channel, power);
  Serial.println(buffer);
  setTriacPower(channel, power);
}

void loop()
{
  ethernet_loop();
  mqtt_loop();
  DS18B20_loop();
  publishData();
  power_control_loop();
}
