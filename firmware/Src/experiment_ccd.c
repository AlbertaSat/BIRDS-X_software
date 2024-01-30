
#include "experiment_ccd.h"
#include "mboss_handler.h" // for send_str_to_mboss() for debugging
#include "drivers/watchdog.h"
#include "antilib_adc.h"
#include <stdint.h>
#include "drivers/modem.h" // for afsk_disable_timers/afsk_restore_disabled_timers
#include "frame_handler.h" // cringe, but easy way to store the poll data as a packet (store_frame_for_boss)

// a poll_period is how often the CCD data is fetched and the counts are updated
// a stat_period is how often the CCD data is logged to the packet store, and the stats are reset
// a fetch is a a subset of a poll; multiple fetches happen per poll

// period config
uint8_t ccd_config_poll_period_sec = 10; // [default: 10 = 0x0A] configurable via command, extern; 0 to disable
uint16_t ccd_config_stat_period_sec = 240; // [default: 240 = 0xF0] configurable via command, extern; 0 to disable

// operational/functional config
uint16_t ccd_config_pixels_per_shutter = 50; // [default: 50] configurable via command, extern
uint8_t ccd_config_fetches_per_poll = 10; // [must be <10; default 10] via command, extern
uint8_t ccd_config_alert_threshold_points = 10; // configurable via command, extern

// period tracking
uint32_t ccd_result_stat_period_start_uptime_sec = 0;
uint32_t ccd_result_last_poll_period_start_uptime_sec = 0;

// stats since boot (not really utlized; could make a command that gets them)
uint32_t ccd1_result_triggered_pix_count_since_boot = 0;
uint32_t ccd2_result_triggered_pix_count_since_boot = 0;
uint32_t ccd_result_total_pix_count_since_boot = 0;
uint16_t ccd_result_poll_count_since_boot = 0;

// stats in last stat_period
uint16_t ccd1_result_triggered_pix_count_in_last_period = 0;
uint16_t ccd2_result_triggered_pix_count_in_last_period = 0;
uint32_t ccd_result_total_pix_count_in_last_period = 0;
uint16_t ccd_result_poll_count_in_last_period = 0;

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

/*
 * Main action for boss_cmd_exp_ccd_do_debug_convert() exclusively.
*/
void fetch_ccd_measurement_and_log_it(uint8_t ccd_num, uint16_t elements_per_group) {
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
	char msg[50];
	sprintf(
		msg,
		"%sRESP: CCD%d elements_per_group=%d, data=[",
		MBOSS_RESPONSE_START_STR,
		ccd_num,
		elements_per_group
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
	delay_ms(40);
	Wdog_reset();

	char msg2[250];
	msg2[0] = 0;
	for (uint16_t i = 0; i < CCD_DATA_LEN_BYTES; i++) {
		if ((i % elements_per_group) != 0) {
			// TODO: average within the group instead

			// only print every bunch of bytes
			continue;
		}

		sprintf(
			&msg2[strlen(msg2)],
			"%02X ",
			fetched_data[i]
		);

		// send in chuncks
		if (strlen(msg2) > 240) {
			term_sendToMode((uint8_t*)msg2, strlen(msg2), MODE_BOSS);
			delay_ms(10); // these will crazy it if you try to remove them
			Wdog_reset();
			msg2[0] = 0;
		}
	}
	
	// final chunck
	if (strlen(msg2) > 0) {
		term_sendToMode((uint8_t*)msg2, strlen(msg2), MODE_BOSS);
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

/*
 * Fetches all pixels for one CCD into a buffer.
 * Used for both the boss_cmd_exp_ccd_do_debug_convert() command, and the periodic polling.
 */
void query_ccd_measurement(uint8_t *fetched_data, uint8_t ccd_num) {
	// this is a naive timer-less implementation
	uint8_t last_sh_val = 0;

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

	init_adc_ccd(ccd_num);

	uint32_t adc_channel = ADC_CHANNEL_5; // refers to PA5 or PA6

	if (ccd_num == 1) {
		adc_channel = ADC_CHANNEL_5;
		
	}
	else if (ccd_num == 2) {
		adc_channel = ADC_CHANNEL_6;
	}
	ADC2->SQR3 = adc_channel;

	// DEBUG: write LED high to see how long this func takes
	// HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_SET);

	// trigger the CCD reading
	write_phi_m_pin(AFTER_INVERTER_IS_HIGH); // PHI_M must be HIGH when ICG goes L->H
	write_icg_pin(AFTER_INVERTER_IS_HIGH); // indicates start of data transfer dump
	// SH is alread AFTER_INVERTER_IS_LOW

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

		ADC2->SQR3 = adc_channel;
		
		// SOURCE: ChatGPT
		// Start the conversion
		ADC2->CR2 |= ADC_CR2_SWSTART;
		// Wait for the conversion to complete
		while (!(ADC2->SR & ADC_SR_EOC));
		// END SOURCE

		adc_val = (uint16_t) (ADC2->DR & 0x0FFF);

		// adc_val = 1; // for debugging

		fetched_data[i] = (uint8_t)(adc_val >> 4);

		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		write_phi_m_pin(AFTER_INVERTER_IS_HIGH);
		// asm("NOP"); // write_0 + NOP -> high for 250ns
		// asm("NOP"); // write_0 + NOP + NOP -> high for 360ns

		//HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, GPIO_PIN_SET);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);
		write_phi_m_pin(AFTER_INVERTER_IS_LOW);

		if (i % ccd_config_pixels_per_shutter == 0 && i != 0) {
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
	// HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_RESET);

	set_resting_ccd_state();
	deinit_adc_ccd();

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
void init_adc_ccd(uint8_t ccd_num)
{
    /* Set ADC prescalar*/
    // RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;
	// Already done in Afsk_init()

    /* Enable clock for ADC & PortA */
    // orig: // RCC->APB2ENR |= RCC_APB2ENR_ADC2EN | RCC_APB2ENR_IOPAEN;
	RCC->APB2ENR |= RCC_APB2ENR_ADC2EN | RCC_APB2ENR_IOPAEN;

    /* Configure PA5 and PA6 in analog input mode */
    // GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE0);
	if (ccd_num == 1)
		GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE0);
	else if (ccd_num == 2)
	    GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE0);

    /* Set sampling time = 28.5 cycles*/
    // orig: // ADC2->SMPR2 |= (ADC_SMPR2_SMP0_1 | ADC_SMPR2_SMP0_0);
	// other orig: ADC2->SMPR2 = ADC_SAMPLE_TIME6(SAMPLE_TIME_1_5);
	if (ccd_num == 1)
    	ADC2->SMPR2 = ADC_SAMPLE_TIME5(SAMPLE_TIME_1_5);
	else if (ccd_num == 2)
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

void deinit_adc_ccd(void) {
	GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE0); // PA5 (CCD1)
	GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE0); // PA6 (CCD2)

	ADC2->CR2 &= ~ADC_CR2_ADON;
	ADC2->CR2 &= ~ADC_CR2_CONT;
	ADC2->SMPR2 = 0;
}

void loop_service_ccd_experiment() {
	if (timestamp_sec_at_boot == 0) {
		// skip doing anything before a timestamp is set, as it'll make the data kinda nasty
		return;
	}

	if (ccd_config_poll_period_sec == 0 || ccd_config_stat_period_sec == 0) {
		// disabled
		
		// reset the logged values
		ccd1_result_triggered_pix_count_in_last_period = 0;
		ccd2_result_triggered_pix_count_in_last_period = 0;
		ccd_result_total_pix_count_in_last_period = 0;
		ccd_result_poll_count_in_last_period = 0;
		
		return;
	}

	if (get_system_uptime_sec() - ccd_result_last_poll_period_start_uptime_sec > ccd_config_poll_period_sec) {
		ccd_result_last_poll_period_start_uptime_sec = get_system_uptime_sec();
		//HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_SET);

		if (get_system_uptime_sec() - ccd_result_stat_period_start_uptime_sec > ccd_config_stat_period_sec) {
			// log to the CCD results
			char log_result[100];
			sprintf(
				log_result,
				"CCD_res,ts=%lu_%lu,trig=%d,%d,TOT=%lu,p=%d",
				ccd_result_stat_period_start_uptime_sec + timestamp_sec_at_boot, // start timestamp (of stats period)
				get_unix_timestamp_sec_now(), // end timestamp (of stats period)
				ccd1_result_triggered_pix_count_in_last_period, // triggered count (CCD1)
				ccd2_result_triggered_pix_count_in_last_period, // triggered count (CCD2)
				ccd_result_total_pix_count_in_last_period, // total pixels in stats period
				ccd_result_poll_count_in_last_period // poll count (number of times triggered from the superloop)
			);
			store_frame_for_boss((uint8_t*)log_result, strlen(log_result));

			// reset stat period
			ccd_result_stat_period_start_uptime_sec = get_system_uptime_sec();

			// reset the logged values
			ccd1_result_triggered_pix_count_in_last_period = 0;
			ccd2_result_triggered_pix_count_in_last_period = 0;
			ccd_result_total_pix_count_in_last_period = 0;
			ccd_result_poll_count_in_last_period = 0;
		}

		// run the experiment
		ccd_result_poll_count_since_boot++;
		ccd_result_poll_count_in_last_period++;
		for (uint8_t fetch_num = 0; fetch_num < ccd_config_fetches_per_poll; fetch_num++) {
			do_one_fetch_of_ccd_experiment(); // polls both CCDs
		}
	}

	// HAL_GPIO_WritePin(PIN_LED_D304_GPIO_Port, PIN_LED_D304_Pin, GPIO_PIN_RESET);
}


/*
 * Updates the CCD measurements for both CCDs (e.g., ccd1_result_triggered_pix_count_in_last_period, etc.)
 * Called from within the loop_service_ccd_experiment function only.
*/
void do_one_fetch_of_ccd_experiment() {
	uint16_t pix_count = (CCD_DATA_LEN_BYTES - CCD_PIXELS_IGNORE_START - CCD_PIXELS_IGNORE_END);

	for (uint8_t ccd_num = 1; ccd_num <= 2; ccd_num++) {
		uint8_t fetched_data[CCD_DATA_LEN_BYTES];
		query_ccd_measurement(fetched_data, ccd_num);

		// first find the average, min, and max (but didn't end up using the min/max)
		uint32_t sum = 0;
		// uint8_t min_val = 0xFF; // set high so that everything is less
		// uint8_t max_val = 0; // set low so that everything is greater
		for (uint16_t pix_num = CCD_PIXELS_IGNORE_START; pix_num < CCD_DATA_LEN_BYTES - CCD_PIXELS_IGNORE_END; pix_num++) {
			sum += fetched_data[pix_num];

			// if (fetched_data[pix_num] < min_val) {
			// 	min_val = fetched_data[pix_num];
			// }
			// if (fetched_data[pix_num] > max_val) {
			// 	max_val = fetched_data[pix_num];
			// }
		}
		uint8_t avg = sum / pix_count;
		// uint8_t spread = max_val - min_val;
		uint8_t threshold_val = avg - ccd_config_alert_threshold_points;

		// now, count the pixels that exceed the threshold
		for (uint16_t pix_num = CCD_PIXELS_IGNORE_START; pix_num < CCD_DATA_LEN_BYTES - CCD_PIXELS_IGNORE_END; pix_num++) {
			if (fetched_data[pix_num] < threshold_val) {
				// this pixel is "brighter" than the threshold, so it is a galactic cosmic ray
				
				if (ccd_num == 1) {
					ccd1_result_triggered_pix_count_since_boot++;
					ccd1_result_triggered_pix_count_in_last_period++;
				}
				else if (ccd_num == 2) {
					ccd2_result_triggered_pix_count_since_boot++;
					ccd2_result_triggered_pix_count_in_last_period++;
				}
			}
		}
	}

	ccd_result_total_pix_count_since_boot += pix_count;
	ccd_result_total_pix_count_in_last_period += pix_count;
}
