
#include <Arduino.h>

#include "rf_controller.h"
#include "command_uart_controller.h"
#include "main_loop_controller.h"


void setup() {

	run_once_at_boot();

	uint64_t superloop_counter = 0;
	while (1) {
		BossSerial.printf("INFO: Starting loop() (superloop_counter=%lu)\n", superloop_counter++);

		main_loop_single_iteration();
		
	}

}

void loop() {
	BossSerial.println("ERROR: Please reboot the board. Superloop broke.");
	delay(1000);
}
