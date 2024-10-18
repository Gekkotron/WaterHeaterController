// mosquitto -c /etc/mosquitto/mosquitto.conf

#include <ArduinoJson.h>
#include <DS18B20.h>
#include "config.h"

// Sensors
DS18B20 ds(3);
int deviceCount = 0;

// Begin Triacs
const int triacPin[3] = {9, 12, 13};
bool triacState[3] = {false, false, false};
unsigned long triacPower[3] = {0, 0, 0};
// End Triacs

// half-cycle is 10ms
float total_timer_count = 0.010 / 0.00001;
float coeff = total_timer_count / 100;

// Begin Zero-crossing
volatile unsigned long lastTime = 0; // Last time a zero-crossing was detected
volatile unsigned long count = 0;    // Time between two zero-crossings (half cycle)

const int zeroCrossingPin = 2; // Pin connected to MOC3041 output (INT0 on Arduino Uno)
bool zeroCrossingDetected = false;
float frequency = 0;

unsigned long goal = 0;
unsigned long timerCounter = 0;

// Interrupt Service Routine (ISR) for zero-crossing detection
void zeroCrossingISR()
{
    zeroCrossingDetected = true;
    timerCounter = 0; // Reset the counter on zero-crossing
    count++;          // Increment the zero-crossing counter to calculate the frequency
    digitalWrite(triacPin[0], HIGH);
}
// End zero-crossing

ISR(TIMER2_COMPA_vect)
{
    if (zeroCrossingDetected)
    {
        timerCounter++;
        if (timerCounter >= goal)
        {
            digitalWrite(triacPin[0], LOW); // Fire the TRIAC
            zeroCrossingDetected = false;   // Reset the zero-crossing flag
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
    OCR2A = 20;              // Set compare match value for 10µs: 2,000,000 ticks/second * 0.00001 = 20 ticks
    TIMSK2 |= (1 << OCIE2A); // Enable Timer2 Compare A Match interrupt

    // Re-enable interrupts
    sei();
}

void DS18B20_setup()
{
    Serial.print("Devices: ");
    delay(1000);
    Serial.println(ds.getNumberOfDevices());

    // Zero crossing
    pinMode(zeroCrossingPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(zeroCrossingPin), zeroCrossingISR, CHANGE); // Trigger on falling edge

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

    uint8_t index = 0;
    while (ds.selectNext())
    {
        String idx = String(index++);
        float temperature = ds.getTempC();
        doc["temp_" + idx] = temperature;
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
    doc["total"] = total_timer_count;

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