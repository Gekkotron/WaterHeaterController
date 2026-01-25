#pragma once

#include <Arduino.h>

#ifdef __STM32__
   #include <RTTStream.h>
   RTTStream rtt;
#endif

void logger_begin() {
   #ifdef __STM32__
   #else
      Serial.begin(115200);
      Serial.println("Starting Water Heater Controller");
   #endif
}

void logger_print(const char* message) {
   #ifdef __STM32__
      rtt.print(message);
   #else
      Serial.print(message);
   #endif
}

void logger_println(const char* message) {
   #ifdef __STM32__
      rtt.println(message);
   #else
      Serial.println(message);
   #endif
}