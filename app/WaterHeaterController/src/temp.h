#pragma once

#include <DS18B20.h>

// Sensors
DS18B20 ds(dsb_sensor_pin);
uint8_t deviceCount = 0;
#define ExceptedNbSensors 4  // Increase to support up to 4 sensors
float *temperatures = new float[ExceptedNbSensors];

void DS18B20_setup()
{
    delay(1000);
    
    // Initialize all temperature values to NaN or 0
    for (uint8_t i = 0; i < ExceptedNbSensors; i++) {
        temperatures[i] = -127.0;  // DS18B20 error value
    }
    
    deviceCount = ds.getNumberOfDevices();
    logger_println(("Setup DS18B20 - Devices: " + String(deviceCount)).c_str());

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

    if(deviceCount == 0)
        return;

    logger_println("Temperatures: ");
    while (ds.selectNext() && index < deviceCount) // Ensure we stay within bounds
    {
        temperatures[index] = ds.getTempC(); // Store the temperature in the array
        logger_print(String(temperatures[index]).c_str());
        logger_print(" C ");

        index++;                             // Increment after assignment
    }
    logger_println("");
}