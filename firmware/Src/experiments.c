
#include "experiments.h"
#include "antilib_adc.h"

// extern config settings
uint8_t config_enable_radfet_experiment = 0;
uint8_t config_enable_pin_experiment = 0;

#define RADFET_ADC_SAMPLE_TIME_CLOCK_CYCLES (SAMPLE_TIME_239_5)

uint16_t ADC_Read_Channel(uint32_t channel) {

    uint16_t adcValue = 0;

    // Configure the channel to be converted
    ADC2->SQR3 = channel;

    // SOURCE: ChatGPT
    // Start the conversion
    ADC2->CR2 |= ADC_CR2_SWSTART;
    // Wait for the conversion to complete
    while (!(ADC2->SR & ADC_SR_EOC));
    // END SOURCE

    // delay_ms(20); // try waiting for conversion

    return (uint16_t) (ADC2->DR & 0x0FFF);

    return adcValue;
}


// TODO: create start/stop ADC functions, and call them from the RADFET enable place

uint16_t get_radfet_measurement(uint8_t radfet_num) {
    uint32_t sum_value = 0;
    const uint8_t num_samples = 8;
    
    uint32_t radfet_channel = ADC_CHANNEL_1; // default to radfet 1
    if (radfet_num == 1) {
        radfet_channel = ADC_CHANNEL_1;
    }
    else if (radfet_num == 2) {
        radfet_channel = ADC_CHANNEL_8;
    }
    else if (radfet_num == 3) {
        radfet_channel = ADC_CHANNEL_9;
    }
    else if (radfet_num == 4) {
        radfet_channel = ADC_CHANNEL_4;
    }
    
    for (uint8_t i = 0; i < num_samples; i++) {
        sum_value += ADC_Read_Channel(radfet_channel);
    }

    return (uint16_t)(sum_value / num_samples);
}

void init_adc_radfets(void)
{
    // "Configure ADC in continuous mode"

    /* Set ADC prescalar*/
    // RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;

    /* Enable clock for ADC & PortA */
    // orig: // RCC->APB2ENR |= RCC_APB2ENR_ADC2EN | RCC_APB2ENR_IOPAEN;
	RCC->APB2ENR |= RCC_APB2ENR_ADC2EN | RCC_APB2ENR_IOPAEN;

    /* Configure PA5/PA6 (CCD) in analog input mode */
    // GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE0);
	// GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE0);

    /* Configure PA1/PB0/PB1/PA4 in analog input mode */
    GPIOA->CRL &= ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE0);
    GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0);
    GPIOB->CRL &= ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE0);
    GPIOA->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE0);

    /* Set sampling time = 28.5 cycles*/
    // orig: // ADC2->SMPR2 |= (ADC_SMPR2_SMP0_1 | ADC_SMPR2_SMP0_0);
	// ADC2->SMPR2 = ADC_SAMPLE_TIME6(SAMPLE_TIME_1_5);
    ADC2->SMPR2 = ADC_SAMPLE_TIME1(RADFET_ADC_SAMPLE_TIME_CLOCK_CYCLES) | ADC_SAMPLE_TIME8(RADFET_ADC_SAMPLE_TIME_CLOCK_CYCLES) | ADC_SAMPLE_TIME9(RADFET_ADC_SAMPLE_TIME_CLOCK_CYCLES) | ADC_SAMPLE_TIME4(RADFET_ADC_SAMPLE_TIME_CLOCK_CYCLES);

    /* Put adc in Continuous mode and wake up from power down mode*/
    ADC2->CR2 |= (ADC_CR2_CONT | ADC_CR2_ADON);

    /* Set right data alignment */
    ADC2->CR2 &= ~ADC_CR2_ALIGN;

    /* Reset Calibration registers */
    ADC2->CR2 |= (ADC_CR2_RSTCAL);
    while (ADC2->CR2 & ADC_CR2_RSTCAL)
        ;

    /* Start caliberation */
    ADC2->CR2 |= (ADC_CR2_CAL);
    while (ADC2->CR2 & ADC_CR2_CAL)
        ;

    /* Start conversion */
    ADC2->CR2 |= ADC_CR2_ADON;
}
