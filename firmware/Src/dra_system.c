
#include "dra_system.h"
#include "terminal.h"
#include "common.h"
#include "main.h" // for HAL_GPIO

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// whether to enable echoing of DRA commands to the BOSS
const uint8_t debug_enable_dra_echo_to_boss = 0;

// whether to enable echoing DRA responses to the BOSS
const uint8_t debug_enable_dra_response_logs_to_boss = 0;

uint8_t latest_dra_response_buf[DRA_RESPONSE_BUF_SIZE]; // extern

void receive_incoming_dra_message(uint8_t *msg, uint16_t len) {
	// gets called from Loop_process_incoming_uart_commands -> term_parse; do not use in here

	if (debug_enable_dra_response_logs_to_boss) {
		const char boss_pre_message[] = "DEBUG: From DRA >>";
		term_sendToMode((uint8_t*)boss_pre_message, strlen(boss_pre_message), MODE_BOSS);
		
		term_sendToMode((uint8_t*)msg, len, MODE_BOSS); // null termination not guaranteed (don't use strlen)
	}

	uint16_t i;
	for (i = 0; (i < len) && (i < DRA_RESPONSE_BUF_SIZE-1); i++) {
		latest_dra_response_buf[i] = msg[i];
	}
	latest_dra_response_buf[i+1] = '\0';
}


void send_str_to_dra(uint8_t *str) {
	term_sendToMode(str, strlen((char*)str), MODE_DRA);

	if (debug_enable_dra_echo_to_boss) {
		const char boss_pre_message[] = "DEBUG: To DRA >>";
		term_sendToMode((uint8_t*)boss_pre_message, strlen(boss_pre_message), MODE_BOSS);
		
		term_sendToMode(str, strlen((char*)str), MODE_BOSS);
	}
}

uint8_t send_dra_init_commands() {
	const uint16_t delay_ms_between_commands = 300;

	uint8_t error_val = 0;
	
	// Init connection
	send_str_to_dra((uint8_t*)"AT+DMOCONNECT\r\n");
	delay_ms(delay_ms_between_commands);
	Loop_process_incoming_uart_commands(0, 1);
	if (strstr((char *)latest_dra_response_buf, "+DMOCONNECT:0") == NULL) {
		error_val |= 1 << 1; // 2
	}

	// Send channel configuration
	// 1 = 25k channel (no other options, says it works well enough at 12.5k)
	// 145.8250,145.8250 = TX freq, RX freq
	// 0000,0,0000 = CTCSS TX, Carrier squelch level, CTCSS RX
	send_str_to_dra((uint8_t*)"AT+DMOSETGROUP=1,145.8250,145.8250,0000,0,0000\r\n");
	delay_ms(delay_ms_between_commands);
	Loop_process_incoming_uart_commands(0, 1);
	if (strstr((char *)latest_dra_response_buf, "+DMOSETGROUP") == NULL) {
		error_val |= 1 << 2; // 4
	}

	// Adjust volume 1-8
	send_str_to_dra((uint8_t*)"AT+DMOSETVOLUME=8\r\n");
	delay_ms(delay_ms_between_commands);
	Loop_process_incoming_uart_commands(0, 1);
	if (strstr((char *)latest_dra_response_buf, "+DMOSETVOLUME") == NULL) {
		error_val |= 1 << 3; // 8
	}

	// Turn off pre/de-emphasis, filters
	// Format: pre/de (inverted),highpass,lowpass
	// With this, need to set config: "flat on"
	send_str_to_dra((uint8_t*)"AT+SETFILTER=1,0,0\r\n");
	delay_ms(delay_ms_between_commands);
	Loop_process_incoming_uart_commands(0, 1);
	if (strstr((char *)latest_dra_response_buf, "+DMOSETFILTER:") == NULL) {
		error_val |= 1 << 4; // 16
	}

	return error_val;
}

void set_dra_awake_mode(uint8_t new_state) {
	if (new_state == 0) {
		// turn off PA11 (LOW)
		HAL_GPIO_WritePin(PIN_DRA_ENABLE_GPIO_Port, PIN_DRA_ENABLE_Pin, GPIO_PIN_RESET);
	}
	else {
		// turn on PA11 (HIGH)
		HAL_GPIO_WritePin(PIN_DRA_ENABLE_GPIO_Port, PIN_DRA_ENABLE_Pin, GPIO_PIN_SET);
	}
}
