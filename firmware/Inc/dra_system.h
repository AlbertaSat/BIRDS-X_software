
// include guard
#ifndef __DRA_SYSTEM_H__
#define __DRA_SYSTEM_H__

#include "terminal.h"

#define DRA_RESPONSE_BUF_SIZE 100

extern uint8_t latest_dra_response_buf[DRA_RESPONSE_BUF_SIZE];

void receive_incoming_dra_message(uint8_t *msg, uint16_t len);

void send_str_to_dra(uint8_t *str);

// new_state=0 means sleep, new_state=1 means turn on
void set_dra_awake_mode(uint8_t new_state);

// returns 0 if no error, >0 if error
uint8_t send_dra_init_commands();



// end include guard
#endif    // __DRA_SYSTEM_H__
