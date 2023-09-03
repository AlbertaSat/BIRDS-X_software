
// include guard
#ifndef __DRA_SYSTEM_H__
#define __DRA_SYSTEM_H__

#include "terminal.h"

void receive_incoming_dra_message(uint8_t *cmd, uint16_t len, Terminal_stream src);


// end include guard
#endif    // __DRA_SYSTEM_H__
