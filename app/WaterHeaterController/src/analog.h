#pragma once

#if defined(__STM32F1__) || defined(__STM32__)
ADC_HandleTypeDef hadc1;
#endif

volatile uint16_t adc_result = 0;

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
#endif
}

void Start_ADC_Conversion()
{
#if defined(__STM32F1__) || defined(__STM32__)
    //THROTTLE(10000);
    HAL_ADC_Start_IT(&hadc1);
#elif defined(__AVR__)
    // For AVR, read analog directly
    adc_result = analogRead(temp_sensor_adc_pin);
#endif
}

#if defined(__STM32F1__) || defined(__STM32__)
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_result = HAL_ADC_GetValue(hadc);
    }
}
#endif

uint16_t getAdcResult()
{
    return adc_result;
}

void adc_loop() {
    THROTTLE(10000);
    Start_ADC_Conversion();
}