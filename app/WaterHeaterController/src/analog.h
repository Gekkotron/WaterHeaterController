#pragma once

#if defined(__STM32F1__) || defined(__STM32__)
ADC_HandleTypeDef hadc1;
#endif

// Number of samples averaged per reading to reduce noise
#define ADC_SAMPLES 16

volatile uint16_t adc_result = 0;

#if defined(__STM32F1__) || defined(__STM32__)
volatile uint8_t  adc_sample_count = 0;
volatile uint32_t adc_accumulator = 0;
#endif

void adc_setup(void)
{
#if defined(__STM32F1__) || defined(__STM32__)
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Configure PB0 as analog input
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_8; // PB0
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    // Enable ADC interrupt
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
#elif defined(__AVR__)
    // Configure ADC for AVR
    pinMode(temp_sensor_adc_pin, INPUT);

    // Use the stable on-chip 1.1V bandgap reference instead of the noisy AVCC.
    // Signal is < 1.1V, so this also improves resolution (~1.07mV/count).
    analogReference(INTERNAL1V1);

    // Discard the first reads: the AREF cap needs time to settle after a
    // reference change, otherwise early samples are invalid.
    for (uint8_t i = 0; i < 4; i++) {
        analogRead(temp_sensor_adc_pin);
        delay(2);
    }
#endif
}

void Start_ADC_Conversion()
{
#if defined(__STM32F1__) || defined(__STM32__)
    //THROTTLE(10000);
    adc_accumulator = 0;
    adc_sample_count = 0;
    HAL_ADC_Start_IT(&hadc1);
#elif defined(__AVR__)
    // For AVR, read analog directly and average ADC_SAMPLES reads
    uint32_t sum = 0;
    for (uint8_t i = 0; i < ADC_SAMPLES; i++) {
        sum += analogRead(temp_sensor_adc_pin);
    }
    adc_result = sum / ADC_SAMPLES;
#endif
}

#if defined(__STM32F1__) || defined(__STM32__)
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_accumulator += HAL_ADC_GetValue(hadc);
        adc_sample_count++;

        if (adc_sample_count < ADC_SAMPLES)
        {
            // Keep the conversion going until ADC_SAMPLES are collected
            HAL_ADC_Start_IT(hadc);
        }
        else
        {
            adc_result = adc_accumulator / ADC_SAMPLES;
            adc_accumulator = 0;
            adc_sample_count = 0;
        }
    }
}
#endif

uint16_t getAdcResult()
{
    return adc_result;
}

uint16_t getAdcVoltageMv()
{
    // Convert ADC result to millivolts. Cast to 32-bit to avoid overflow
    // (raw * ref exceeds 16-bit on AVR).
#if defined(__STM32F1__) || defined(__STM32__)
    // STM32: reference ~3.3V, 12-bit ADC (4096 levels)
    return (uint16_t)(((uint32_t)adc_result * 3300) / 4095);
#elif defined(__AVR__)
    // AVR: internal 1.1V reference, 10-bit ADC (1024 levels)
    return (uint16_t)(((uint32_t)adc_result * 1100) / 1023);
#endif
}

void adc_loop() {
    THROTTLE(10000);
    Start_ADC_Conversion();
}