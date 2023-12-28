
#ifndef __EXPERIMENTS_H__
#define __EXPERIMENTS_H__

#include "common.h"
#include "main.h"

uint16_t ADC_Read_Channel(uint32_t channel);

void do_radfet_measurements(uint16_t* adc_val_radfet_1, uint16_t* adc_val_radfet_2, uint16_t* adc_val_radfet_3, uint16_t* adc_val_radfet_4);

uint16_t get_radfet_measurement(uint8_t radfet_num);

void init_adc_radfets(void);
void deinit_adc_radfets(void);

void write_radfet_enable(uint8_t enable);

#endif  // __EXPERIMENTS_H__
