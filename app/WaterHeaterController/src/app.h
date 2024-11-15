#include <ArduinoJson.h>
#include "config.h"
#include "temp.h"

// Configuration
const int zeroCrossingPin = 2;       // Zero-crossing input pin
const int triacPin[3] = {4, 12, 13}; // Triac control pins
volatile bool zeroCrossingDetected = false;
volatile unsigned long lastZeroCrossingTime = 0;
volatile unsigned long zeroCrossingInterval = 0;
volatile unsigned long zeroCrossingCount = 0;
volatile unsigned int timerCounter = 0; // Timer ticks since last zero-crossing

float acFrequency = 50.0;                 // Expected AC frequency (adjust if needed)
const unsigned long debounceDelay = 1000; // Debounce time for zero-crossing detection (in microseconds)

// Phase control
volatile unsigned long goalTicks[3] = {0, 0, 0};     // Phase delay targets (in timer ticks)
volatile bool triacFired[3] = {false, false, false}; // Triac firing states
volatile int powerLevel[3] = {0, 0, 0};              // Power level for each triac (0-100)

// Timer configuration
const unsigned long timerIntervalMicroseconds = 50; // Timer interval (50 µs for fine-grain control)
const unsigned long halfCycleTicks = (1e6 / acFrequency) / 2 / timerIntervalMicroseconds;

volatile float measuredFrequency = 0.0;

// Zero-Crossing Interrupt
void zeroCrossingISR()
{
    unsigned long currentTime = micros();
    if (currentTime - lastZeroCrossingTime >= debounceDelay)
    {
        zeroCrossingDetected = true;
        zeroCrossingInterval = currentTime - lastZeroCrossingTime; // Measure interval
        lastZeroCrossingTime = currentTime;

        zeroCrossingCount++;

        // Update frequency calculation
        measuredFrequency = 1e6 / (zeroCrossingInterval * 2); // Double zero crossings per cycle

        // Reset timer counters and triac states
        timerCounter = 0;
        for (int i = 0; i < 3; i++)
        {
            triacFired[i] = false; // Reset firing state
        }
    }
}

// Timer Interrupt: Handles triac firing delays
ISR(TIMER1_COMPA_vect)
{
    timerCounter++;

    for (int i = 0; i < 3; i++)
    {
        // Check if it's time to fire the triac
        if (!triacFired[i] && timerCounter >= goalTicks[i] && goalTicks[i] > 0)
        {
            digitalWrite(triacPin[i], HIGH); // Fire the triac
            delayMicroseconds(100);           // Short pulse to latch triac
            digitalWrite(triacPin[i], LOW);  // Turn off the pulse
            triacFired[i] = true;            // Mark as fired
        }
    }
}

// Timer1 Setup
void setupTimer1()
{
    cli();      // Disable interrupts
    TCCR1A = 0; // Clear Timer1 control registers
    TCCR1B = 0;
    TCCR1B |= (1 << WGM12);                      // Configure Timer1 in CTC mode
    TCCR1B |= (1 << CS11);                       // Set prescaler to 8 (2 MHz timer frequency)
    OCR1A = (timerIntervalMicroseconds * 2) - 1; // Set compare match value for 50 µs interval
    TIMSK1 |= (1 << OCIE1A);                     // Enable Timer1 compare interrupt
    sei();                                       // Enable interrupts
}

// Power Control Function
void setTriacPower(int triacIndex, int power)
{
    if (power < 0)
        power = 0;
    if (power >= 99)
        power = 100;

    powerLevel[triacIndex] = power;
    if (power == 0)
    {
        goalTicks[triacIndex] = 0; // Turn off the triac
    }
    else if (power == 100)
    {
        digitalWrite(triacPin[0], HIGH); // Fire the triac immediately
        goalTicks[triacIndex] = 0;       // Fire at every zero-crossing
    }
    else
    {
        goalTicks[triacIndex] = (100 - power) * halfCycleTicks / 100;
    }
}

// JSON Reporting
JsonDocument createJsonData()
{
    JsonDocument doc;

    float *temperatures = DS18B20_getData();
    for (uint8_t i = 0; i < deviceCount; i++)
    {
        String idx = String(i);
        doc["temp_" + idx] = temperatures[i];
    }
    delete[] temperatures;

    doc["measuredFrequency"] = measuredFrequency;
    doc["zeroCrossingCount"] = zeroCrossingCount;
    doc["timerCounter"] = timerCounter;

    zeroCrossingCount = 0; // Reset zero-crossing count

    for (int i = 0; i < 3; i++)
    {
        doc["triac_" + String(i) + "_power"] = powerLevel[i];
        doc["triac_" + String(i) + "_goalTicks"] = goalTicks[i];
    }

    return doc;
}

// Arduino Setup
void app_setup()
{
    Serial.begin(115200);

    // Setup zero-crossing pin
    pinMode(zeroCrossingPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(zeroCrossingPin), zeroCrossingISR, RISING);

    // Setup triac control pins
    for (int i = 0; i < 3; i++)
    {
        pinMode(triacPin[i], OUTPUT);
        digitalWrite(triacPin[i], LOW);
    }

    // Initialize Timer1
    setupTimer1();

    // Set initial power levels
    setTriacPower(0, 0); // Example: 50% power on TRIAC 0
    // setTriacPower(1, 75); // Example: 75% power on TRIAC 1
    // setTriacPower(2, 25); // Example: 25% power on TRIAC 2
}

uint8_t power = 0;
bool flow = true;
void app_loop()
{
    /*
    if (power >= 100)
    {
        flow = false;
    }
    else if (power <= 0)
    {
        flow = true;
    }

    power = power + (5 * (flow ? 1 : -1));

    setTriacPower(0, power);
    delay(2000);
    */
}
