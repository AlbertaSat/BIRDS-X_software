
#include "mboss_handler.h"
#include "common.h"
#include "sys_reboot_reason.h"
#include "experiments.h"
#include "terminal.h"
#include "default_settings.h"
#include "frame_handler.h"
#include "dra_system.h"
#include "ax25.h"

#include "drivers/temperature_sensors.h"
#include "drivers/modem.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

// when enabled, echo back the received command immediately upon receiving it
// is not compliant with the MBOSS protocol, but is useful for debugging
const uint8_t debug_enable_echo_command_back = 0;

uint32_t timestamp_sec_at_boot = 0;

BossCommandEntry boss_command_table[] = {
	{0x00, boss_cmd_turn_off_payload},
	{0x01, boss_cmd_set_active_aprs_mode},
	{0x02, boss_cmd_transfer_aprs_data_packets},
	{0x03, boss_cmd_send_temperature},
	{0x04, boss_cmd_enable_pin_diode_experiment},
	{0x05, boss_cmd_disable_pin_diode_experiment},
	{0x06, boss_cmd_enable_radfet_experiment},
	{0x07, boss_cmd_disable_radfet_experiment},
	{0x08, boss_cmd_enable_both_experiments},
	{0x09, boss_cmd_disable_both_experiments},
	{0x10, boss_cmd_echo_command},
	{0x11, boss_cmd_transfer_n_raw_experiment_packets},
	{0x12, boss_cmd_get_experiment_polling_times},
	{0x13, boss_cmd_set_pin_diode_polling_time},
	{0x14, boss_cmd_set_radfet_polling_time},
	{0x15, boss_cmd_set_both_polling_time},
	{0x16, boss_cmd_set_unix_timestamp},
	{0x17, boss_cmd_set_unix_timestamp_shutdown},
	{0x18, boss_cmd_run_power_on_self_test},
	{0x19, boss_cmd_force_reboot_system},
	{0x20, boss_cmd_set_beacon_period},
	{0x21, boss_cmd_clear_aprs_packet_store},
	{0x22, boss_cmd_exit_mission_boss_mode},
	{0x23, boss_cmd_get_sys_uptime_and_reboot_reason},
	{0x24, boss_cmd_get_unix_timestamp},
	{0x25, boss_cmd_set_experiment_stat_calc_period},
	{0x26, boss_cmd_get_experiment_stat_calc_period},
	{0x27, boss_cmd_transfer_n_statistical_experiment_measurements},
	{0x28, boss_cmd_get_stored_aprs_packets_stats},
};

RF_APRS_Mode_t current_aprs_mode = RF_APRS_MODE_INACTIVE;
uint8_t current_config_beacon_period_minutes = DEFAULT_BEACON_PERIOD_MINUTES;

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

	if (debug_enable_echo_command_back) {
		debug_echo_back_mboss_command(cmd, len, src);
	}

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
	// maybe sorta like NVIC_SystemReset(); ?
	// also do shutdown tasks, like storing any info to flash we want, then stall for up to an hour

	send_str_to_mboss("RESP: safe to power off");
}

void boss_cmd_set_active_aprs_mode(uint8_t *cmd, Terminal_stream src) {
	// M = 0: inactive
	// M = 1: digipeating mode
	// M = 2: store-and-forward mode

	uint8_t new_mode = cmd[7];
	if (new_mode > 2) {
		char msg[255];
		sprintf(
			msg,
			"%sERROR: new_mode=%d is invalid, must be 0, 1, or 2%s",
			MBOSS_RESPONSE_START_STR, new_mode, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);
		return;
	}

	current_aprs_mode = (RF_APRS_Mode_t)new_mode;

	char msg[255];
	sprintf(
		msg,
		"%sRESP: set new_mode=%d%s",
		MBOSS_RESPONSE_START_STR, new_mode, MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen(msg), MODE_BOSS);

	if (new_mode == RF_APRS_MODE_INACTIVE) {
		// turn off the DRA enable pin
		set_dra_awake_mode(0);

		// disable the vp-digi beacon
		execute_vp_digi_config_cmd("beacon 0 off");
		execute_vp_digi_config_cmd("digi 0 off");
		execute_vp_digi_config_cmd("digi off");


	}

	if (new_mode == RF_APRS_MODE_DIGIPEAT || new_mode == RF_APRS_MODE_STORE_AND_FORWARD) {
		// turn on the DRA enable pin
		set_dra_awake_mode(1);
		
		// run the DRA init commands
		send_dra_init_commands();

		// run all the vp-digi config commands (beacons)
		execute_vp_digi_config_cmd("beacon 0 on");
		
		// run the vp-digi config command to update the beacon freq
		uint8_t vp_cmd[100];
		sprintf((char*)vp_cmd, "beacon 0 iv %d", current_config_beacon_period_minutes);
		execute_vp_digi_config_cmd((char*)vp_cmd); // like "beacon 0 iv 1"

		execute_vp_digi_config_cmd("beacon 0 dl 1"); // delay at boot
		execute_vp_digi_config_cmd("beacon 0 path WIDE1-1");
		execute_vp_digi_config_cmd("beacon 0 data >Hello from JASPER satellite");

		// run all the vp-digi config commands (digipeater)
		execute_vp_digi_config_cmd("digi on");
		execute_vp_digi_config_cmd("digi 0 on");
		execute_vp_digi_config_cmd("digi 0 alias WIDE");
		execute_vp_digi_config_cmd("digi 0 max 2");
		execute_vp_digi_config_cmd("digi 0 rep 3");

	}

}

void boss_cmd_transfer_aprs_data_packets(uint8_t *cmd, Terminal_stream src) {
	// FIXME: implement critical function
	
	// for debugging, transfer the whole store-and-forward buffer
	/*
	uint8_t msg[STORE_AND_FORWARD_BUFFER_SIZE+100];
	sprintf(
		(char*)msg,
		"%sRESP: store-and-forward buffer: len=%d, val=%s<END>%s",
		MBOSS_RESPONSE_START_STR, sf_buffer_wr_idx, sf_buffer, MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen((char*)msg), MODE_BOSS);
	*/

	uint8_t target_frame_fetch_count = cmd[7]; // number of frames to catch

	if (target_frame_fetch_count == 0) {
		send_str_to_mboss("ERROR: can't fetch 0 frames");
		return;
	}

	uint16_t cur_frame_count = get_stored_frame_count();
	if (cur_frame_count == 0) {
		send_str_to_mboss("ERROR: no frames stored yet");
		return;
	}

	if (target_frame_fetch_count > cur_frame_count) {
		target_frame_fetch_count = cur_frame_count;
	}

	for (uint8_t frame_num = 0; frame_num < target_frame_fetch_count; frame_num++) {
		uint8_t frame[FRAMELEN+10];
		memset(frame, 0, FRAMELEN+9);

		uint16_t frame_length = 0;
		uint8_t fetch_result = get_leftmost_stored_frame_and_shift_left(frame, &frame_length);

		// do error check
		if (fetch_result > 0) {
			char err_msg[255];
			sprintf(
				err_msg,
				"%sERROR: failed to fetch frame %d from store-and-forward buffer, error code %d%s",
				MBOSS_RESPONSE_START_STR, frame_num, fetch_result, MBOSS_RESPONSE_END_STR
			);
			term_sendToMode(err_msg, strlen(err_msg), MODE_BOSS);
			return;
		}

		// just send the frame as-is
		char msg[255];
		sprintf(
			msg,
			"%sRESP: len=%d, frame=%s%s",
			MBOSS_RESPONSE_START_STR, frame_length, frame, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);
	}
	
}

void boss_cmd_send_temperature(uint8_t *cmd, Terminal_stream src) {
	uint16_t internal_temp_k = get_internal_temperature_k();
	
	// NOTE: this function is blocking and takes about 1.5s to run
	// TODO: make the get_external_temperature_k() part run asynchronously so that all the sensors can be fetched in parallel

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
	config_enable_pin_experiment = 1;
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_disable_pin_diode_experiment(uint8_t *cmd, Terminal_stream src) {
	config_enable_pin_experiment = 0;
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_enable_radfet_experiment(uint8_t *cmd, Terminal_stream src) {
	config_enable_radfet_experiment = 1;
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_disable_radfet_experiment(uint8_t *cmd, Terminal_stream src) {
	config_enable_radfet_experiment = 0;
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_enable_both_experiments(uint8_t *cmd, Terminal_stream src) {
	config_enable_pin_experiment = 1;
	config_enable_radfet_experiment = 1;
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_disable_both_experiments(uint8_t *cmd, Terminal_stream src) {
	config_enable_pin_experiment = 0;
	config_enable_radfet_experiment = 0;
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
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

void boss_cmd_transfer_n_raw_experiment_packets(uint8_t *cmd, Terminal_stream src) {
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_get_experiment_polling_times(uint8_t *cmd, Terminal_stream src) {
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_set_pin_diode_polling_time(uint8_t *cmd, Terminal_stream src) {
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_set_radfet_polling_time(uint8_t *cmd, Terminal_stream src) {
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_set_both_polling_time(uint8_t *cmd, Terminal_stream src) {
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_set_unix_timestamp(uint8_t *cmd, Terminal_stream src) {
	// Example CMD: 0xE0 0x16 X X TS_3 TS_2 TS_1 TS_0 0xED

	uint32_t uptime_sec_at_timestamp_set = get_system_uptime_sec();

	// Initialize variables to store the extracted bytes
	uint8_t byte_TS_3 = cmd[4];
	uint8_t byte_TS_2 = cmd[5];
	uint8_t byte_TS_1 = cmd[6];
	uint8_t byte_TS_0 = cmd[7];

	uint32_t timestamp_sec = (byte_TS_3 << 24) | (byte_TS_2 << 16) | (byte_TS_1 << 8) | byte_TS_0;

	// do check that timestamp is between 2023-01-01 and 2028-01-01
	if (timestamp_sec < 1672531200 || timestamp_sec > 1830301200) {
		char msg[255];
		sprintf(
			msg,
			"%sERROR: timestamp_sec=%lu is invalid, must be between 2023-01-01 and 2028-01-01%s",
			MBOSS_RESPONSE_START_STR, timestamp_sec, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);
		return;
	}

	else {
		// set into extern
		timestamp_sec_at_boot = timestamp_sec - uptime_sec_at_timestamp_set;

		// print success message
		char msg[255];
		sprintf(
			msg,
			"%sRESP: timestamp_sec=%lu, uptime_sec_at_timestamp_set=%lu, timestamp_sec_at_boot=%lu%s",
			MBOSS_RESPONSE_START_STR,
			
			timestamp_sec, uptime_sec_at_timestamp_set, timestamp_sec_at_boot,
			
			MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);
		return;
	}
}

void boss_cmd_set_unix_timestamp_shutdown(uint8_t *cmd, Terminal_stream src) {
	// Example CMD: 0xE0 0x17 X X TS_3 TS_2 TS_1 TS_0 0xED

	// Initialize variables to store the extracted bytes
	uint8_t byte_TS_3 = cmd[4];
	uint8_t byte_TS_2 = cmd[5];
	uint8_t byte_TS_1 = cmd[6];
	uint8_t byte_TS_0 = cmd[7];

	uint32_t timestamp_sec = (byte_TS_3 << 24) | (byte_TS_2 << 16) | (byte_TS_1 << 8) | byte_TS_0;

	// TODO: store timestamp as an extern, if it needs to be used anywhere
	send_str_to_mboss("RESP: shutdown timestamp set");
}

void boss_cmd_run_power_on_self_test(uint8_t *cmd, Terminal_stream src) {
	// POST = power on self test

	// TEST 1: check that the DRA is responding to UART commands
	// TODO: turn on the DRA, if it's not already, and then set it back to its previous state (via the PD sleep pin/function)
	send_str_to_dra("AT+DMOCONNECT\r\n");
	delay_ms(800); // await response

	uint8_t dra_connect_check_passed = (strstr(latest_dra_response_buf, "+DMO") != NULL); // ideally contains "+DMOCONNECT:0", but "+DMOERROR" means it's at least responding

	char msg[255];
	sprintf(
		msg,
		"%sRESP: dra_connect_check=%s, is_pwm_mode_on=%s%s",
		MBOSS_RESPONSE_START_STR,

		// dra_connect_check=XXXX
		dra_connect_check_passed ? "PASS" : "FAIL",
		// latest_dra_response_buf, // dra_response=XXXX (for debugging)

		// is_pwm_mode_on=XXXX
		(afskCfg.usePWM == 1) ? "PASS" : "FAIL",
		
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen(msg), MODE_BOSS);

	// TODO: add more checks
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
		
		NVIC_SystemReset();
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
	char msg[255]; 
	uint8_t beacon_period_minutes = cmd[7];

	if (beacon_period_minutes == 0) {
		sprintf(
			msg,
			"%sERROR: beacon_period_minutes=%d is invalid, must be >0%s",
			MBOSS_RESPONSE_START_STR, beacon_period_minutes, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode(msg, strlen(msg), MODE_BOSS);
		return;
	}

	current_config_beacon_period_minutes = beacon_period_minutes;
	uint8_t ran_vp_digi_update = 0;

	if (current_aprs_mode == RF_APRS_MODE_DIGIPEAT || current_aprs_mode == RF_APRS_MODE_STORE_AND_FORWARD) {
		// run the vp-digi config command to update the beacon freq
		uint8_t vp_cmd[100];
		sprintf((char*)vp_cmd, "beacon 0 iv %d", beacon_period_minutes);
		execute_vp_digi_config_cmd((char*)vp_cmd);
		ran_vp_digi_update = 1;
	}

	sprintf(
		msg,
		"%sRESP: set period_minutes=%d, ran_vp_digi_update=%d%s",
		MBOSS_RESPONSE_START_STR,
		beacon_period_minutes, ran_vp_digi_update,
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen(msg), MODE_BOSS);
	
}

void boss_cmd_clear_aprs_packet_store(uint8_t *cmd, Terminal_stream src) {
	
	clear_frame_store();
	
	char msg[255];
	sprintf(
		msg,
		"%sRESP: Cleared APRS packet store memory%s",
		MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen(msg), MODE_BOSS);
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

	reset_cause_t reset_cause = this_boot_reset_cause; // formerly: reset_cause_get();
	
	char msg[200];
	sprintf(
		msg,
		"%sRESP: uptime_ms=%lu, uptime_sec=%lu, timestamp_sec_at_boot=%lu, timestamp_sec_now=%lu, reset_cause_str=%s, reset_cause_enum_int=%d%s",
		MBOSS_RESPONSE_START_STR,
		system_uptime_ms, get_system_uptime_sec(),
		timestamp_sec_at_boot, get_unix_timestamp_sec_now(),
		reset_cause_get_name(reset_cause), (uint8_t)reset_cause,
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen(msg), MODE_BOSS);
}

void boss_cmd_get_unix_timestamp(uint8_t *cmd, Terminal_stream src) {
	// alias for get_sys_uptime_and_reboot_reason
	boss_cmd_get_sys_uptime_and_reboot_reason(cmd, src);
}


void boss_cmd_set_experiment_stat_calc_period(uint8_t *cmd, Terminal_stream src) {
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}
void boss_cmd_get_experiment_stat_calc_period(uint8_t *cmd, Terminal_stream src) {
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}
void boss_cmd_transfer_n_statistical_experiment_measurements(uint8_t *cmd, Terminal_stream src) {
	send_str_to_mboss("RESP: experiment functionality not implemented"); // TODO: implement experiment functions
}

void boss_cmd_get_stored_aprs_packets_stats(uint8_t *cmd, Terminal_stream src) {
	char msg[255];
	sprintf(
		msg,
		"%sRESP: aprs_packets_stored=%d, aprs_packets_stored_bytes_occupied=%d, aprs_packets_total_bytes=%d, frame_rx_count_since_boot=%lu, beacon_count_since_boot=%lu%s",
		MBOSS_RESPONSE_START_STR,
		get_stored_frame_count(), sf_buffer_wr_idx, STORE_AND_FORWARD_BUFFER_SIZE, frame_rx_count_since_boot, beacon_count_since_boot,
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen(msg), MODE_BOSS);
}

uint8_t check_cmd_password(uint8_t cmd[], uint8_t full_command_with_password[9]) {
	for (uint8_t i = 0; i < MBOSS_COMMAND_LENGTH; i++) {
		if (cmd[i] != full_command_with_password[i]) {
			return 0;
		}
	}
	return 1;
}

void send_str_to_mboss(char input_msg[]) {
	char msg[255];
	sprintf(
		msg,
		"%s%s%s",
		MBOSS_RESPONSE_START_STR, input_msg, MBOSS_RESPONSE_END_STR
	);
	term_sendToMode(msg, strlen(msg), MODE_BOSS);
}

RF_APRS_Mode_t get_current_aprs_mode(void) {
	return current_aprs_mode;
}

uint32_t get_unix_timestamp_sec_now() {
	return timestamp_sec_at_boot + get_system_uptime_sec();
}
