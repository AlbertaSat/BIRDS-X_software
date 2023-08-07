// main_loop_controller.c

#include "all_includes.h"
#include "main_loop_controller.h"
#include "main.h"
#include <stdio.h>

void run_once_on_boot() {
	debug_println("Starting run_once_on_boot()");

	init_sx1276();



	debug_println("INFO: Done run_once_on_boot()");
}

void main_loop() {
	debug_println("INFO: Starting main_loop()");
	
	read_and_execute_next_incoming_command();

	// test the SPI stuff
	test_sx_spi();

}

