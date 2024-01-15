
#ifndef __EXPERIMENT_CCD_H__
#define __EXPERIMENT_CCD_H__

#include "common.h"
#include "main.h"


// Period config
extern uint8_t ccd_config_poll_period_sec;  // [default: 15] configurable via command, extern; 0 to disable
extern uint16_t ccd_config_stat_period_sec;  // [default: 240] configurable via command, extern; 0 to disable

// Operational/functional config
extern uint16_t ccd_config_pixels_per_shutter;       // configurable via command, extern
extern uint8_t ccd_config_fetches_per_poll;           // configurable via command, extern
extern uint8_t ccd_config_alert_threshold_points;    // configurable via command, extern


#define CCD_DATA_LEN_BYTES 1546 // real: 1546
#define CCD_PIXELS_IGNORE_START 64 // only need 32, but ignore some extras
#define CCD_PIXELS_IGNORE_END 32 // only need 14, but ignore some extras

// #define CCD_ENABLE_DEBUG_PRINTS 1 // comment to disable

void query_ccd_measurement(uint8_t *fetched_data, uint8_t ccd_num);

void fetch_ccd_measurement_and_log_it(uint8_t ccd_num, uint16_t elements_per_group);

void init_adc_ccd(uint8_t ccd_num);
void deinit_adc_ccd(void);

void set_resting_ccd_state(void);

void write_ccd_pins(uint8_t phi_m, uint8_t icg, uint8_t sh);

void loop_service_ccd_experiment(void);
void do_one_fetch_of_ccd_experiment();

#endif  // __EXPERIMENT_CCD_H__
