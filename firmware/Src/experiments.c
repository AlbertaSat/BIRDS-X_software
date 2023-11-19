
#include "experiments.h"


// extern config settings
uint8_t config_enable_radfet_experiment = 0;
uint8_t config_enable_pin_experiment = 0;


uint16_t ADC_Read_Channel(uint32_t channel) {
	// Source: ChatGPT
	// TODO: consider replacing the Error_Handler with a reboot or return 0;

    uint16_t adcValue = 0;

    // Configure the channel to be converted
    hadc1.Instance->SQR3 = channel;

    // Start ADC conversion
    /*if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        Error_Handler();
    }*/

    // Wait for conversion to complete
    if (HAL_ADC_PollForConversion(&hadc1, 1000) == HAL_OK) {
        // Read ADC value
        adcValue = HAL_ADC_GetValue(&hadc1);
    } else {
        Error_Handler();
    }

    // Stop ADC conversion
    /*if (HAL_ADC_Stop(&hadc1) != HAL_OK) {
        Error_Handler();
    }*/

    return adcValue;
}

// TODO: create start/stop ADC functions, and call them from the RADFET enable place

