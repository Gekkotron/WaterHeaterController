#pragma once

#include <ArduinoJson.h>

#define THROTTLE(delay)                   \
    static unsigned long lastPublish = 0; \
    if (millis() - lastPublish < delay)   \
    {                                     \
        return;                           \
    }                                     \
    lastPublish = millis();

#define water_sensor_pin 3
#define temp_sensor_pin 4
#define triac_pin_1 5
#define triac_pin_2 6
#define triac_pin_3 7

enum ResetCause
{
    RESET_UNKNOWN,
    RESET_SYSTEM,
    RESET_WATCHDOG,
    RESET_EXTERNAL,
    RESET_BROWNOUT_33,
    RESET_BROWNOUT_12,
    RESET_POWERON
};

JsonDocument getResetCause()
{
    JsonDocument doc;

    // Find the cause of reset
    ResetCause reset_cause = RESET_UNKNOWN;
    uint8_t reset_cause_code = 0;

#if defined(__AVR__)
    if (MCUSR & (1 << WDRF))
    {
        reset_cause = RESET_WATCHDOG;
        reset_cause_code = 2;
    }
    else if (MCUSR & (1 << EXTRF))
    {
        reset_cause = RESET_EXTERNAL;
        reset_cause_code = 3;
    }
    else if (MCUSR & (1 << BORF))
    {
        reset_cause = RESET_BROWNOUT_33;
        reset_cause_code = 4;
    }
    else if (MCUSR & (1 << PORF))
    {
        reset_cause = RESET_POWERON;
        reset_cause_code = 6;
    }
#elif defined(__STM32F1__)
    if (RCC->CSR & RCC_CSR_WWDGRSTF)
    {
        reset_cause = RESET_WATCHDOG;
        reset_cause_code = 2;
    }
    else if (RCC->CSR & RCC_CSR_PORRSTF)
    {
        reset_cause = RESET_POWERON;
        reset_cause_code = 6;
    }
#endif

    doc["reset_cause"] = reset_cause;
    doc["reset_cause_code"] = reset_cause_code;

    return doc;
}