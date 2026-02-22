#pragma once

#include <ArduinoJson.h>

#define THROTTLE(delay)                   \
    static unsigned long lastPublish = 0; \
    if (millis() - lastPublish < delay)   \
    {                                     \
        return;                           \
    }                                     \
    lastPublish = millis();


// Leds
#if defined(__STM32F1__) || defined(__STM32__)
#define LedPin PC5
#define LedPin2 PC10
#elif defined(__AVR__)
#define LedPin 13
#define LedPin2 12
#endif

// Ethernet
#if defined(__STM32F1__) || defined(__STM32__)
#define ethernet_SPI_CS PD2
#define ethernet_SPI_MOSI PB5
#define ethernet_SPI_MISO PB4
#define ethernet_SPI_SCK PB3
#elif defined(__AVR__)
#define ethernet_SPI_CS 53
#define ethernet_SPI_MOSI 51
#define ethernet_SPI_MISO 50
#define ethernet_SPI_SCK 52
#endif

// Sensors
#if defined(__STM32F1__) || defined(__STM32__)
#define water_sensor_pin PB2
#define dsb_sensor_pin PB1
#define temp_sensor_adc_pin PB0
#elif defined(__AVR__)
// #define zeroCrossingPin 2
#define water_sensor_pin 3
#define dsb_sensor_pin 4
#define temp_sensor_adc_pin A0
#endif

// Triac pins
#if defined(__STM32F1__) || defined(__STM32__)
#define triac_pin_1 PC8
#define triac_pin_2 PC7
#define triac_pin_3 PC6
#elif defined(__AVR__)
#define triac_pin_1 5
#define triac_pin_2 6
#define triac_pin_3 7
#endif

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
#elif defined(__STM32F1__) || defined(__STM32__)
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

    const char* reset_cause_str = "unknown";
    switch (reset_cause) {
        case RESET_UNKNOWN:      reset_cause_str = "unknown"; break;
        case RESET_SYSTEM:       reset_cause_str = "system"; break;
        case RESET_WATCHDOG:     reset_cause_str = "watchdog"; break;
        case RESET_EXTERNAL:     reset_cause_str = "external"; break;
        case RESET_BROWNOUT_33:  reset_cause_str = "brownout_33"; break;
        case RESET_BROWNOUT_12:  reset_cause_str = "brownout_12"; break;
        case RESET_POWERON:      reset_cause_str = "poweron"; break;
    }
    doc["reset_cause"] = reset_cause_str;
    doc["reset_cause_code"] = reset_cause_code;

    return doc;
}