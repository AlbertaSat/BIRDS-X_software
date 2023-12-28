
#include "experiment_ccd.h"
#include "mboss_handler.h" // for send_str_to_mboss() for debugging
#include "drivers/watchdog.h"
#include "antilib_adc.h"
#include <stdint.h>
#include "drivers/modem.h" // for afsk_disable_timers/afsk_restore_disabled_timers

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

void write_ccd_pins(uint8_t phi_m, uint8_t icg, uint8_t sh) {
	write_phi_m_pin(phi_m);
	write_icg_pin(icg);
	write_sh_pin(sh);
}

void fetch_ccd_measurement_and_log_it(uint8_t ccd_num) {
	// send_str_to_mboss_no_tail("DEBUG: boss_cmd_exp_ccd_do_debug_convert -> called");
	// delay_ms(50);

	uint8_t fetched_data[CCD_DATA_LEN_BYTES+1];
	
	// fill all bytes with zeros (for good measure)
	for (int i = 0; i < CCD_DATA_LEN_BYTES; i++) {
		fetched_data[i] = 0;
	}

	// fill the arrays with data
	// the sensor works better when it's "warmed up" (or operating continuously)
	for (uint8_t i = 0; i < 30; i++) {
		query_ccd_measurement(fetched_data, ccd_num);
	}
	Wdog_reset();

	// prep start of message
	char msg[60];
	sprintf(
		msg,
		"%sRESP: CCD%d data=[",
		MBOSS_RESPONSE_START_STR,
		ccd_num
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
	delay_ms(40);
	Wdog_reset();

	for (uint16_t i = 0; i < CCD_DATA_LEN_BYTES; i++) {
		if (i % 50 != 0) {
			// only print every bunch of bytes
			continue;
		}

		sprintf(
			msg,
			"%02X ",
			fetched_data[i]
		);

		// add a newline every 50 bytes
		// if (i % 50 == 0) {
		// 	uint16_t msg_len = strlen(msg);
		// 	msg[msg_len] = '\n';
		// 	msg[msg_len + 1] = '\0';
		// }

		// send
		term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
		delay_ms(10); // these will crazy it if you try to remove them
		Wdog_reset();
	}

	// prep end of message
	sprintf(
		msg,
		"]%s",
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);

	// TODO: check and log statistics data

}

void query_ccd_measurement(uint8_t *fetched_data, uint8_t ccd_num) {
	// this is a naive timer-less implementation

	uint8_t last_sh_val = 0;
	const uint16_t pixels_between_sh_toggles = 1000;

	// delay_ms(120);
	Wdog_reset();

	// configure the ADC for reading
	// hadc2.Instance->SQR3 = ADC_CHANNEL_6; // NOW DONE BELOW
	// Note: PA5 = IC601 = ADC_CHANNEL_5
	//       PA6 = IC701 = ADC_CHANNEL_6

	// disable UART for the duration of this function
	uart_config(&uart1, 0);
	uart_config(&uart2, 0);
	afsk_disable_timers();

	uint32_t adc_channel = ADC_CHANNEL_5; // refers to PA5 or PA6

	if (ccd_num == 1) {
		adc_channel = ADC_CHANNEL_5;
		
	}
	else if (ccd_num == 2) {
		adc_channel = ADC_CHANNEL_6;
	}
	ADC2->SQR3 = adc_channel;

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

	write_sh_pin(AFTER_INVERTER_ISRCE: ChatGPT
		// // Start the conversion
		// ADC2->CR2 |= ADC_CR2_SWSTART;
		// // Wait for the conversion to complete
		// while (!(ADC2->SR & ADC_SR_EOC));
		// // END SOURCE_LOW);

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

	uint16_t adc_val = 0;

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


		#if 1
		ADC2->SQR3 = adc_channel;
		
		// SOURCE: ChatGPT
		// Start the conversion
		ADC2->CR2 |= ADC_CR2_SWSTART;
		// Wait for the conversion to complete
		while (!(ADC2->SR & ADC_SR_EOC));
		// END SOURCE

		adc_val = (uint16_t) (ADC2->DR & 0x0FFF);

		#else
		adc_val = 1;
		#endif

		fetched_data[i] = (uint8_t)(adc_val >> 4);

		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		// asm("NOP"); // write_0 + NOP -> high for 250ns
		// asm("NOP"); // write_0 + NOP + NOP -> high for 360ns

		//HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, GPIO_PIN_SET);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);

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

	// stop recording
	write_icg_pin(AFTER_INVERTER_IS_LOW);
	write_sh_pin(AFTER_INVERTER_IS_HIGH);

	// write LED low
	HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_RESET);

	set_resting_ccd_state();

	// re-enable timers
	afsk_restore_disabled_timers();

	// re-enable uarts
	uart_config(&uart1, 1);
	uart_config(&uart2, 1);
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
	// Already done in Afsk_init()

    /* Enable clock for ADC & PortA */
    // orig: // RCC->APB2ENR |= RCC_APB2ENR_ADC2EN | RCC_APB2ENR_IOPAEN;
	RCC->APB2ENR |= RCC_APB2ENR_ADC2EN | RCC_APB2ENR_IOPAEN;

    /* Configure PA5 and PA6 in analog input mode */
    // GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE0);
	GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE0);
    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE0);

    /* Set sampling time = 28.5 cycles*/
    // orig: // ADC2->SMPR2 |= (ADC_SMPR2_SMP0_1 | ADC_SMPR2_SMP0_0);
	// other orig: ADC2->SMPR2 = ADC_SAMPLE_TIME6(SAMPLE_TIME_1_5);
    ADC2->SMPR2 = ADC_SAMPLE_TIME5(SAMPLE_TIME_1_5) | ADC_SAMPLE_TIME6(SAMPLE_TIME_1_5);

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
