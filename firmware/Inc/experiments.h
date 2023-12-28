
#ifndef __EXPERIMENTS_H__
#define __EXPERIMENTS_H__

#include "common.h"
#include "main.h"

extern uint8_t config_enable_radfet_experiment;
extern uint8_t config_enable_pin_experiment;


uint16_t ADC_Read_Channel(uint32_t channel);

uint16_t get_radfet_measurement(uint8_t radfet_num);

void init_adc_radfets(void);

#endif  // __EXPERIMENTS_H__
