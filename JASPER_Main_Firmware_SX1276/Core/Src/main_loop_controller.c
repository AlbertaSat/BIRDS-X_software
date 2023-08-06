// main_loop_controller.c

#include "all_includes.h"
#include "main_loop_controller.h"
#include "main.h"
#include <stdio.h>

void run_once_on_boot() {
	//DebugSerial.printf("Starting run_once_on_boot()\n");

	init_sx1276();



	//DebugSerial.printf("Done run_once_on_boot()\n");
}

void main_loop() {
	//DebugSerial.printf("Starting main_loop()\n"); // FIXME: get this print working
	
	read_and_execute_next_incoming_command();

	// test the SPI stuff
	test_sx_spi();

}

