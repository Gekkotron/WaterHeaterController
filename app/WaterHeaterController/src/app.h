// mosquitto -c /etc/mosquitto/mosquitto.conf

#include <ArduinoJson.h>
#include "config.h"
#include "temp.h"

// Begin Triacs
const int triacPin[3] = {9, 12, 13};
bool triacState[3] = {false, false, false};
unsigned long triacPower[3] = {0, 0, 0};
// End Triacs

// half-cycle is 10ms
float coeff = 2000 / 100;

// Begin Zero-crossing
volatile unsigned long lastTime = 0; // Last time a zero-crossing was detected
volatile unsigned long count = 0;    // Time between two zero-crossings (half cycle)

unsigned long lastZeroCrossingTime = 0;
const unsigned long debounceDelay = 1000; // 1ms debounce time
const int zeroCrossingPin = 2;            // Pin connected to MOC3041 output (INT0 on Arduino Uno)
bool zeroCrossingDetected = false;
float frequency = 0;

unsigned long goal = 0;
unsigned long timerCounter = 0;

// Interrupt Service Routine (ISR) for zero-crossing detection
void zeroCrossingISR()
{
    unsigned long currentTime = micros();

    // Debounce the zero-crossing signal
    if (currentTime - lastZeroCrossingTime >= debounceDelay)
    {
        zeroCrossingDetected = true;
        timerCounter = 0; // Reset the counter on zero-crossing
        count++;          // Increment the zero-crossing counter to calculate the frequency
        digitalWrite(triacPin[0], LOW);
        lastZeroCrossingTime = currentTime;
    }
}
// End zero-crossing

ISR(TIMER2_COMPA_vect)
{
    if (zeroCrossingDetected)
    {
        timerCounter++;
        if (timerCounter >= goal)
        {
            if (triacPower[0] > 0)
                digitalWrite(triacPin[0], HIGH); // Fire the TRIAC

            zeroCrossingDetected = false; // Reset the zero-crossing flag
        }
    }
}

void setup_timer1()
{
    // Disable interrupts during timer configuration
    cli();

    TCCR2A = 0; // Normal operation (no PWM)
    TCCR2B = 0;
    TCCR2A |= (1 << WGM21);  // CTC mode (Clear Timer on Compare Match)
    TCCR2B |= (1 << CS21);   // Prescaler = 8 (16MHz / 8 = 2MHz timer frequency)
    OCR2A = 10;              // Set compare match value for 5µs: 2,000,000 ticks/second * 0.00001 = 20 ticks
    TIMSK2 |= (1 << OCIE2A); // Enable Timer2 Compare A Match interrupt

    // Re-enable interrupts
    sei();
}

void app_setup()
{
    // Zero crossing
    pinMode(zeroCrossingPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(zeroCrossingPin), zeroCrossingISR, RISING); // Trigger on falling edge

    // Triacs setup

    for (int i = 0; i < 3; i++)
    {
        pinMode(triacPin[i], OUTPUT);
        digitalWrite(triacPin[i], LOW);
    }

    functionPointer = [](int value)
    {
        triacPower[0] = value;
        goal = (100 - triacPower[0]) * coeff;
    };

    setup_timer1();
}

// Data
JsonDocument data()
{
    JsonDocument doc;

    float *temp = DS18B20_getData();
    for (uint8_t i = 0; i < deviceCount; i++)
    {
        String idx = String(i);
        doc["temp_" + idx] = temp[i];
    }

    // Send frequency if it is not zero
    if (count > 0)
    {
        // Frequency = 1 / (full cycle time in seconds)
        long diff = micros() - lastTime;
        frequency = 1000000.0 / (diff * 2 / count);
        doc["frequency"] = frequency;
        doc["count"] = count;
        count = 0;
        lastTime = micros();
    }

    // Send triac state
    for (int i = 0; i < 3; i++)
    {
        String index = String(i);
        doc["tria_state_" + index] = triacState[i] ? 1 : 0;
        doc["tria_power_" + index] = triacPower[i];
    }

    doc["goal"] = goal;

    return doc;
}

void app_loop()
{
    /*
    THROTTLE(500);
    triacPower[0]++;
    if (triacPower[0] > 100)
    {
        triacPower[0] = 0;
    }
    */
}