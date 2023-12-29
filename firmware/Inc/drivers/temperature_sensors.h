
#ifndef __TEMPERATURE_SENSORS_H__
#define __TEMPERATURE_SENSORS_H__

#include "main.h"
#include "common.h"

#define FAKE_SENSOR_TEMP_ON_BOOT 21
extern uint16_t min_sensor_temp_k, max_sensor_temp_k;

// sensor_index = 0,1,2,3,4
uint16_t get_external_temperature_k(uint8_t sensor_index);

void get_external_temperature_k_raw(uint8_t sensor_index, uint8_t read_data[]);

#endif   // __TEMPERATURE_SENSORS_H__
