#include <ArduinoJson.h>
#include "config.h"

void updateTriacPowerAnimated(int8_t index);

// Configuration
const int zeroCrossingPin = 2;                                   // Zero-crossing input pin
const int triacPin[3] = {triac_pin_1, triac_pin_2, triac_pin_3}; // Triac control pins
volatile bool zeroCrossingDetected = false;
volatile unsigned long lastZeroCrossingTime = 0;
volatile unsigned long zeroCrossingInterval = 0;
volatile unsigned long zeroCrossingCount = 0;
volatile unsigned int timerCounter = 0; // Timer ticks since last zero-crossing

const unsigned long debounceDelay = 2000; // Debounce time for zero-crossing detection (in microseconds)

// Phase control
volatile unsigned long goalTicks[3] = {0, 0, 0};     // Phase delay targets (in timer ticks)
volatile bool triacFired[3] = {false, false, false}; // Triac firing states
volatile int powerLevel[3] = {0, 0, 0};              // Power level for each triac (0-100)

// Timer configuration
const unsigned long timerIntervalMicroseconds = 50; // Timer interval (50 µs for fine-grain control)
const float acFrequency = 50.0;                     // Expected AC frequency (adjust if needed)
unsigned long halfCycleTicks;                       // = (1e6 / acFrequency) / 2 / timerIntervalMicroseconds;

volatile float measuredFrequency = 0.0;

// Function prototypes
void setTriacPower(int triacIndex, int power);

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

        // Update half-cycle ticks with measured frequency
        halfCycleTicks = (1e6 / measuredFrequency) / 2 / timerIntervalMicroseconds;

        // Update goal ticks for phase control
        for (int i = 0; i < 3; i++)
        {
            setTriacPower(i, powerLevel[i]);
        }

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
            delayMicroseconds(50);           // Short pulse to latch triac
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
    // Fix positive overflow
    else if (power >= 99)
        power = 100;

    powerLevel[triacIndex] = power;
    if (power == 0)
    {
        goalTicks[triacIndex] = 0; // Turn off the triac
    }
    else if (power == 100)
    {
        digitalWrite(triacPin[0], HIGH); // Fire the triac immediately
        goalTicks[triacIndex] = 1;       // Fire at every zero-crossing
    }
    else
    {
        goalTicks[triacIndex] = (100 - power) * halfCycleTicks / 100;
    }
}

// Arduino Setup
void power_control_setup()
{
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
    setTriacPower(0, 0); // Example: 0% power on TRIAC 0
    setTriacPower(1, 0); // Example: 0% power on TRIAC 1
    setTriacPower(2, 0); // Example: 0% power on TRIAC 2
}

void power_control_loop()
{
    /* Animated power update PURPOSE */
    updateTriacPowerAnimated(0);
    updateTriacPowerAnimated(1);
    updateTriacPowerAnimated(2);
    delay(1000);
}

// Animated power update PURPOSE
uint8_t power[3] = {0, 33, 66};
bool flow[3] = {true, false, true};
void updateTriacPowerAnimated(int8_t index)
{
    if (power[index] >= 100)
    {
        flow[index] = false;
    }
    else if (power[index] <= 0)
    {
        flow[index] = true;
    }

    power[index] = power[index] + (2 * (flow[index] ? 1 : -1));

    // Fix negative overflow
    if (power[index] == 255)
    {
        power[index] = 0;
    }

    Serial.print("Setting TRIAC ");
    Serial.print(index);
    Serial.print(" power to ");
    Serial.println(power[index]);

    setTriacPower(index, power[index]);
}
