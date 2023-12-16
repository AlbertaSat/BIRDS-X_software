
#include "experiment_ccd.h"


void experiment_ccd_init(void) {
	// TODO: implement


}

void fetch_ccd_measurement_and_log_it(ccd_sensor_t ccd_sensor) {
	// TODO: implement

	uint8_t fetched_data[CCD_DATA_LEN_BYTES];
	query_ccd_measurement(fetched_data, ccd_sensor);
	
	// log the array to debug output (maybe averaging over adjacent bits for earth debugging)
	for (int i = 0; i < CCD_DATA_LEN_BYTES; i++) {
		// printf("%02X ", fetched_data[i]);
	}
	// printf("\n");

	// TODO: check and log statistics data

}

void query_ccd_measurement(uint8_t fetched_data[], ccd_sensor_t ccd_sensor) {
	// TODO: implement
}


// TODO: create a function to store statistics data


