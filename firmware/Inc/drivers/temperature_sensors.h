
#ifndef __TEMPERATURE_SENSORS_H__
#define __TEMPERATURE_SENSORS_H__

#include "main.h"
#include "common.h"

uint16_t get_internal_temperature_k();

#define FAKE_SENSOR_TEMP_ON_BOOT 21
extern uint16_t min_sensor_temp_k, max_sensor_temp_k;

// sensor_index = 0,1,2,3,4
uint16_t get_external_temperature_k(uint8_t sensor_index);

#endif   // __TEMPERATURE_SENSORS_H__
