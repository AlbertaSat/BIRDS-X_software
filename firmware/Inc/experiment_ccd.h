
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


void query_ccd_measurement(uint8_t *fetched_data_1, uint8_t *fetched_data_2);

void init_ccd_adc(void);

void set_resting_ccd_state(void);


#endif  // __EXPERIMENT_CCD_H__
