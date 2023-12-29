#pragma once

// include guard
#ifndef __MBOSS_HANDLER_H__
#define __MBOSS_HANDLER_H__

#include "terminal.h"

// #include <stdint.h>

#define LED_SUCCESS_FAILURE_MIN_TIME_ON_MS 800
extern uint32_t uptime_at_last_success_failure_led_on;

typedef struct {
	uint8_t cmd_byte;
	void (*cmd_func)(uint8_t*, Terminal_stream);
} BossCommandEntry;


typedef enum {
	RF_APRS_MODE_INACTIVE = 0,
	RF_APRS_MODE_DIGIPEAT = 1,
	RF_APRS_MODE_STORE_AND_FORWARD = 2,
} RF_APRS_Mode_t;


// externs
extern uint32_t timestamp_sec_at_boot;
extern RF_APRS_Mode_t current_aprs_mode;

#define MBOSS_COMMAND_LENGTH 9
#define MBOSS_COMMAND_START_BYTE 0xE0
#define MBOSS_COMMAND_END_BYTE 0xED
#define MBOSS_RESPONSE_START_STR "\xDA\xBE"
#define MBOSS_RESPONSE_END_STR "\xDA\xED"

void receive_incoming_boss_cmd(uint8_t *cmd, uint16_t len, Terminal_stream src);

// returns 1 if the command is good, 0 if bad
uint8_t validate_incoming_boss_cmd(uint8_t *cmd, uint16_t len, Terminal_stream src);


// have to disable a few commands to save space
// #define ENABLE_boss_cmd_cycle_ccd_pin_options

void boss_cmd_turn_off_payload(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_active_aprs_mode(uint8_t *cmd, Terminal_stream src);
void boss_cmd_transfer_data_packets(uint8_t *cmd, Terminal_stream src);
void boss_cmd_echo_command(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_unix_timestamp(uint8_t *cmd, Terminal_stream src);
void boss_cmd_run_power_on_self_test(uint8_t *cmd, Terminal_stream src);
void boss_cmd_force_reboot_system(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_beacon_period(uint8_t *cmd, Terminal_stream src);
void boss_cmd_clear_aprs_packet_store(uint8_t *cmd, Terminal_stream src);
void boss_cmd_exit_mission_boss_mode(uint8_t *cmd, Terminal_stream src);
void boss_cmd_get_uptime_and_status(uint8_t *cmd, Terminal_stream src);
void boss_cmd_get_stored_aprs_packets_stats(uint8_t *cmd, Terminal_stream src);
void boss_cmd_beacon_right_now(uint8_t *cmd, Terminal_stream src);
void boss_cmd_exp_get_radfet_values(uint8_t *cmd, Terminal_stream src);
void boss_cmd_exp_get_radfet_values_on_loop(uint8_t *cmd, Terminal_stream src);
void boss_cmd_exp_ccd_do_debug_convert(uint8_t *cmd, Terminal_stream src);
void boss_cmd_test_delay_ms(uint8_t *cmd, Terminal_stream src);

#ifdef ENABLE_boss_cmd_cycle_ccd_pin_options
void boss_cmd_cycle_ccd_pin_options(uint8_t *cmd, Terminal_stream src);
#endif

void boss_cmd_debug_fetch_raw_temperatures(uint8_t *cmd, Terminal_stream src);
void boss_cmd_exp_set_ccd_config(uint8_t *cmd, Terminal_stream src);

// returns 1/true if 9 bytes of cmd and full_command_with_password match, 0/false if they don't match
uint8_t check_cmd_password(uint8_t cmd[], uint8_t full_command_with_password[9]);

// send str to mission boss (includes start/end delims by default)
void send_str_to_mboss(char input_msg[]);
void send_str_to_mboss_no_tail(char input_msg[]);

RF_APRS_Mode_t get_current_aprs_mode(void);

uint32_t get_unix_timestamp_sec_now();

void set_led_success();
void set_led_failure();

// end include guard
#endif    // __MBOSS_HANDLER_H__
