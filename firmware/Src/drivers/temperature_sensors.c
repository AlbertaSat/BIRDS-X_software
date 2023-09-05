
#include "drivers/temperature_sensors.h"
#include "main.h"
#include <stm32f1xx_hal_i2c.h>

#define TRUE 1
#define FALSE 0

uint16_t get_internal_temperature_k() {
	// FIXME: implement this, eventually
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

	const uint8_t resolution = 9; // default, 0.5C sensitivity, fastest reads
	// return ((read_data[0] >> (16 - resolution)) * 1000) >> (resolution - 8);

	// return read_data[1]; // TODO: include the other byte read

	uint16_t temp_as_9_bits = (read_data[0] << 1 | read_data[1] >> 7);
	uint16_t temp_unsigned_c = temp_as_9_bits >> 1; // divide by 2, because 0.5C per bit
	
	// execute two-complement conversion
	int16_t temp_signed_c;
	if (temp_unsigned_c & 0x0100) {
		temp_signed_c = -1 * ((~temp_unsigned_c + 1) & 0x01FF);
	} else {
		temp_signed_c = temp_unsigned_c;
	}

	// convert to kelvin
	uint16_t temp_k = temp_signed_c + 273;
	return temp_k;
}
