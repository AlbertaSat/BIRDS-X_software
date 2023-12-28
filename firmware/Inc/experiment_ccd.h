
#ifndef __EXPERIMENT_CCD_H__
#define __EXPERIMENT_CCD_H__

#include "common.h"
#include "main.h"

// CCD Selection (there are two sensors onboard)
typedef enum {
    CCD_SENSOR_1 = 1,
    CCD_SENSOR_2 = 2
} ccd_sensor_t;

#define CCD_DATA_LEN_BYTES 1546 // real: 1546


void query_ccd_measurement(uint8_t *fetched_data, uint8_t ccd_num);

void fetch_ccd_measurement_and_log_it(uint8_t ccd_num, uint16_t elements_per_group);

void init_adc_ccd(uint8_t ccd_num);
void deinit_adc_ccd(void);

void set_resting_ccd_state(void);

void write_ccd_pins(uint8_t phi_m, uint8_t icg, uint8_t sh);


#endif  // __EXPERIMENT_CCD_H__
