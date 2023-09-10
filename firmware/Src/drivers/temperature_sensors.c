
#include "drivers/temperature_sensors.h"
#include "main.h"
#include <stm32f1xx_hal_i2c.h>

#define TRUE 1
#define FALSE 0

uint16_t get_internal_temperature_k() {
	// Referencing https://github.com/avislab/STM32F103/
	int adc_value;
	int temperature;
	const uint16_t V25 = 1750;// when V25=1.41V at ref 3.3V
	const uint16_t Avg_Slope = 5; //when avg_slope=4.3mV/C at ref 3.3V

	// FIXME: implement get_internal_temperature_k()
	adc_value = ADC_GetConversionValue(ADC1);
	temperature = (uint16_t)((V25-adc_value)/Avg_Slope+25);
	return 1;
}

uint16_t get_external_temperature_k(uint8_t sensor_index) {
	
	// sensor model: STDS75DS2F

	// sensor_index = 0 => read_reg = 0x91
	// sensor_index = 1 => read_reg = 0x93 ...
	// sensor_index = 4 => read_reg = 0x99

	uint8_t read_reg = 0x91 + (sensor_index << 1);

	// start read
	uint8_t read_data[2] = {0, 0};
	HAL_I2C_Mem_Read(
		&hi2c2, read_reg, 0x00, I2C_MEMADD_SIZE_8BIT, read_data, 2, 1000
	);

	// const uint8_t resolution = 9; // default, 0.5C sensitivity, fastest reads
	
	uint8_t sign_bit = read_data[0] & (1 << 7); // 0=positive, 1=negative; 2's complement
	uint16_t temp_as_9_bits = (read_data[0] << 1 | read_data[1] >> 7);
	uint16_t temp_unsigned_c = temp_as_9_bits >> 1; // divide by 2, because 0.5C per bit
	// TODO: maybe do sign extension here?
	// FIXME: this probably doesn't work with negative values
	
	// execute two-complement conversion
	int8_t sign = (sign_bit == 0) ? 1 : -1; // Determine the sign
	int16_t absolute_value = (temp_unsigned_c & ~(1 << 15)); // Calculate absolute value
	int16_t temp_signed_c = sign * absolute_value;

	// convert to kelvin
	uint16_t temp_k = temp_signed_c + 273;
	return temp_k;
}
