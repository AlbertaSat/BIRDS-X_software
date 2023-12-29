
#include "drivers/temperature_sensors.h"
#include "main.h"
#include <stm32f1xx_hal_i2c.h>

// externs
uint16_t min_sensor_temp_k=FAKE_SENSOR_TEMP_ON_BOOT, max_sensor_temp_k=FAKE_SENSOR_TEMP_ON_BOOT;

uint16_t get_external_temperature_k(uint8_t sensor_index) {
	// sensor model: STDS75DS2F

	// start read
	uint8_t read_data[2] = {0, 0};
	get_external_temperature_k_raw(sensor_index, read_data);

	// const uint8_t resolution = 9; // default, 0.5C sensitivity, fastest reads
	
	// Example:
	// -15: 1 1111 0001 => -15 * 0.5 = -7.5C = 265.5K
    //      1111 1000 1XXX XXXX
	// uint8_t read_data[2] = {0xF8, 0x80};

	// read the literal value, in 0.5C resolution
	int16_t literal_value = (read_data[0] << 1) | (read_data[1] >> 7); // rightshift each by 16-9 (and the [0] bit is leftshifted by 8 already)

	// If the most significant bit (MSB) is set, indicating a negative number, sign-extend
	if (literal_value & 0x0100) {
		literal_value |= 0xFF00;
	}

	// convert to signed
	int16_t temp_signed_c = literal_value >> 1; // *0.5C-per-literal

	// convert to kelvin
	uint16_t temp_k = temp_signed_c + 273;

	// update the global min/max store
	if (temp_k > max_sensor_temp_k || max_sensor_temp_k == FAKE_SENSOR_TEMP_ON_BOOT) {
		max_sensor_temp_k = temp_k;
	}
	if (temp_k < min_sensor_temp_k || min_sensor_temp_k == FAKE_SENSOR_TEMP_ON_BOOT) {
		min_sensor_temp_k = temp_k;
	}
	
	return temp_k;
}

void get_external_temperature_k_raw(uint8_t sensor_index, uint8_t read_data[]) {
	// sensor model: STDS75DS2F

	// sensor_index = 0 => read_reg = 0x91
	// sensor_index = 1 => read_reg = 0x93 ...
	// sensor_index = 4 => read_reg = 0x99

	uint8_t read_reg = 0x91 + (sensor_index << 1);

	// start read
	HAL_I2C_Mem_Read(
		&hi2c2, read_reg, 0x00, I2C_MEMADD_SIZE_8BIT, read_data, 2, 1000
	);
}
