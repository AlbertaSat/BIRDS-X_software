#pragma once

// include guard
#ifndef __MBOSS_HANDLER_H__
#define __MBOSS_HANDLER_H__

#include "terminal.h"

void receive_incoming_boss_cmd(uint8_t *cmd, uint16_t len, Terminal_stream src);



// end include guard
#endif    // __MBOSS_HANDLER_H__
