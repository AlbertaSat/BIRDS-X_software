#include "main_loop_controller.h"

#include "command_uart_controller.h"
#include "rf_controller.h"

void run_once_at_boot() {
	init_boss_serial();

	init_sx1276();

}


void main_loop_single_iteration() {
	do_test_sx1276();
	
}

