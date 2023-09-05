
#ifndef __TEMPERATURE_SENSORS_H__
#define __TEMPERATURE_SENSORS_H__

#include "main.h"

uint16_t get_internal_temperature_k();

// sensor_index = 0,1,2,3,4
uint16_t get_external_temperature_k(uint8_t sensor_index);

#endif   // __TEMPERATURE_SENSORS_H__
