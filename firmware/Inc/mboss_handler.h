#pragma once

// include guard
#ifndef __MBOSS_HANDLER_H__
#define __MBOSS_HANDLER_H__

#include "terminal.h"

#define MBOSS_COMMAND_LENGTH 9
#define MBOSS_COMMAND_START_BYTE 0xE0
#define MBOSS_COMMAND_END_BYTE 0xED
#define MBOSS_RESPONSE_START_STR "\xDA\xBE"
#define MBOSS_RESPONSE_END_STR "\xDA\xED"

typedef struct {
    uint8_t cmd_byte;
    void (*cmd_func)(uint8_t*, Terminal_stream);
} BossCommandEntry;


typedef enum {
    RF_APRS_MODE_INACTIVE = 0,
    RF_APRS_MODE_DIGIPEAT = 1,
    RF_APRS_MODE_STORE_AND_FORWARD = 2,
} RF_APRS_Mode_t;

void receive_incoming_boss_cmd(uint8_t *cmd, uint16_t len, Terminal_stream src);

// returns 1 if the command is good, 0 if bad
uint8_t validate_incoming_boss_cmd(uint8_t *cmd, uint16_t len, Terminal_stream src);


void boss_cmd_turn_off_payload(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_active_aprs_mode(uint8_t *cmd, Terminal_stream src);
void boss_cmd_transfer_aprs_data_packets(uint8_t *cmd, Terminal_stream src);
void boss_cmd_send_temperature(uint8_t *cmd, Terminal_stream src);
void boss_cmd_enable_pin_diode_experiment(uint8_t *cmd, Terminal_stream src);
void boss_cmd_disable_pin_diode_experiment(uint8_t *cmd, Terminal_stream src);
void boss_cmd_enable_radfet_experiment(uint8_t *cmd, Terminal_stream src);
void boss_cmd_disable_radfet_experiment(uint8_t *cmd, Terminal_stream src);
void boss_cmd_enable_both_experiments(uint8_t *cmd, Terminal_stream src);
void boss_cmd_disable_both_experiments(uint8_t *cmd, Terminal_stream src);
void boss_cmd_echo_command(uint8_t *cmd, Terminal_stream src);
void boss_cmd_transfer_n_raw_experiment_packets(uint8_t *cmd, Terminal_stream src);
void boss_cmd_get_experiment_polling_times(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_pin_diode_polling_time(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_radfet_polling_time(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_both_polling_time(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_unix_timestamp(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_unix_timestamp_shutdown(uint8_t *cmd, Terminal_stream src);
void boss_cmd_run_power_on_self_test(uint8_t *cmd, Terminal_stream src);
void boss_cmd_force_reboot_system(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_beacon_period(uint8_t *cmd, Terminal_stream src);
void boss_cmd_clear_flash_memory(uint8_t *cmd, Terminal_stream src);
void boss_cmd_exit_mission_boss_mode(uint8_t *cmd, Terminal_stream src);
void boss_cmd_get_sys_uptime_and_reboot_reason(uint8_t *cmd, Terminal_stream src);
void boss_cmd_get_unix_timestamp(uint8_t *cmd, Terminal_stream src);
void boss_cmd_set_experiment_stat_calc_period(uint8_t *cmd, Terminal_stream src);
void boss_cmd_get_experiment_stat_calc_period(uint8_t *cmd, Terminal_stream src);
void boss_cmd_transfer_n_statistical_experiment_measurements(uint8_t *cmd, Terminal_stream src);

// returns 1/true if 9 bytes of cmd and full_command_with_password match, 0/false if they don't match
uint8_t check_cmd_password(uint8_t cmd[], uint8_t full_command_with_password[9]);

// send str to mission boss (do not put start/end delims)
void send_str_to_mboss(char input_msg[]);

RF_APRS_Mode_t get_current_aprs_mode(void);

// end include guard
#endif    // __MBOSS_HANDLER_H__
