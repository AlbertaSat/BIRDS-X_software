
#include "mboss_handler.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// create a command table for the BOSS, which is a list of byte arrays and function pointers
// for example, when the bytes are 0xE0 0xFF, call the boss_cmd_turn_off_payload() function
// for example, when the bytes are 0xE0 0x0E, call the boss_cmd_turn_on_digipeat_mode() function

BossCommandEntry boss_command_table[] = {
	{ 0xFF, boss_cmd_turn_off_payload },
	{ 0x0E, boss_cmd_turn_on_digipeat_mode },
	{ 0x1E, boss_cmd_turn_off_sf_mode },
	// TODO: add more

	{ 0x10, boss_cmd_echo_command }
};

// TODO: make some sort of check that there are no duplicates in the command table

void debug_echo_back_mboss_command(uint8_t *cmd, uint16_t len, Terminal_stream src) {
	char received_msg_as_hex[100];
	for (int i = 0; i < len; i++) {
		char hex[10];
		sprintf(hex, "%02X ", cmd[i]);
		strcat(received_msg_as_hex, hex);
	}
	strcat(received_msg_as_hex, "<END>\n");

	char msg[255];
	sprintf(
		msg,
		"%sDEBUG: echo cmd: len=%d, hex=%s%s",
		MBOSS_RESPONSE_START_STR, len, received_msg_as_hex, MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);

}

void receive_incoming_boss_cmd(uint8_t *cmd, uint16_t len, Terminal_stream src) {

	// FIXME: disable this, or make it a flag
	debug_echo_back_mboss_command(cmd, len, src);
	

	if (validate_incoming_boss_cmd(cmd, len, src) == 0) {
		char err_msg[255];
		sprintf(
			err_msg,
			"%sERROR: BOSS command failed validation%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(err_msg, strlen(err_msg), MODE_BOSS);
		return;
	}
	
	// run the actual command
	for (uint8_t i = 0; i < sizeof(boss_command_table)/sizeof(BossCommandEntry); i++) {
		if (cmd[1] == boss_command_table[i].cmd_byte) {
			boss_command_table[i].cmd_func(cmd, src);
			break;
		}
	}

}

uint8_t validate_incoming_boss_cmd(uint8_t *cmd, uint16_t len, Terminal_stream src) {
	// command should be 9 bytes (MBOSS_COMMAND_LENGTH)
	if (len != MBOSS_COMMAND_LENGTH) {
		char err_msg[255];
		sprintf(
			err_msg,
			"%sERROR: BOSS command must be %d bytes, but received %d bytes instead%s",
			MBOSS_RESPONSE_START_STR, MBOSS_COMMAND_LENGTH, len, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(err_msg, strlen(err_msg), MODE_BOSS);
		return 0;
	}

	// command should start with 0xE0 (MBOSS_COMMAND_START_BYTE)
	if (cmd[0] != MBOSS_COMMAND_START_BYTE) {
		char err_msg[255];
		sprintf(
			err_msg,
			"%sERROR: BOSS command must start with 0xE0, but received 0x%02X instead%s",
			MBOSS_RESPONSE_START_STR, cmd[0], MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(err_msg, strlen(err_msg), MODE_BOSS);
		return 0;
	}

	// command should end with MBOSS_COMMAND_END_BYTE
	if (cmd[8] != MBOSS_COMMAND_END_BYTE) {
		char err_msg[255];
		sprintf(
			err_msg,
			"%sERROR: BOSS command must end with 0xED, but received 0x%02X instead%s",
			MBOSS_RESPONSE_START_STR, cmd[8], MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(err_msg, strlen(err_msg), MODE_BOSS);
		return 0;
	}

	// command should not contain any 0xE0 or 0xED bytes in the middle
	for (uint8_t i = 1; i < MBOSS_COMMAND_LENGTH-1; i++) {
		if (cmd[i] == MBOSS_COMMAND_START_BYTE || cmd[i] == MBOSS_COMMAND_END_BYTE) {
			char err_msg[255];
			sprintf(
				err_msg,
				"%sERROR: BOSS command cannot contain 0xE0 or 0xED in the middle, but received 0x%02X at index %d%s",
				MBOSS_RESPONSE_START_STR, cmd[i], i, MBOSS_RESPONSE_END_STR
			);
			term_sendToMode(err_msg, strlen(err_msg), MODE_BOSS);
			return 0;
		}
	}

	// check that it's a valid command, by looping through all the options in the command table
	uint8_t found = 0;
	for (uint8_t i = 0; i < sizeof(boss_command_table)/sizeof(BossCommandEntry); i++) {
		if (cmd[1] == boss_command_table[i].cmd_byte) {
			found = 1;
			break;
		}
	}
	if (found == 0) {
		char err_msg[255];
		sprintf(
			err_msg,
			"%sERROR: BOSS command byte (cmd[1]=%02X) is not in command table%s",
			MBOSS_RESPONSE_START_STR, cmd[1], MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(err_msg, strlen(err_msg), MODE_BOSS);
		return 0;
	}

	return 1;

}


void boss_cmd_turn_off_payload(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}
void boss_cmd_turn_on_digipeat_mode(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}
void boss_cmd_turn_off_sf_mode(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}




void boss_cmd_echo_command(uint8_t *cmd, Terminal_stream src) {
	char received_msg_as_hex[100];
	for (int i = 0; i < MBOSS_COMMAND_LENGTH; i++) {
		char hex[10];
		sprintf(hex, "%02X ", cmd[i]);
		strcat(received_msg_as_hex, hex);
	}
	strcat(received_msg_as_hex, "<END>\n");

	char msg[255];
	sprintf(
		msg,
		"%sECHO: len=%d, hex=%s%s",
		MBOSS_RESPONSE_START_STR, MBOSS_COMMAND_LENGTH, received_msg_as_hex, MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen(msg), MODE_BOSS);
}
