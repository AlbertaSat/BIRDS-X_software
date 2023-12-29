
#include "mboss_handler.h"
#include "common.h"
#include "sys_reboot_reason.h"
#include "experiments.h"
#include "experiment_ccd.h"
#include "terminal.h"
#include "default_settings.h"
#include "frame_handler.h"
#include "dra_system.h"
#include "ax25.h"

#include "drivers/temperature_sensors.h"
#include "drivers/modem.h"
#include "drivers/watchdog.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

// when enabled, echo back the received command immediately upon receiving it
// is not compliant with the MBOSS protocol, but is useful for debugging
const uint8_t debug_enable_echo_command_back = 0;

uint32_t timestamp_sec_at_boot = 0;

uint32_t uptime_at_last_success_failure_led_on = 0; // extern

BossCommandEntry boss_command_table[] = {
	// required commands from ICD v1.5 (random command IDs)
	{0xFF, boss_cmd_turn_off_payload},
	{0xEE, boss_cmd_set_active_aprs_mode},
	{0x0C, boss_cmd_set_active_aprs_mode},
	{0x11, boss_cmd_transfer_aprs_data_packets},

	// 0x20-0x2F namespace: APRS-related and status-related commands
	{0x20, boss_cmd_run_power_on_self_test},
	{0x21, boss_cmd_get_stored_aprs_packets_stats},
	{0x22, boss_cmd_beacon_right_now},
	{0x23, boss_cmd_force_reboot_system},
	{0x24, boss_cmd_set_beacon_period},
	{0x25, boss_cmd_get_uptime_and_status},

	// 0x30-0x3F namespace: experiment-related production commands
	{0x31, boss_cmd_set_unix_timestamp},
	
	// 0xC0-0xCF namespace: RADFET experiment debugging
	// TODO

	// 0xD0-0xDF namespace: CCD experiment debugging
	// TODO

	// 0x90-0x9F namespace: other debugging commands
	{0x90, boss_cmd_echo_command},
	{0x91, boss_cmd_clear_aprs_packet_store},
	{0x92, boss_cmd_exit_mission_boss_mode},
	{0x93, boss_cmd_test_delay_ms},
	{0x94, boss_cmd_exp_get_adc_values},
	{0x95, boss_cmd_exp_get_adc_values_on_loop},

	{0x96, boss_cmd_exp_ccd_do_debug_convert}, // TODO: confirm

	#ifdef ENABLE_boss_cmd_cycle_ccd_pin_options
	{0x97, boss_cmd_cycle_ccd_pin_options},
	#endif

	{0x98, boss_cmd_debug_fetch_raw_temperatures},
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
		set_led_failure();

		char err_msg[255];
		sprintf(
			err_msg,
			"%sERROR: BOSS command failed validation%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)err_msg, strlen(err_msg), MODE_BOSS);
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
		set_led_failure();

		char err_msg[255];
		sprintf(
			err_msg,
			"%sERROR: BOSS command must be %d bytes, but received %d bytes instead%s",
			MBOSS_RESPONSE_START_STR, MBOSS_COMMAND_LENGTH, len, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)err_msg, strlen(err_msg), MODE_BOSS);
		return 0;
	}

	// command should start with 0xE0 (MBOSS_COMMAND_START_BYTE)
	if (cmd[0] != MBOSS_COMMAND_START_BYTE) {
		set_led_failure();

		char err_msg[255];
		sprintf(
			err_msg,
			"%sERROR: BOSS command must start with 0xE0, but received 0x%02X instead%s",
			MBOSS_RESPONSE_START_STR, cmd[0], MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)err_msg, strlen(err_msg), MODE_BOSS);
		return 0;
	}

	// command should end with MBOSS_COMMAND_END_BYTE
	if (cmd[8] != MBOSS_COMMAND_END_BYTE) {
		set_led_failure();

		char err_msg[255];
		sprintf(
			err_msg,
			"%sERROR: BOSS command must end with 0xED, but received 0x%02X instead%s",
			MBOSS_RESPONSE_START_STR, cmd[8], MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)err_msg, strlen(err_msg), MODE_BOSS);
		return 0;
	}

	// command should not contain any 0xE0 or 0xED bytes in the middle
	for (uint8_t i = 1; i < MBOSS_COMMAND_LENGTH-1; i++) {
		set_led_failure();

		if (cmd[i] == MBOSS_COMMAND_START_BYTE || cmd[i] == MBOSS_COMMAND_END_BYTE) {
			char err_msg[255];
			sprintf(
				err_msg,
				"%sERROR: BOSS command cannot contain 0xE0 or 0xED in the middle, but received 0x%02X at index %d%s",
				MBOSS_RESPONSE_START_STR, cmd[i], i, MBOSS_RESPONSE_END_STR
			);
			term_sendToMode((uint8_t*)err_msg, strlen(err_msg), MODE_BOSS);
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
		set_led_failure();

		char err_msg[255];
		sprintf(
			err_msg,
			"%sERROR: BOSS command byte (cmd[1]=0x%02X) is not in command table%s",
			MBOSS_RESPONSE_START_STR, cmd[1], MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)err_msg, strlen(err_msg), MODE_BOSS);
		return 0;
	}

	return 1;
}


void boss_cmd_turn_off_payload(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	// maybe sorta like NVIC_SystemReset(); ?
	// also do shutdown tasks, like storing any info to flash we want, then stall for up to an hour
	set_led_success();

	send_str_to_mboss("RESP: safe to power off");
}

void boss_cmd_set_active_aprs_mode(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	// M = 0: inactive
	// M = 1: digipeating mode
	// M = 2: store-and-forward mode

	uint8_t new_mode = cmd[7];
	if (new_mode > 2) {
		set_led_failure();
		char msg[255];
		sprintf(
			msg,
			"%sERROR: new_mode=%d is invalid, must be 0, 1, or 2%s",
			MBOSS_RESPONSE_START_STR, new_mode, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
		return;
	}

	current_aprs_mode = (RF_APRS_Mode_t)new_mode;

	char dra_error_msg[50];
	dra_error_msg[0] = 0;

	if (new_mode == RF_APRS_MODE_INACTIVE) {
		// turn off the DRA enable pin
		set_dra_awake_mode(0);
		delay_ms(100);

		// disable the vp-digi beacon
		execute_vp_digi_config_cmd((uint8_t*)"beacon 0 off");
		execute_vp_digi_config_cmd((uint8_t*)"digi 0 off");
		execute_vp_digi_config_cmd((uint8_t*)"digi off");

		strcpy(dra_error_msg, "N/A");
	}

	if (new_mode == RF_APRS_MODE_DIGIPEAT || new_mode == RF_APRS_MODE_STORE_AND_FORWARD) {

		// turn on the DRA enable pin
		set_dra_awake_mode(1);

		uint8_t dra_init_error_val = 0;
		for (uint8_t i = 0; i <= 4; i++) {
			// allow up to 4 retries
			delay_ms(600); // datasheet says 300-500ms

			// purge any incoming commands from the previous try
			Loop_process_incoming_uart_commands(0, 1);

			// run the DRA init commands
			dra_init_error_val = send_dra_init_commands();

			sprintf(
				&dra_error_msg[strlen(dra_error_msg)],
				"%s(%d)",
				((dra_init_error_val == 0) ? "OK" : "ERROR"), // error level comment
				dra_init_error_val
			);

			if (dra_init_error_val == 0) {
				break;
			}
		}
		
		// run all the vp-digi config commands (beacons)
		execute_vp_digi_config_cmd((uint8_t*)"beacon 0 on");
		
		// run the vp-digi config command to update the beacon freq
		uint8_t vp_cmd[100];
		sprintf((char*)vp_cmd, "beacon 0 iv %d", current_config_beacon_period_minutes);
		execute_vp_digi_config_cmd(vp_cmd); // like "beacon 0 iv 1"

		execute_vp_digi_config_cmd((uint8_t*)"beacon 0 dl 1"); // delay at boot
		execute_vp_digi_config_cmd((uint8_t*)"beacon 0 path WIDE1-1");
		execute_vp_digi_config_cmd((uint8_t*)"beacon 0 data >Hello from JASPER satellite");

		// run all the vp-digi config commands (digipeater)
		execute_vp_digi_config_cmd((uint8_t*)"digi on");
		execute_vp_digi_config_cmd((uint8_t*)"digi 0 on");
		execute_vp_digi_config_cmd((uint8_t*)"digi 0 alias WIDE");
		execute_vp_digi_config_cmd((uint8_t*)"digi 0 max 2");
		execute_vp_digi_config_cmd((uint8_t*)"digi 0 rep 3");
	}
	
	char msg[100];
	sprintf(
		msg,
		"%sRESP: set new_mode=%d, dra_init_errors=%s%s",
		MBOSS_RESPONSE_START_STR,
		new_mode,
		dra_error_msg,
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
	set_led_success();
}

uint32_t frame_count_returned_since_boot = 0;
void boss_cmd_transfer_aprs_data_packets(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	
	// for debugging, transfer the whole store-and-forward buffer
	/*
	uint8_t msg[STORE_AND_FORWARD_BUFFER_SIZE+100];
	sprintf(
		(char*)msg,
		"%sRESP: store-and-forward buffer: len=%d, val=%s<END>%s",
		MBOSS_RESPONSE_START_STR, sf_buffer_wr_idx, sf_buffer, MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen((char*)msg), MODE_BOSS);
	*/

	uint8_t target_frame_fetch_count = cmd[7]; // number of frames to catch

	if (target_frame_fetch_count == 0) {
		set_led_failure();
		send_str_to_mboss("ERROR: can't fetch 0 frames");
		return;
	}

	uint16_t cur_frame_count = get_stored_frame_count();
	if (cur_frame_count == 0) {
		set_led_failure();
		send_str_to_mboss("ERROR: no frames stored yet");
		return;
	}

	if (target_frame_fetch_count > cur_frame_count) {
		target_frame_fetch_count = cur_frame_count;
	}

	for (uint8_t frame_num = 0; frame_num < target_frame_fetch_count; frame_num++) {
		set_led_success();

		uint8_t frame[FRAMELEN+10];
		memset(frame, 0, FRAMELEN+9);

		uint16_t frame_length = 0;
		uint8_t fetch_result = get_leftmost_stored_frame_and_shift_left(frame, &frame_length);

		// do error check
		if (fetch_result > 0) {
			set_led_failure();

			char err_msg[255];
			sprintf(
				err_msg,
				"%sERROR: failed to fetch frame %d from store-and-forward buffer, error code %d%s",
				MBOSS_RESPONSE_START_STR, frame_num, fetch_result, MBOSS_RESPONSE_END_STR
			);
			term_sendToMode((uint8_t*)err_msg, strlen(err_msg), MODE_BOSS);
			return;
		}

		// just send the frame as-is
		char msg[255];
		sprintf(
			msg,
			"%sRESP: idx=%lu, len=%d, frame=%s%s",
			MBOSS_RESPONSE_START_STR,
			frame_count_returned_since_boot++,
			frame_length, frame, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
	}
}

void boss_cmd_echo_command(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	set_led_success();

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
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
}

void boss_cmd_set_unix_timestamp(uint8_t *cmd, Terminal_stream src) {
	// Example CMD: 0xE0 ____ X X TS_3 TS_2 TS_1 TS_0 0xED
	// FINAL

	uint32_t uptime_sec_at_timestamp_set = get_system_uptime_sec();

	// Initialize variables to store the extracted bytes
	uint8_t byte_TS_3 = cmd[4];
	uint8_t byte_TS_2 = cmd[5];
	uint8_t byte_TS_1 = cmd[6];
	uint8_t byte_TS_0 = cmd[7];

	uint32_t timestamp_sec = (byte_TS_3 << 24) | (byte_TS_2 << 16) | (byte_TS_1 << 8) | byte_TS_0;

	// do check that timestamp is between 2023-01-01 and 2028-01-01
	if (timestamp_sec < 1672531200 || timestamp_sec > 1830301200) {
		set_led_failure();

		char msg[255];
		sprintf(
			msg,
			"%sERROR: timestamp_sec=%lu is invalid, must be between 2023-01-01 and 2028-01-01%s",
			MBOSS_RESPONSE_START_STR, timestamp_sec, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
		return;
	}

	else {
		set_led_success();

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
		term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
		return;
	}
}

void boss_cmd_run_power_on_self_test(uint8_t *cmd, Terminal_stream src) {
	// FINAL

	set_dra_awake_mode(1);
	set_led_success();
	delay_ms(600); // datasheet says 300-500ms

	// TEST 1: check that the DRA is responding to UART commands
	// TODO: turn on the DRA, if it's not already, and then set it back to its previous state (via the PD sleep pin/function)
	send_str_to_dra((uint8_t*)"AT+DMOCONNECT\r\n");
	delay_ms(250); // await response
	Loop_process_incoming_uart_commands(0, 1);

	uint8_t dra_connect_check_passed = (strstr((char *)latest_dra_response_buf, "+DMO") != NULL); // ideally contains "+DMOCONNECT:0", but "+DMOERROR" means it's at least responding

	// char dra_response_if_failed[102];
	// if (dra_connect_check_passed) {
	// 	strcpy(dra_response_if_failed, "PASS");
	// }
	// else if (latest_dra_response_buf[0] == '\0') {
	// 	strcpy(dra_response_if_failed, "<EMPTY>");
	// }
	// else {
	// 	for (uint16_t i = 0; i < 100; i++) {
	// 		dra_response_if_failed[i] = latest_dra_response_buf[i];
	// 	}
	// 	dra_response_if_failed[100] = '\0';
	// }

	char msg[255];
	sprintf(
		msg,
		"%sRESP: is_pwm_mode_on=%s, dra_connect_check=%s, dra_response=%s%s",
		MBOSS_RESPONSE_START_STR,

		// is_pwm_mode_on=XXXX
		(afskCfg.usePWM == 1) ? "PASS" : "FAIL",

		// dra_connect_check=XXXX
		dra_connect_check_passed ? "PASS" : "FAIL",

		// dra_response=XXXX
		latest_dra_response_buf,
		
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);

	// TODO: add more checks
}

void boss_cmd_force_reboot_system(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	uint8_t cmd_password[9] = { 0xE0, 0x23, 0x35, 0xA6, 0x32, 0x18, 0xD3, 0xFF, 0xED };
	
	if (check_cmd_password(cmd, cmd_password)) {
		set_led_success();
		delay_ms(500);

		char msg[255];
		sprintf(
			msg,
			"%sRESP: Rebooting...%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
		
		NVIC_SystemReset();
	}
	else {
		set_led_failure();

		char msg[255];
		sprintf(
			msg,
			"%sERROR: password is incorrect%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
	}
}

void boss_cmd_set_beacon_period(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	char msg[255];
	uint8_t beacon_period_minutes = cmd[7];

	if (beacon_period_minutes == 0) {
		set_led_failure();

		sprintf(
			msg,
			"%sERROR: beacon_period_minutes=%d is invalid, must be >0%s",
			MBOSS_RESPONSE_START_STR, beacon_period_minutes, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
		return;
	}

	current_config_beacon_period_minutes = beacon_period_minutes;
	uint8_t ran_vp_digi_update = 0;

	if (current_aprs_mode == RF_APRS_MODE_DIGIPEAT || current_aprs_mode == RF_APRS_MODE_STORE_AND_FORWARD) {
		// run the vp-digi config command to update the beacon freq
		uint8_t vp_cmd[100];
		sprintf((char*)vp_cmd, "beacon 0 iv %d", beacon_period_minutes);
		execute_vp_digi_config_cmd(vp_cmd);
		ran_vp_digi_update = 1;
	}

	set_led_success();
	sprintf(
		msg,
		"%sRESP: set period_minutes=%d, ran_vp_digi_update=%d%s",
		MBOSS_RESPONSE_START_STR,
		beacon_period_minutes, ran_vp_digi_update,
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
	
}

void boss_cmd_clear_aprs_packet_store(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	clear_frame_store();

	set_led_success();
	
	char msg[255];
	sprintf(
		msg,
		"%sRESP: Cleared APRS packet store memory%s",
		MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
}

void boss_cmd_exit_mission_boss_mode(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	uint8_t cmd_password[9] = { 0xE0, 0x92, 0x38, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xED };
	
	if (check_cmd_password(cmd, cmd_password)) {
		set_led_failure();

		char msg[255];
		sprintf(
			msg,
			"%sRESP: Exiting mission BOSS mode%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
		switchPortToMonitorMode(src);
	}
	else {
		set_led_success();

		char msg[255];
		sprintf(
			msg,
			"%sERROR: password is incorrect%s",
			MBOSS_RESPONSE_START_STR, MBOSS_RESPONSE_END_STR
		);
		term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
	}
}

void boss_cmd_get_uptime_and_status(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	set_led_success();
	uint32_t system_uptime_ms = get_system_uptime_ms();

	reset_cause_t reset_cause = this_boot_reset_cause; // formerly: reset_cause_get();

	char msg[200];
	sprintf(
		msg,
		"%sRESP: ut_ms=%lu, ut_sec=%lu, ts_sec_at_boot=%lu, ts_sec_now=%lu, reset_cause=%s(%d), aprs_mode=%d, temp_k=%d,%d,%d,%d,%d%s",
		MBOSS_RESPONSE_START_STR,
		system_uptime_ms, get_system_uptime_sec(),
		timestamp_sec_at_boot, get_unix_timestamp_sec_now(),
		reset_cause_get_name(reset_cause), (uint8_t)reset_cause,
		(uint8_t) current_aprs_mode,
		get_external_temperature_k(0), get_external_temperature_k(1), get_external_temperature_k(2), get_external_temperature_k(3), get_external_temperature_k(4),
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
}

void boss_cmd_get_stored_aprs_packets_stats(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	set_led_success();

	char msg[180];
	sprintf(
		msg,
		"%sRESP: aprs_packets_stored=%d, aprs_packets_stored_bytes=%d, aprs_packets_total_bytes=%d, frames_rxd_since_boot=%lu, beacons_since_boot=%lu%s",
		MBOSS_RESPONSE_START_STR,
		get_stored_frame_count(), sf_buffer_wr_idx, STORE_AND_FORWARD_BUFFER_SIZE, frame_rx_count_since_boot, beacon_count_since_boot,
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
}

void boss_cmd_beacon_right_now(uint8_t *cmd, Terminal_stream src) {
	// FINAL

	if (current_aprs_mode == RF_APRS_MODE_INACTIVE) {
		set_led_failure();
		send_str_to_mboss("ERROR: can't beacon when APRS mode is inactive");
	}
	else if (current_aprs_mode == RF_APRS_MODE_DIGIPEAT || current_aprs_mode == RF_APRS_MODE_STORE_AND_FORWARD) {
		set_led_success();
		execute_vp_digi_monitor_cmd((uint8_t*)"beacon 0");
		send_str_to_mboss("RESP: beacon 0 sent");
	}
	
}


void boss_cmd_exp_get_adc_values(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	set_led_success();

	uint32_t unix_time = get_unix_timestamp_sec_now();

	uint16_t adc_val_radfet_1, adc_val_radfet_2, adc_val_radfet_3, adc_val_radfet_4;
	do_radfet_measurements(&adc_val_radfet_1, &adc_val_radfet_2, &adc_val_radfet_3, &adc_val_radfet_4);

	// prep message
	char msg[100];
	sprintf(
		msg,
		"%sRESP: %lu,%d,%d,%d,%d%s",
		MBOSS_RESPONSE_START_STR,
		unix_time,
		adc_val_radfet_1,
		adc_val_radfet_2,
		adc_val_radfet_3,
		adc_val_radfet_4,
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
}

void boss_cmd_exp_get_adc_values_on_loop(uint8_t *cmd, Terminal_stream src) {
	// FINAL
	set_led_success();

	while (1) {
		boss_cmd_exp_get_adc_values(cmd, src);

		// service watchdog
		Wdog_reset();

		// delay
		delay_ms(100);
	}
}

void boss_cmd_exp_ccd_do_debug_convert(uint8_t *cmd, Terminal_stream src) {
	// FINAL

	uint8_t ccd_num = cmd[7];
	uint8_t resolution_number_of_groups_of_16 = cmd[6];

	if (resolution_number_of_groups_of_16 == 0) {
		resolution_number_of_groups_of_16 = 1;
	}

	uint16_t elems_per_group = CCD_DATA_LEN_BYTES / (resolution_number_of_groups_of_16 * 16);

	if (elems_per_group < 1) {
		elems_per_group = 1;
	}

	if (ccd_num != 1 && ccd_num != 2) {
		set_led_failure();
		send_str_to_mboss_no_tail("ERROR: ccd_num must be 1 or 2");
		delay_ms(50);
		return;
	}
	
	set_led_success();
	fetch_ccd_measurement_and_log_it(ccd_num, elems_per_group);
}

void boss_cmd_test_delay_ms(uint8_t *cmd, Terminal_stream src) {
	// FINAL

	set_led_success();

	// fetch delay time from command (in ms)
	uint32_t delay_duration_ms = cmd[6] << 8 | cmd[7];
	
	// prep start message
	char msg_start[100];
	sprintf(
		msg_start,
		"%sRESP: delay_duration_ms=%lu, starting delay...",
		MBOSS_RESPONSE_START_STR, delay_duration_ms
	);

	// prep end message
	char msg_end[100];
	sprintf(
		msg_end,
		"complete%s",
		MBOSS_RESPONSE_END_STR
	);

	term_sendToMode((uint8_t*)msg_start, strlen(msg_start), MODE_BOSS);
	if (delay_ms > 0) delay_ms(delay_duration_ms);
	term_sendToMode((uint8_t*)msg_end, strlen(msg_end), MODE_BOSS);
}

void boss_cmd_debug_fetch_raw_temperatures(uint8_t *cmd, Terminal_stream src) {
	// FINAL

	set_led_success();

	// start read
	uint8_t read_data[2] = {0, 0};
	get_external_temperature_k_raw(1, read_data); // sensor 1 is fine

	// get actual temperature
	uint16_t actual_temp_k = get_external_temperature_k(1);

	char msg[100];
	sprintf(
		msg,
		"%sRESP: raw bytes: 0x%02X 0x%02X, temp_k=%d%s",
		MBOSS_RESPONSE_START_STR,
		read_data[0],
		read_data[1],
		actual_temp_k,
		MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
}

#ifdef ENABLE_boss_cmd_cycle_ccd_pin_options
void boss_cmd_cycle_ccd_pin_options(uint8_t *cmd, Terminal_stream src) {
	// FINAL

	for (uint8_t sh = 0; sh <= 1; sh++) {
		for (uint8_t icg = 0; icg <= 1; icg++) {
			for (uint8_t phi_m = 0; phi_m <= 1; phi_m++) {
				write_ccd_pins(phi_m, icg, sh);
				
				char msg[100];
				sprintf(
					msg,
					"%sRESP: sh=%d, icg=%d, phi_m=%d%s",
					MBOSS_RESPONSE_START_STR,
					sh,
					icg,
					phi_m,
					MBOSS_RESPONSE_END_STR
				);
				term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);

				delay_ms(4000);
				Wdog_reset();

				set_led_success();
				delay_ms(100);
				HAL_GPIO_WritePin(PIN_LED_SUCCESS_GPIO_Port, PIN_LED_SUCCESS_Pin, GPIO_PIN_RESET);
			}
		}
	}
}
#endif

uint8_t check_cmd_password(uint8_t cmd[], uint8_t full_command_with_password[9]) {
	for (uint8_t i = 0; i < MBOSS_COMMAND_LENGTH; i++) {
		if (cmd[i] != full_command_with_password[i]) {
			return 0;
		}
	}
	return 1;
}

void send_str_to_mboss_no_tail(char input_msg[]) {
	term_sendToMode((uint8_t*)input_msg, strlen(input_msg), MODE_BOSS);
}

void send_str_to_mboss(char input_msg[]) {
	char msg[255];
	sprintf(
		msg,
		"%s%s%s",
		MBOSS_RESPONSE_START_STR, input_msg, MBOSS_RESPONSE_END_STR
	);
	term_sendToMode((uint8_t*)msg, strlen(msg), MODE_BOSS);
}

RF_APRS_Mode_t get_current_aprs_mode(void) {
	return current_aprs_mode;
}

uint32_t get_unix_timestamp_sec_now() {
	return timestamp_sec_at_boot + get_system_uptime_sec();
}

void set_led_success() {
	HAL_GPIO_WritePin(PIN_LED_SUCCESS_GPIO_Port, PIN_LED_SUCCESS_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(PIN_LED_FAILURE_GPIO_Port, PIN_LED_FAILURE_Pin, GPIO_PIN_RESET);
	uptime_at_last_success_failure_led_on = get_system_uptime_ms();
}

void set_led_failure() {
	HAL_GPIO_WritePin(PIN_LED_FAILURE_GPIO_Port, PIN_LED_FAILURE_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(PIN_LED_SUCCESS_GPIO_Port, PIN_LED_SUCCESS_Pin, GPIO_PIN_RESET);
	uptime_at_last_success_failure_led_on = get_system_uptime_ms();
}
