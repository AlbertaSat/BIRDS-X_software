#include <Arduino.h>

#include "main_loop_controller.h"
#include "pin_defs.h"
#include "command_uart_controller.h"
#include "rf_controller.h"

HardwareSerial TncSerial(LPUART1);

void run_once_at_boot() {
	init_boss_serial();

	pinMode(PIN_ONBOARD_LED, OUTPUT);

	// TODO: remove this
	// set UART to high-Z so we can use a regular adapter on it
	// pinMode(PIN_UART_TNC_TO_JASPER, INPUT);
	// pinMode(PIN_UART_JASPER_TO_TNC, INPUT);

	TncSerial.setRx(PIN_UART_TNC_TO_JASPER);
	TncSerial.setTx(PIN_UART_JASPER_TO_TNC);
	TncSerial.begin(38400); // source: https://nbviewer.org/github/mobilinkd/NucleoTNC/blob/master/Build/NucleoTNC.ipynb#Programming:~:text=is%20now%20operational!-,Operation,-Before%20we%20begin

}


void main_loop_single_iteration() {

	BossSerial.println("INFO: main_loop_single_iteration()");

	// do serial passthrough
	// TODO: revert this eventually
	while (BossSerial.available()) {
		TncSerial.write(BossSerial.read());
	}
	if (TncSerial.available()) {
		BossSerial.print("INFO: TNC serial incoming: ");
		while (TncSerial.available()) {
			BossSerial.write(TncSerial.read());
		}
	}

	digitalWrite(PIN_ONBOARD_LED, HIGH);
	delay(10);
	digitalWrite(PIN_ONBOARD_LED, LOW);
	delay(100);
	
}

