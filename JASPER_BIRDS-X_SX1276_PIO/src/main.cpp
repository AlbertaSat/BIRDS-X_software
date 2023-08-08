#include <Arduino.h>

HardwareSerial BossSerial(USART1);

void setup() {

	BossSerial.setRx(PB7);
	BossSerial.setTx(PB6);
	BossSerial.begin(115200);
	BossSerial.println("INFO: Booting JASPER SX1276 Board (Rev2) with PIO Firmware");

	// Debug LEDs in output mode
	pinMode(PC6, OUTPUT);
	pinMode(PA11, OUTPUT);


	uint64_t superloop_counter = 0;
	while (1) {
		BossSerial.printf("INFO: Starting loop() (superloop_counter=%lu)\n", superloop_counter++);

		// write PC6 and PA11 to blink
		digitalWrite(LED_BUILTIN, HIGH);
		digitalWrite(PA11, HIGH);

		delay(500);

		digitalWrite(LED_BUILTIN, LOW);
		digitalWrite(PA11, LOW);
		delay(1000);
	}

}

void loop() {
	BossSerial.println("ERROR: Please reboot the board. Superloop broke.");
	delay(1000);
}
