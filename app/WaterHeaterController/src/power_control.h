#include <ArduinoJson.h>
#include "config.h"

void updateTriacPowerAnimated(int8_t index);

// Configuration
#if defined(__STM32F1__) || defined(__STM32__)
const int zeroCrossingPin = PA15;                                // Zero-crossing input pin
#elif defined(__AVR__)
const int zeroCrossingPin = 2;                                   // Zero-crossing input pin (use INT0 or INT1)
#endif
const int triacPin[3] = {triac_pin_1, triac_pin_2, triac_pin_3}; // Triac control pins

// Fast port manipulation macros for critical timing sections
#if defined(__AVR__)
// ATmega2560: Pin 5=PE3, Pin 6=PH3, Pin 7=PH4
#define TRIAC_0_HIGH() PORTE |= (1 << 3)
#define TRIAC_0_LOW()  PORTE &= ~(1 << 3)
#define TRIAC_1_HIGH() PORTH |= (1 << 3)
#define TRIAC_1_LOW()  PORTH &= ~(1 << 3)
#define TRIAC_2_HIGH() PORTH |= (1 << 4)
#define TRIAC_2_LOW()  PORTH &= ~(1 << 4)
#elif defined(__STM32F1__) || defined(__STM32__)
// STM32: PC8, PC7, PC6
#define TRIAC_0_HIGH() GPIOC->BSRR = (1 << 8)
#define TRIAC_0_LOW()  GPIOC->BSRR = (1 << (8 + 16))
#define TRIAC_1_HIGH() GPIOC->BSRR = (1 << 7)
#define TRIAC_1_LOW()  GPIOC->BSRR = (1 << (7 + 16))
#define TRIAC_2_HIGH() GPIOC->BSRR = (1 << 6)
#define TRIAC_2_LOW()  GPIOC->BSRR = (1 << (6 + 16))
#endif
volatile bool zeroCrossingDetected = false;
volatile unsigned long lastZeroCrossingTime = 0;
volatile unsigned long zeroCrossingInterval = 0;
volatile unsigned long zeroCrossingCount = 0;
volatile unsigned int timerCounter = 0; // Timer ticks since last zero-crossing

// Power timeout configuration
const unsigned long powerTimeoutMs = 15UL * 60UL * 1000UL; // 15 minutes in milliseconds
unsigned long lastPowerCommandTime = 0; // Last time a power command was received

const unsigned long debounceDelay = 2000; // Debounce time for zero-crossing detection (in microseconds)

// Phase control
volatile unsigned long goalTicks[3] = {0, 0, 0};     // Phase delay targets (in timer ticks)
volatile bool triacFired[3] = {false, false, false}; // Triac firing states
volatile bool triacPinState[3] = {false, false, false}; // Track pin HIGH/LOW state
volatile int powerLevel[3] = {0, 0, 0};              // Power level for each triac (0-100)

// Timer configuration
const unsigned long timerIntervalMicroseconds = 50; // Timer interval (50 µs for fine-grain control)
const float acFrequency = 50.0;                     // Expected AC frequency (adjust if needed)
unsigned long halfCycleTicks = 200;                 // = (1e6 / 50Hz) / 2 / 50µs = 200 ticks (updated by ISR)

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

        // Reset timer counters and triac states FIRST
        timerCounter = 0;
        
        // Triac 0
        triacFired[0] = false;
        if (powerLevel[0] < 100 && triacPinState[0])
        {
            TRIAC_0_LOW();
            triacPinState[0] = false;
        }
        
        // Triac 1
        triacFired[1] = false;
        if (powerLevel[1] < 100 && triacPinState[1])
        {
            TRIAC_1_LOW();
            triacPinState[1] = false;
        }
        
        // Triac 2
        triacFired[2] = false;
        if (powerLevel[2] < 100 && triacPinState[2])
        {
            TRIAC_2_LOW();
            triacPinState[2] = false;
        }

        // Update half-cycle ticks with measured frequency
        // DISABLED: frequency recalculation causing glitches
        // unsigned long newHalfCycleTicks = (1e6 / measuredFrequency) / 2 / timerIntervalMicroseconds;
        //
        // // Only recalculate goalTicks if frequency changed significantly (use larger threshold for stability)
        // if (abs((long)(newHalfCycleTicks - halfCycleTicks)) > 3)
        // {
        //     halfCycleTicks = newHalfCycleTicks;
        //     for (int i = 0; i < 3; i++)
        //     {
        //         // Recalculate goalTicks based on stored powerLevel
        //         if (powerLevel[i] > 0 && powerLevel[i] < 100)
        //         {
        //             goalTicks[i] = (100 - powerLevel[i]) * halfCycleTicks / 100;
        //         }
        //     }
        // }
    }
}

void timer1Interrupt()
{
    // Read counter once to avoid race condition with zero-crossing ISR
    unsigned int currentCounter = ++timerCounter;

    // Triac 0
    if (powerLevel[0] != 100)
    {
        if (!triacFired[0] && currentCounter >= goalTicks[0] && goalTicks[0] > 0)
        {
            TRIAC_0_HIGH();
            triacPinState[0] = true;
            triacFired[0] = true;
        }
        else if (triacFired[0] && triacPinState[0])
        {
            TRIAC_0_LOW();
            triacPinState[0] = false;
        }
    }

    // Triac 1
    if (powerLevel[1] != 100)
    {
        if (!triacFired[1] && currentCounter >= goalTicks[1] && goalTicks[1] > 0)
        {
            TRIAC_1_HIGH();
            triacPinState[1] = true;
            triacFired[1] = true;
        }
        else if (triacFired[1] && triacPinState[1])
        {
            TRIAC_1_LOW();
            triacPinState[1] = false;
        }
    }

    // Triac 2
    if (powerLevel[2] != 100)
    {
        if (!triacFired[2] && currentCounter >= goalTicks[2] && goalTicks[2] > 0)
        {
            TRIAC_2_HIGH();
            triacPinState[2] = true;
            triacFired[2] = true;
        }
        else if (triacFired[2] && triacPinState[2])
        {
            TRIAC_2_LOW();
            triacPinState[2] = false;
        }
    }
}

#ifdef __AVR__
// Timer Interrupt: Handles triac firing delays
ISR(TIMER1_COMPA_vect)
{
    timer1Interrupt();
}
#elif defined(__STM32F1__)
void __Timer1(void)
{
    timer1Interrupt();
}
#endif

// Timer1 Setup
void setupTimer1()
{
#ifdef __AVR__
    cli();      // Disable interrupts
    TCCR1A = 0; // Clear Timer1 control registers
    TCCR1B = 0;
    TCCR1B |= (1 << WGM12);                      // Configure Timer1 in CTC mode
    TCCR1B |= (1 << CS11);                       // Set prescaler to 8 (2 MHz timer frequency)
    OCR1A = (timerIntervalMicroseconds * 2) - 1; // Set compare match value for 50 µs interval
    TIMSK1 |= (1 << OCIE1A);                     // Enable Timer1 compare interrupt
    sei();                                       // Enable interrupts
#elif defined(__STM32F1__)
    // Disable interruptions
    __disable irq()

        // Enable Timer1 clock
        RCC -> APB2ENR |= RCC_APB2ENR_TIM1EN;

    // Configure Timer1
    TIM1->CR1 = 0;                                   // Clear control register
    TIM1->CR2 = 0;                                   // Clear control register
    TIM1->PSC = 0;                                   // Set prescaler to 0
    TIM1->ARR = (timerIntervalMicroseconds * 2) - 1; // Set auto-reload value for 50 µs interval
    TIM1->DIER |= TIM_DIER_UIE;                      // Enable update interrupt
    TIM1->CR1 |= TIM_CR1_CEN;                        // Enable Timer1

    NVIC_EnableIRQ(TIM1_UP_IRQn); // Enable Timer1 interrupt
    __enable_irq();               // Enable interruptions
#endif
}

// Power Control Function
void setTriacPower(int triacIndex, int power)
{
    // Update last command time whenever power is set (except during timeout)
    lastPowerCommandTime = millis();
    
    if (power < 0)
        power = 0;
    // Fix positive overflow
    else if (power > 95)
        power = 100;

    powerLevel[triacIndex] = power;
    if (power == 0)
    {
        goalTicks[triacIndex] = 0; // Turn off the triac
        digitalWrite(triacPin[triacIndex], LOW); // Ensure pin is LOW
    }
    else if (power >= 100)
    {
        goalTicks[triacIndex] = 0;
        digitalWrite(triacPin[triacIndex], HIGH);
    }
    else
    {
        // Ensure pin is LOW before switching to phase control mode
        digitalWrite(triacPin[triacIndex], LOW);
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

void checkPowerTimeout()
{
    // Check if timeout has elapsed
    if (lastPowerCommandTime > 0 && (millis() - lastPowerCommandTime) > powerTimeoutMs)
    {
        // Timeout occurred - set all power to 0
        for (int i = 0; i < 3; i++)
        {
            if (powerLevel[i] != 0)
            {
                powerLevel[i] = 0;
                goalTicks[i] = 0;
                digitalWrite(triacPin[i], LOW);
            }
        }
        // Reset timestamp to prevent repeated triggering
        lastPowerCommandTime = 0;
    }
}

void power_control_loop()
{
    /* Animated power update PURPOSE */
    updateTriacPowerAnimated(0);
    updateTriacPowerAnimated(1);
    updateTriacPowerAnimated(2);
    delay(1000);
    
    // Check for power timeout
    checkPowerTimeout();
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

    logger_print("Setting TRIAC ");
    logger_print(String(index).c_str());
    logger_print(" power to ");
    logger_println(String(power[index]).c_str());

    setTriacPower(index, power[index]);
}
