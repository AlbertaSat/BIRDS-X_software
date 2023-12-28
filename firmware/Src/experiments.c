
#include "experiments.h"
#include "antilib_adc.h"

#define RADFET_ADC_SAMPLE_TIME_CLOCK_CYCLES (SAMPLE_TIME_239_5)

uint16_t ADC_Read_Channel(uint32_t channel) {
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
}

void do_radfet_measurements(uint16_t* adc_val_radfet_1, uint16_t* adc_val_radfet_2, uint16_t* adc_val_radfet_3, uint16_t* adc_val_radfet_4) {
    init_adc_radfets();
    delay_ms(100); // probably not needed

	write_radfet_enable(1);
    delay_ms(400); // wait for voltage to stablize after turning on current source basically

	// read ADC values
	*adc_val_radfet_1 = get_radfet_measurement(1);
	*adc_val_radfet_2 = get_radfet_measurement(2);
	*adc_val_radfet_3 = get_radfet_measurement(3);
	*adc_val_radfet_4 = get_radfet_measurement(4);

	write_radfet_enable(0);
    deinit_adc_radfets(); // save power
}

uint16_t get_radfet_measurement(uint8_t radfet_num) {

    uint32_t sum_value = 0;
    const uint8_t num_samples = 128;
    
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

    /* Set ADC clock prescalar*/
    // RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6; // 64MHz / 6
    // Already done in Afsk_init()

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

void deinit_adc_radfets(void) {
    GPIOA->CRL &= ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE1);// PA1
    GPIOB->CRL &= ~(GPIO_CRL_CNF0 | GPIO_CRL_MODE0); // PB0
    GPIOB->CRL &= ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE1); // PB1
    GPIOA->CRL &= ~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4); // PA4

    ADC2->CR2 &= ~ADC_CR2_ADON;
    ADC2->CR2 &= ~ADC_CR2_CONT;
    ADC2->SMPR2 = 0;
}

void write_radfet_enable(uint8_t enable) {
    if (enable) {
        HAL_GPIO_WritePin(PIN_RFET_EN_OUT_GPIO_Port, PIN_RFET_EN_OUT_Pin, GPIO_PIN_SET);
    }
    else {
        HAL_GPIO_WritePin(PIN_RFET_EN_OUT_GPIO_Port, PIN_RFET_EN_OUT_Pin, GPIO_PIN_RESET);
    }
}
