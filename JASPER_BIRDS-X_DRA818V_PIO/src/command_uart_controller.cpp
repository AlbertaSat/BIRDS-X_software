#include <HardwareSerial.h>
#include <Arduino.h>

#include "command_uart_controller.h"
#include "pin_defs.h"

HardwareSerial BossSerial(USART1);

void init_boss_serial() {
	
	BossSerial.setRx(PIN_UART_BOSS_TO_JASPER);
	BossSerial.setTx(PIN_UART_JASPER_TO_BOSS);
	BossSerial.begin(115200);

	BossSerial.println("\n\nINFO: Booting JASPER SX1276 Board (Rev2) with PIO Firmware");
	BossSerial.print("INFO: Build date: ");
	BossSerial.println(__TIMESTAMP__);

}

