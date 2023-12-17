
#include "experiment_ccd.h"
#include "mboss_handler.h" // for send_str_to_mboss() for debugging
#include "drivers/watchdog.h"

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

	send_str_to_mboss_no_tail("query_ccd_measurement -> called");
	delay_ms(120);
	Wdog_reset();

	// configure the ADC for reading
	hadc1.Instance->SQR3 = ADC_CHANNEL_6;
	// Note: PA5 = IC601 = ADC_CHANNEL_5
	//       PA6 = IC701 = ADC_CHANNEL_6
	// TODO: un-hardcode this
	
	send_str_to_mboss_no_tail("query_ccd_measurement -> configured ADC");
	delay_ms(40);
	Wdog_reset();

	// start recording: set SH=LOW, wait 50us, set ICG=HIGH [but opposite bc inverter in path]
	HAL_GPIO_WritePin(PIN_CCD_SH_GPIO_Port, PIN_CCD_SH_Pin, GPIO_PIN_SET);
	// delay_ms(10); // good enough for now; supposed to be nearly nothing
	HAL_GPIO_WritePin(PIN_CCD_ICG_GPIO_Port, PIN_CCD_ICG_Pin, GPIO_PIN_RESET);

	send_str_to_mboss_no_tail("query_ccd_measurement -> set init pin vals");
	delay_ms(40);
	Wdog_reset();

	uint32_t adc_val = 0;

	for (uint16_t i = 0; i < CCD_DATA_LEN_BYTES; i++) {
		Wdog_reset();

		HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, GPIO_PIN_RESET);
		//delay_ms(20);
		HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, GPIO_PIN_SET);
		//delay_ms(20);

		if (i < 3) { // FIXME
			send_str_to_mboss_no_tail("query_ccd_measurement -> right before ADC");
			delay_ms(40);
		}

		// read the ADC, and convert the 12-bit ADC value to an 8-bit value
		adc_val = HAL_ADC_GetValue(&hadc1);
		fetched_data_1[i] = (uint8_t)(adc_val >> 4);

		// alternative:
		// if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK) {
		// 	// Read ADC value
		// 	// adcValue = HAL_ADC_GetValue(&hadc1);
		// 	fetched_data_1[i] = HAL_ADC_GetValue(&hadc1);
		// }
		// else {
		// 	fetched_data_1[i] = 42; // hardcoded read error
		// }
		//fetched_data_1[i] = 99;

		if (i < 3) { // FIXME
			send_str_to_mboss_no_tail("query_ccd_measurement -> right after ADC");
			delay_ms(40);
		}

		HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(PIN_CCD_PHI_M_GPIO_Port, PIN_CCD_PHI_M_Pin, GPIO_PIN_SET);
	}

	send_str_to_mboss_no_tail("query_ccd_measurement -> after recording"); // FIXME
	delay_ms(40);
	Wdog_reset();

	// stop recording
	HAL_GPIO_WritePin(PIN_CCD_ICG_GPIO_Port, PIN_CCD_ICG_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(PIN_CCD_SH_GPIO_Port, PIN_CCD_SH_Pin, GPIO_PIN_RESET);
}


// TODO: create a function to store statistics data
