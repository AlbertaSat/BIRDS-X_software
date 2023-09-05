
#include "mboss_handler.h"
#include "common.h"
#include "sys_reboot_reason.h"
#include "drivers/temperature_sensors.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// create a command table for the BOSS, which is a list of byte arrays and function pointers
// for example, when the bytes are 0xE0 0xFF, call the boss_cmd_turn_off_payload() function
// for example, when the bytes are 0xE0 0x0E, call the boss_cmd_turn_on_digipeat_mode() function

BossCommandEntry boss_command_table[] = {
	{0xFF, boss_cmd_turn_off_payload},
	{0x0E, boss_cmd_set_active_aprs_mode},
	{0x12, boss_cmd_transfer_data_packets},
	{0x03, boss_cmd_send_temperature},
	{0x04, boss_cmd_enable_pin_diode_experiment},
	{0x05, boss_cmd_disable_pin_diode_experiment},
	{0x06, boss_cmd_enable_radfet_experiment},
	{0x07, boss_cmd_disable_radfet_experiment},
	{0x08, boss_cmd_enable_both_experiments},
	{0x09, boss_cmd_disable_both_experiments},
	{0x10, boss_cmd_echo_command},
	{0x13, boss_cmd_set_pin_diode_polling_time},
	{0x14, boss_cmd_set_radfet_polling_time},
	{0x15, boss_cmd_set_both_polling_time},
	{0x16, boss_cmd_set_unix_timestamp},
	{0x17, boss_cmd_set_unix_timestamp_shutdown},
	{0x18, boss_cmd_run_power_on_self_test},
	{0x19, boss_cmd_force_reboot_system},
	{0x20, boss_cmd_set_beacon_period},
	{0x21, boss_cmd_clear_flash_memory},
	{0x22, boss_cmd_exit_mission_boss_mode},
	{0x23, boss_cmd_get_sys_uptime_and_reboot_reason},
	{0x24, boss_cmd_get_unix_timestamp}
};

// TODO: make some sort of check that there are no duplicates in the command table

void debug_echo_back_mboss_command(uint8_t *cmd, uint16_t len, Terminal_stream src) {
	char received_msg_as_hex[100] = "";
	for (int i = 0; i < len; i++) {
		char hex[10];
		sprintf(hex, "%02X ", cmd[i]);
		strcat(received_msg_as_hex, hex);
	}
	strcat(received_msg_as_hex, "<END>");

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

void boss_cmd_set_active_aprs_mode(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_transfer_data_packets(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_send_temperature(uint8_t *cmd, Terminal_stream src) {
	uint16_t internal_temp_k = get_internal_temperature_k();

	char msg[255];
	sprintf(
		msg,
		"%sRESP: internal_temp_k=%d, ext_temp_0_k=%d, ext_temp_1_k=%d, ext_temp_2_k=%d, ext_temp_3_k=%d, ext_temp_4_k=%d%s",
		MBOSS_RESPONSE_START_STR,
		internal_temp_k,
		get_external_temperature_k(0), get_external_temperature_k(1), get_external_temperature_k(2), get_external_temperature_k(3), get_external_temperature_k(4),
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
}

void boss_cmd_enable_pin_diode_experiment(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_disable_pin_diode_experiment(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_enable_radfet_experiment(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_disable_radfet_experiment(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_enable_both_experiments(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_disable_both_experiments(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_echo_command(uint8_t *cmd, Terminal_stream src) {
	char received_msg_as_hex[100] = "";
	for (int i = 0; i < MBOSS_COMMAND_LENGTH; i++) {
		char hex[10];
		sprintf(hex, "%02X ", cmd[i]);
		strcat(received_msg_as_hex, hex);
	}
	strcat(received_msg_as_hex, "<END>");

	char msg[255];
	sprintf(
		msg,
		"%sRESP: len=%d, hex=%s%s",
		MBOSS_RESPONSE_START_STR, MBOSS_COMMAND_LENGTH, received_msg_as_hex, MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen(msg), MODE_BOSS);
}


void boss_cmd_set_pin_diode_polling_time(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_set_radfet_polling_time(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_set_both_polling_time(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_set_unix_timestamp(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_set_unix_timestamp_shutdown(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_run_power_on_self_test(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_force_reboot_system(uint8_t *cmd, Terminal_stream src) {
	uint8_t cmd_password[9] = { 0xE0, 0x19, 0x35, 0xA6, 0x32, 0x18, 0xD3, 0xFF, 0xED };
	
	if (check_cmd_password(cmd, cmd_password)) {
		char msg[255];
		sprintf(
			msg,
			"%sRESP: Rebooting...%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);
		
		// FIXME: do reboot
	}
	else {
		char msg[255];
		sprintf(
			msg,
			"%sERROR: password is incorrect%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);
	}
}

void boss_cmd_set_beacon_period(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}

void boss_cmd_clear_flash_memory(uint8_t *cmd, Terminal_stream src) {
	uint8_t cmd_password[9] = { 0xE0, 0x21, 0x37, 0x56, 0xCD, 0x21, 0x3D, 0xEE, 0xED };
	
	if (check_cmd_password(cmd, cmd_password)) {
		char msg[255];
		sprintf(
			msg,
			"%sRESP: Exiting mission BOSS mode%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);

		// FIXME: clear flash mem
	}
	else {
		char msg[255];
		sprintf(
			msg,
			"%sERROR: password is incorrect%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);
	}

	
}

void boss_cmd_exit_mission_boss_mode(uint8_t *cmd, Terminal_stream src) {
	uint8_t cmd_password[9] = { 0xE0, 0x22, 0x38, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xED };
	
	if (check_cmd_password(cmd, cmd_password)) {
		char msg[255];
		sprintf(
			msg,
			"%sRESP: Exiting mission BOSS mode%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);
		switchPortToMonitorMode(src);
	}
	else {
		char msg[255];
		sprintf(
			msg,
			"%sERROR: password is incorrect%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);
	}
}

void boss_cmd_get_sys_uptime_and_reboot_reason(uint8_t *cmd, Terminal_stream src) {
	uint32_t system_uptime_ms = get_system_uptime_ms();

	// reset_cause_t reset_cause = reset_cause_get(); // FIXME: make this work
	
	char msg[200];
	sprintf(
		msg,
		"%sRESP: uptime_ms=%lu, reset_cause_str=%s, reset_cause_enum_int=%d%s",
		MBOSS_RESPONSE_START_STR,
		system_uptime_ms,
		"UNKNOWN", 99,
		//reset_cause_get_name(reset_cause), (uint8_t)reset_cause,
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen(msg), MODE_BOSS);
}

void boss_cmd_get_unix_timestamp(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement
}


uint8_t check_cmd_password(uint8_t cmd[], uint8_t full_command_with_password[9]) {
	for (uint8_t i = 0; i < MBOSS_COMMAND_LENGTH; i++) {
		if (cmd[i] != full_command_with_password[i]) {
			return 0;
		}
	}
	return 1;
}
