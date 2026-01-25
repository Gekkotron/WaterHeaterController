#include <Arduino.h>
#include "config.h"
#include "ethernet_manager.h"
#include "temp.h"
#include "water_sensor.h"
#include "power_control.h"
#include "app.h"
#include "mqtt.h"
#include "logger.h"
#include "analog.h"
#include "ota_update.h"


void setup()
{
  //HAL_Init();
  logger_begin();

  ethernet_setup();
  ota_setup();
  mqtt_setup();
  power_control_setup();
  //water_sensor_setup();
  adc_setup();

  pinMode(LedPin, OUTPUT); // Initialize the LED pin as an output
  digitalWrite(LedPin, HIGH);

  delay(2000);
}

void updatePower(uint8_t channel, uint8_t power)
{
  sprintf(buffer, "Channel %d Power %d", channel, power);
  logger_println(buffer);
  setTriacPower(channel, power);
}

unsigned long lastBlink = 0;
void loop()
{
  ota_loop();
  ethernet_loop();
  mqtt_loop();
  adc_loop();
  DS18B20_loop();
  publishData();
  publishHeartbeat();
  checkPowerTimeout();

  // led blinking
#if defined(__STM32F1__) || defined(__STM32__)
  if (HAL_GetTick() - lastBlink > 500) {
      lastBlink = HAL_GetTick();
#elif defined(__AVR__)
  if (millis() - lastBlink > 500) {
      lastBlink = millis();
#endif
      digitalWrite(LedPin, !digitalRead(LedPin));

      /*
      digitalWrite(triac_pin_1, HIGH);
      digitalWrite(triac_pin_2, LOW);
      digitalWrite(triac_pin_3, LOW);
      */

#if defined(__STM32F1__) || defined(__STM32__)
      JsonDocument document = createJsonData();
      rtt.print("JSON: ");
      serializeJson(document, rtt);
      rtt.println();
#endif
  }
}
