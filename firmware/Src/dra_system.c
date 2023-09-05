
#include "dra_system.h"
#include "terminal.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// whether to enable echoing of DRA commands to the BOSS
const uint8_t debug_enable_dra_echo_to_boss = 1; // FIXME: disable

// whether to enable echoing DRA responses to the BOSS
const uint8_t debug_enable_dra_response_logs_to_boss = 1; // FIXME: disable


void receive_incoming_dra_message(uint8_t *msg, uint16_t len, Terminal_stream src) {

	if (debug_enable_dra_response_logs_to_boss) {
		const char boss_pre_message[] = "DEBUG: From DRA >>";
		term_sendToMode(boss_pre_message, strlen(boss_pre_message), MODE_BOSS);
		
		term_sendToMode(msg, len, MODE_BOSS); // null termination not guaranteed (don't use strlen)
	}
}


void send_str_to_dra(uint8_t *str) {
	term_sendToMode(str, strlen(str), MODE_DRA);

	if (debug_enable_dra_echo_to_boss) {
		const char boss_pre_message[] = "DEBUG: To DRA >>";
		term_sendToMode(boss_pre_message, strlen(boss_pre_message), MODE_BOSS);
		
		term_sendToMode(str, strlen(str), MODE_BOSS);
	}
}

void send_dra_init_commands() {
	const uint16_t delay_ms_between_commands = 1000;
	
	// Init connection
	send_str_to_dra("AT+DMOCONNECT\r\n");
	// TODO: add a check that it worked
	delay_ms(delay_ms_between_commands);

	// Send channel configuration
	// 1 = 25k channel (no other options, says it works well enough at 12.5k)
	// 145.8250,145.8250 = TX freq, RX freq
	// 0000,0,0000 = CTCSS TX, Carrier squelch level, CTCSS RX
	send_str_to_dra("AT+DMOSETGROUP=1,145.8250,145.8250,0000,0,0000\r\n");
	delay_ms(delay_ms_between_commands);

	// Adjust volume 1-8
	send_str_to_dra("AT+DMOSETVOLUME=1\r\n");
	delay_ms(delay_ms_between_commands);

	// Turn off pre/de-emphasis, filters
	// Format: pre/de (inverted),highpass,lowpass
	// With this, need to set config: "flat on"
	send_str_to_dra("AT+SETFILTER=1,0,0\r\n");
	delay_ms(delay_ms_between_commands);

}

void set_dra_awake_mode(uint8_t new_state) {
	// TODO: implement this and remove bodge wire, if there's time
	if (new_state == 0) {
		// turn off PA11 (LOW)
	}
	else {
		// turn on PA11 (HIGH)
	}
}

