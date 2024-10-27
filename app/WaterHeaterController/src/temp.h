#pragma once

#include <DS18B20.h>

// Sensors
DS18B20 ds(3);
uint8_t deviceCount = 0;

void DS18B20_setup()
{
    Serial.print("Devices: ");
    delay(1000);
    deviceCount = ds.getNumberOfDevices();
    Serial.println(deviceCount);
}

float *DS18B20_getData()
{
    float *data = new float[deviceCount]; // Allocate an array of floats

    uint8_t index = 0;
    while (ds.selectNext() && index < deviceCount) // Ensure we stay within bounds
    {
        data[index] = ds.getTempC(); // Store the temperature in the array
        index++;                     // Increment after assignment
    }

    return data; // Return the pointer to the float array
}