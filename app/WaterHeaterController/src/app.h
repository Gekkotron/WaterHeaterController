#include <ArduinoJson.h>
#include "config.h"
#include "temp.h"

// JSON Reporting
JsonDocument createJsonData()
{
    JsonDocument doc;

    for (uint8_t i = 0; i < deviceCount; i++)
    {
        String idx = String(i);
        doc["temp_" + idx] = temperatures[i];
    }

    doc["measuredFrequency"] = measuredFrequency;
    doc["zeroCrossingCount"] = zeroCrossingCount;
    doc["frequencyError"] = abs(measuredFrequency - acFrequency) > 2 ? 1 : 0;

    water_sensor_read();
    doc["flow_rate_second"] = getWaterFlowRateSecond();
    doc["flow_rate_minute"] = getWaterFlowRateMinute();
    doc["flow_rate_heure"] = getWaterFlowRateHeure();
    doc["volume"] = getWaterVolume();
    doc["pulse"] = getPulse();

    zeroCrossingCount = 0; // Reset zero-crossing count

    for (int i = 0; i < 3; i++)
    {
        doc["triac_" + String(i) + "_power"] = powerLevel[i];
        doc["triac_" + String(i) + "_goalTicks"] = goalTicks[i];
    }

    return doc;
}