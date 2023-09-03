#pragma once

// include guard
#ifndef __MBOSS_HANDLER_H__
#define __MBOSS_HANDLER_H__

#include "terminal.h"

#define MBOSS_COMMAND_LENGTH 9
#define MBOSS_COMMAND_START_BYTE 0xE0
#define MBOSS_COMMAND_END_BYTE 0xED
#define MBOSS_RESPONSE_START_STR "\xDA\xBE"
#define MBOSS_RESPONSE_END_STR "\xDA\xED\n" // FIXME: remove newline here

typedef struct {
    uint8_t cmd_byte;
    void (*cmd_func)(uint8_t*, Terminal_stream);
} BossCommandEntry;

void receive_incoming_boss_cmd(uint8_t *cmd, uint16_t len, Terminal_stream src);

// returns 1 if the command is good, 0 if bad
uint8_t validate_incoming_boss_cmd(uint8_t *cmd, uint16_t len, Terminal_stream src);

void boss_cmd_turn_off_payload(uint8_t *cmd, Terminal_stream src);
void boss_cmd_turn_on_digipeat_mode(uint8_t *cmd, Terminal_stream src);
void boss_cmd_turn_off_sf_mode(uint8_t *cmd, Terminal_stream src);

void boss_cmd_echo_command(uint8_t *cmd, Terminal_stream src);

// end include guard
#endif    // __MBOSS_HANDLER_H__
