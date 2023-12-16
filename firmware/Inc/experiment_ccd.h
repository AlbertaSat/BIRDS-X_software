
#ifndef __EXPERIMENT_CCD_H__
#define __EXPERIMENT_CCD_H__

#include "common.h"
#include "main.h"

// CCD Selection (there are two sensors onboard)
typedef enum {
    CCD_SENSOR_1 = 1,
    CCD_SENSOR_2 = 2
} ccd_sensor_t;

#define CCD_DATA_LEN_BYTES 1546



#endif  // __EXPERIMENT_CCD_H__
