/*
 * rf_controller.c
 *
 *  Created on: Aug 4, 2023
 *      Author: Parker
 */

#include "all_includes.h"

// scope: this file only
SX1278_hw_t SX1278_hw;
SX1278_t SX1278;


void init_sx1276() {

	debug_println("INFO: Starting init_sx1276()");

	// set port config
	//SX1278_hw.dio0.port = DIO0_GPIO_Port; // TODO: assign DIO pin with a jumper wire (only used for LoRa RX/TX currently)
	//SX1278_hw.dio0.pin = DIO0_Pin;
	SX1278_hw.nss.port = GPIOB;
	SX1278_hw.nss.pin = GPIO_PIN_0;
	//SX1278_hw.reset.port = RESET_GPIO_Port; // NOT ASSIGNED // TODO: check if there are any issues with this
	//SX1278_hw.reset.pin = RESET_Pin;
	SX1278_hw.spi = &hspi1;
	SX1278.hw = &SX1278_hw;
	SX1278.op_mode = OP_MODE_OOK; // use either OP_MODE_OOK or OP_MODE_FSK

	debug_println("INFO: Past the assignment init section.");


	// the following is based losely on `SX1278_config(&SX1278);`
	//	SX1278_init(
	//		&SX1278, 145825000, SX1278_POWER_17DBM, SX1278_LORA_SF_7,
	//		SX1278_LORA_BW_125KHZ, SX1278_LORA_CR_4_5, SX1278_LORA_CRC_EN, 10
	//	);

	// set FSK/OOK mode
	SX1278_sleep(&SX1278);
	SX1278_hw_DelayMs(100);
	sx_debug_print_a_register(RegVersion, "RegVersion (should be 0x12)");

	SX1278_hw_DelayMs(15);
	SX1278_entryFSK(&SX1278); // for now, set OOK=True (as that's what's suggested in the guide)

	sx_debug_print_a_register(RegOpMode, "RegOpMode (after entryFSK)");

	// set carrier freq
	SX1278.frequency = 145825000;
	SX1278_setFreq(&SX1278);

	// set rx bitrate
	float bitrate_khz = 26.4; // 26.4 kHz = (2200 Hz * 1200 Hz / 100) // TODO: should be able so make this work at 13.2 kHz (LCM of the 2 numbers)
	SX1278_FSKSetBitRate(&SX1278, bitrate_khz);

	// set RX bandwidth
	// TODO: optionally set freqDev, rxBw, power (all left as defaults) in the test code
	// RadioLib Debugs: int16_t beginFSK(
		// float freq = 434.0
		// float br = 4.8 // bitrate
		// float freqDev = 5.0
		// float rxBw = 125.0
		// int8_t power = 10
		// uint16_t preambleLength = 16
		// bool enableOOK = false


	//

	// set the frequency

	debug_println("INFO: Done init_sx1276()");

}

void check_sx1276_and_log_status() {
	// HAL_GPIO_ReadPin(RF_DIO2_GPIO_Port, RF_DIO2_Pin); // TODO: enable this and log it very constantly
}



void test_sx_spi() {
	// just do it here I guess

	debug_println("INFO: Starting test_sx_spi()");

	sx_debug_print_a_register(RegVersion, "RegVersion (should be 0x12 or 0x22)");


	//SX1278_init(module, frequency, power, LoRa_SF, LoRa_BW, LoRa_CR, LoRa_CRC_sum, packetLength)
}

void sx_activate_rx_mode() {

}

void sx_debug_print_a_register(uint8_t register_address, uint8_t register_name_cstr[]) {
	// register_address can be something like RegOpMode (0x01) or RegVersion (0x42)

	// do a check: read back the hex value (should be 0x28 with OOK and 0x08 without OOK)
	SX1278_hw_DelayMs(15); // safety
	uint8_t register_value = SX1278_SPIRead(&SX1278, register_address);
	const uint8_t msg1[255];
	sprintf(msg1, "INFO: %s register_value = 0x%02X (hex)", register_name_cstr, register_value);
	debug_println(msg1);
	SX1278_hw_DelayMs(15); // safety

}
