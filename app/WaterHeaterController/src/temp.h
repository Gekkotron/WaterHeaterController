#pragma once

#include <DS18B20.h>

// Sensors
DS18B20 ds(temp_sensor_pin);
uint8_t deviceCount = 0;
#define ExceptedNbSensors 3
float *temperatures = new float[ExceptedNbSensors];

void DS18B20_setup()
{
    Serial.print("Setup DS18B20 - Devices: ");
    delay(1000);
    deviceCount = ds.getNumberOfDevices();
    Serial.println(deviceCount);
}

void DS18B20_loop()
{
    if (deviceCount < ExceptedNbSensors)
    {
        THROTTLE(10000);
        DS18B20_setup();
    }
    else
    {
        THROTTLE(2000);
    }
    uint8_t index = 0;
    while (ds.selectNext() && index < deviceCount) // Ensure we stay within bounds
    {
        temperatures[index] = ds.getTempC(); // Store the temperature in the array
        index++;                             // Increment after assignment
    }
}