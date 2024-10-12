
#include <ArduinoJson.h>

#include <DS18B20.h>

DS18B20 ds(2);

int deviceCount = 0;

void DS18B20_setup()
{
    Serial.print("Devices: ");
    Serial.println(ds.getNumberOfDevices());
}

JsonDocument data()
{
    JsonDocument doc;

    uint8_t index = 0;
    while (ds.selectNext())
    {
        String idx = String(index++);
        float temperature = ds.getTempC();
        doc["temp_" + idx] = temperature;
    }

    return doc;
}