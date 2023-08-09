
#include <RadioLib.h>

#include "rf_controller.h"
#include "command_uart_controller.h"
#include <SPI.h>

SPIClass spi(PB5, PB4, PB3, PB0); // mosi, miso, sclk, CS (used from here)
SPISettings spiSettings(1000000, MSBFIRST, SPI_MODE0); // TODO: increase back to 8 MHz
SX1276 radio = new Module(PB0, RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC, spi, spiSettings); // cs, irq, rst, gpio (maybee DIO2 or DIO1)

void init_sx1276() {
	BossSerial.println("INFO: Starting init_sx1276()");

	// init SPI bus
	spi.begin();

	// check that the SPI works by reading the version register
	uint8_t version = radio.getChipVersion();
	while (version != 0x12 && version != 0x22) {
		BossSerial.printf("WARNING: Bad RF SPI version register fetch: 0x%02X. Retrying...\n", version);
		version = radio.getChipVersion();
		delay(200);
	}
	BossSerial.printf("INFO: SPI version register: 0x%02X (should be 0x12 on SX1276, or 0x22 on others)\n", version);

	BossSerial.println("DEBUG: running radio.standby();");
	radio.standby();
	BossSerial.println("DEBUG: done radio.standby();");

	// do the actual init/begin of the radio
	int state = radio.beginFSK(145.825, 26.4); // 26.4 kHz = 2200 Hz * 1200 Hz (from Bell 202 modulation)
	if (state == RADIOLIB_ERR_NONE) {
		BossSerial.println(F("INFO: radio.beginFSK success!"));
	} else {
		BossSerial.printf("INFO: radio.beginFSK failed, code %d, hanging\n", state);
		BossSerial.println("ERROR: Please reboot the board. RadioLib init failed.");
		while (true) delay(100);
	}

	radio.setOOK(true); // TODO: confirm this is good
	BossSerial.println(F("INFO: radio.setOOK(true) complete!"));

	radio.setDirectSyncWord(0x3F03F03F, 32);

	// set function that will be called each time a bit is received
	radio.setDirectAction(readBitFromRadio);

	// start direct mode reception
	radio.receiveDirect();

	BossSerial.println("INFO: Done init_sx1276()");
}

// this function is called when a new bit is received
void readBitFromRadio(void) {
  // read the data bit
  radio.readBit(PA15); // DIO2 connected to PA15 via jumper wire
}

void do_test_sx1276() {

	BossSerial.println("INFO: Starting do_test_sx1276()");

	if (radio.available() > 22) {
		BossSerial.println("INFO: radio.available() > 22");

		radio.standby(); // put in standby mode to read out // TODO: check if this is necessary (might miss packets received while reading)

		uint8_t data[1000]; // each element is going to be a bit; this is an ineffecient way to store the data
		uint16_t i = 0;
		while (radio.available()) {
			
			data[i++] = radio.read();
		}

		// do something with the data buffer
		// for now, print the data buffer
		BossSerial.printf("INFO: data buffer (len=%d): ", i);
		for (uint16_t j = 0; j < i; j++) {
			BossSerial.printf("%d", data[j]);
		}

		// TODO: add a demod here
		
		radio.receiveDirect(); // put back in receive mode
	}
	else {
		BossSerial.println("INFO: radio.available() <= 22");
	}


	BossSerial.println("INFO: Done do_test_sx1276()");

}

