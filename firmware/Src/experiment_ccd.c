
#include "experiment_ccd.h"
#include "mboss_handler.h" // for send_str_to_mboss() for debugging
#include "drivers/watchdog.h"
#include "antilib_adc.h"
#include <stdint.h>

#define AFTER_INVERTER_IS_LOW 1
#define AFTER_INVERTER_IS_HIGH 0

inline void write_phi_m_pin(uint8_t val) {
	// HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, ...);
	// PB13
	if (val == 0) {
		GPIOB->BSRR = GPIO_BSRR_BR13;
	}
	else {
		GPIOB->BSRR = GPIO_BSRR_BS13;
	}
}

inline void write_icg_pin(uint8_t val) {
	// HAL_GPIO_WritePin(PIN_CCD_ICG_GPIO_Port, PIN_CCD_ICG_Pin, ...);
	// PB14
	if (val == 0) {
		GPIOB->BSRR = GPIO_BSRR_BR14;
	}
	else {
		GPIOB->BSRR = GPIO_BSRR_BS14;
	}
}

inline void write_sh_pin(uint8_t val) {
	// HAL_GPIO_WritePin(PIN_CCD_SH_GPIO_Port, PIN_CCD_SH_Pin, ...);
	// PB15
	if (val == 0) {
		GPIOB->BSRR = GPIO_BSRR_BR15;
	}
	else {
		GPIOB->BSRR = GPIO_BSRR_BS15;
	}
}

void experiment_ccd_init(void) {
	// TODO: implement


}

void fetch_ccd_measurement_and_log_it() {
	// TODO: implement

	uint8_t fetched_data_1[CCD_DATA_LEN_BYTES];
	uint8_t fetched_data_2[CCD_DATA_LEN_BYTES];
	query_ccd_measurement(fetched_data_1, fetched_data_2);
	
	// log the array to debug output (maybe averaging over adjacent bits for earth debugging)
	for (int i = 0; i < CCD_DATA_LEN_BYTES; i++) {
		// printf("%02X ", fetched_data[i]);
	}
	// printf("\n");

	// TODO: check and log statistics data

}

void query_ccd_measurement(uint8_t *fetched_data_1, uint8_t *fetched_data_2) {
	// this is a naive timer-less implementation


	uint8_t last_sh_val = 0;
	const uint8_t pixels_between_sh_toggles = 1000;

	#ifdef ENABLE_CCD_DEBUG_PRINTS
	send_str_to_mboss_no_tail("query_ccd_measurement -> called");
	#endif

	// delay_ms(120);
	Wdog_reset();

	// configure the ADC for reading
	// hadc2.Instance->SQR3 = ADC_CHANNEL_6; // NOW DONE BELOW
	// Note: PA5 = IC601 = ADC_CHANNEL_5
	//       PA6 = IC701 = ADC_CHANNEL_6
	// TODO: un-hardcode this
	
	#ifdef ENABLE_CCD_DEBUG_PRINTS
	send_str_to_mboss_no_tail("query_ccd_measurement -> configured ADC");
	delay_ms(40);
	Wdog_reset();
	#endif

	// DEBUG: write LED high to see how long this func takes
	HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_SET);


	#ifdef ENABLE_RANDOM_STARTUP_GARBAGE
	Wdog_reset();

	// pretend that it's always going
	for (uint16_t i = 0; i < 10; i++) {
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		// 2 and 2 give a T=1.5us
	}

	write_icg_pin(AFTER_INVERTER_IS_LOW);
	write_sh_pin(AFTER_INVERTER_IS_HIGH); // must be 0.1us to 1us after ICG=1

	// must give >1us delay before starting to read
	for (uint16_t i = 0; i < 5; i++) {
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		// 2 and 2 give a T=1.5us
	}

	write_sh_pin(AFTER_INVERTER_IS_LOW);

	// must give >1us (typ 5us) before starting to read
	for (uint16_t i = 0; i < 4; i++) {
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		// 2 and 2 give a T=1.5us
	}
	#endif

	// trigger the CCD reading
	write_phi_m_pin(AFTER_INVERTER_IS_HIGH); // PHI_M must be HIGH when ICG goes L->H
	write_icg_pin(AFTER_INVERTER_IS_HIGH); // indicates start of data transfer dump
	// SH is alread AFTER_INVERTER_IS_LOW

	// awful delay code to test integration
	// for (uint16_t i = 0; i < 1; i++) { // 350 gives 260us
	// 	write_icg_pin(AFTER_INVERTER_IS_HIGH);
	// }

	uint32_t adc_val = 0;

	for (uint16_t i = 0; i < CCD_DATA_LEN_BYTES; i++) { // CCD_DATA_LEN_BYTES
		// Wdog_reset();

		// HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, GPIO_PIN_RESET);
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		// asm("NOP"); // write_0 + NOP -> high for 250ns
		// asm("NOP"); // write_0 + NOP + NOP -> high for 360ns
		// asm("NOP"); // write_0 + NOP + NOP -> high for 600ns

		//delay_ms(20);
		//HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, GPIO_PIN_SET);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW); // no-op equivalent
		// asm("NOP");
		// asm("NOP");
		//delay_ms(20);

		// if (i < 3) { // FIXME
		// 	send_str_to_mboss_no_tail("query_ccd_measurement -> right before ADC");
		// 	delay_ms(40);
		// }

		// configure the ADC for reading
		// hadc2.Instance->SQR3 = ADC_CHANNEL_6; // 5 (PA5, IC601) or 6 (PA6, IC701)
		
		// read the ADC, and convert the 12-bit ADC value to an 8-bit value
		//adc_val = HAL_ADC_GetValue(&hadc2);

		#if 0
		// if (HAL_ADC_PollForConversion(&hadc2, 1) == HAL_OK) {
		// 	// Read ADC value
		// 	adc_val = HAL_ADC_GetValue(&hadc2);
		// }
		// else {
		// 	adc_val = 0;
		// }

		// adc_val = HAL_ADC_GetValue(&hadc2);
		adc_val = ADC2->DR;

		#else
		//adc_val = 1;
		#endif

		//fetched_data_1[i] = (uint8_t)(adc_val >> 4);

		// if (i < 3) { // FIXME
		// 	send_str_to_mboss_no_tail("query_ccd_measurement -> right after ADC");
		// 	delay_ms(40);
		// }

		//HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, GPIO_PIN_RESET);
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		// asm("NOP"); // write_0 + NOP -> high for 250ns
		// asm("NOP"); // write_0 + NOP + NOP -> high for 360ns

		//HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, GPIO_PIN_SET);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		// delay_ms(20);

		if (i % pixels_between_sh_toggles == 0 && i != 0) {
			// toggle SH
			if (last_sh_val == 0) {
				write_sh_pin(AFTER_INVERTER_IS_HIGH);
				last_sh_val = 1;
			}
			else {
				write_sh_pin(AFTER_INVERTER_IS_LOW);
				last_sh_val = 0;
			}
		}

	}

	#ifdef ENABLE_CCD_DEBUG_PRINTS
	send_str_to_mboss_no_tail("query_ccd_measurement -> after recording"); // FIXME
	delay_ms(40);
	#endif

	// stop recording
	write_icg_pin(AFTER_INVERTER_IS_LOW);
	write_sh_pin(AFTER_INVERTER_IS_HIGH);

	// write LED low
	HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_RESET);

	set_resting_ccd_state();
}

void set_resting_ccd_state(void) {
	write_sh_pin(AFTER_INVERTER_IS_LOW);
	write_icg_pin(AFTER_INVERTER_IS_LOW);
	write_phi_m_pin(AFTER_INVERTER_IS_LOW);
}


/**
 * @brief
 * Configure ADC in continuous mode
 */
void init_ccd_adc(void)
{
    /* Set ADC prescalar*/
    // RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;

    /* Enable clock for ADC & PortA */
    // orig: // RCC->APB2ENR |= RCC_APB2ENR_ADC2EN | RCC_APB2ENR_IOPAEN;
	RCC->APB2ENR |= RCC_APB2ENR_ADC2EN | RCC_APB2ENR_IOPAEN;

    /* Configure PA5 and PA6 in analog input mode */
    // GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE0); // FIXME: enable maybe
	GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE0);

    /* Set sampling time = 28.5 cycles*/
    // orig: // ADC2->SMPR2 |= (ADC_SMPR2_SMP0_1 | ADC_SMPR2_SMP0_0);
	ADC2->SMPR2 = ADC_SAMPLE_TIME6(SAMPLE_TIME_1_5);

    /* Put adc in Continuous mode and wake up from power down mode*/
    ADC2->CR2 |= (ADC_CR2_CONT | ADC_CR2_ADON);

    /* Set right data alignement */
    ADC2->CR2 &= ~ADC_CR2_ALIGN;

    /* Reset Caliberation registers */
    ADC2->CR2 |= (ADC_CR2_RSTCAL);
    while (ADC2->CR2 & ADC_CR2_RSTCAL)
        ;

    /* Start caliberation */
    ADC2->CR2 |= (ADC_CR2_CAL);
    while (ADC2->CR2 & ADC_CR2_CAL)
        ;

    /* Start conversion */
    ADC2->CR2 |= ADC_CR2_ADON;

	set_resting_ccd_state();
}


// TODO: create a function to store statistics data
