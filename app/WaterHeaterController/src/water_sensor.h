#include <Arduino.h>
#include <FlowSensor.h>

// Water sensor YF-B5
uint16_t type = 396;

// Flux (Q): 1-30L / min
// Fréquence (F): F = 6,6 * Q
// Pulse (P): P = 6,6 * Q
// 1L = 1000ml
// P = 6,6 * 60 = 396 pulses / min

FlowSensor Sensor(type, water_sensor_pin);

void count()
{
    Serial.println("Count");
    Sensor.count();
}

void water_sensor_setup()
{
    Serial.println("Water Sensor Setup");
    Sensor.begin(count);
}

// Pulse
unsigned long getPulse()
{
    return Sensor.getPulse();
}

// Get Values
void water_sensor_read()
{
    Sensor.read();
}

// Flow rate data
float getWaterFlowRateHeure()
{
    return Sensor.getFlowRate_h();
}

float getWaterFlowRateMinute()
{
    return Sensor.getFlowRate_m();
}

float getWaterFlowRateSecond()
{
    return Sensor.getFlowRate_s();
}

float getWaterVolume()
{
    return Sensor.getVolume();
}

// Reset
void resetPulse()
{
    Sensor.resetPulse();
}

void resetVolume()
{
    Sensor.resetVolume();
}