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

	//SX1278_hw.dio0.port = DIO0_GPIO_Port; // TODO: assign DIO pin with a jumper wire
	//SX1278_hw.dio0.pin = DIO0_Pin;
	SX1278_hw.nss.port = GPIOB;
	SX1278_hw.nss.pin = GPIO_PIN_0;
	//SX1278_hw.reset.port = RESET_GPIO_Port; // NOT ASSIGNED // TODO: check if there are any issues with this
	//SX1278_hw.reset.pin = RESET_Pin;
	SX1278_hw.spi = &hspi1;

	SX1278.hw = &SX1278_hw;

	debug_println("INFO: Past the assignment init section.");

	SX1278_init(
		&SX1278, 434000000, SX1278_POWER_17DBM, SX1278_LORA_SF_7,
		SX1278_LORA_BW_125KHZ, SX1278_LORA_CR_4_5, SX1278_LORA_CRC_EN, 10
	);
	debug_println("INFO: Done init_sx1276()");

}

void check_sx1276_and_log_status() {

}



void test_sx_spi() {
	// just do it here I guess

	debug_println("INFO: Starting test_sx_spi()");


	//SX1278_init(module, frequency, power, LoRa_SF, LoRa_BW, LoRa_CR, LoRa_CRC_sum, packetLength)
}
