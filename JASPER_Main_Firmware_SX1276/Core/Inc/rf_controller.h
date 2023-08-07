/*
 * rf_controller.h
 *
 *  Created on: Aug 4, 2023
 *      Author: Parker
 */

#ifndef INC_RF_CONTROLLER_H_
#define INC_RF_CONTROLLER_H_

#include "all_includes.h"

void init_sx1276();

void check_sx1276_and_log_status();


void sx_debug_print_a_register(uint8_t register_address, uint8_t register_name_cstr[]);

void test_sx_spi();

#endif /* INC_RF_CONTROLLER_H_ */
