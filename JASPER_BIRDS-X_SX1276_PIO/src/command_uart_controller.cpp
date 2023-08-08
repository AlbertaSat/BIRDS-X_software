#include <HardwareSerial.h>
#include <Arduino.h>
#include "command_uart_controller.h"

HardwareSerial BossSerial(USART1);

void init_boss_serial() {
	
	BossSerial.setRx(PB7);
	BossSerial.setTx(PB6);
	BossSerial.begin(115200);
	BossSerial.println("\n\nINFO: Booting JASPER SX1276 Board (Rev2) with PIO Firmware");
	BossSerial.print("INFO: Build date: ");
	BossSerial.print(__DATE__);
	BossSerial.print(" ");
	BossSerial.println(__TIME__);

}

