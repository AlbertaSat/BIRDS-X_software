#pragma once

#include <RadioLib.h>


void init_sx1276();

void do_test_sx1276();

void readBitFromRadio(void);

/*

// in BuildOptUser.h, add the following for debugging:

#define RADIOLIB_DEBUG
#define RADIOLIB_VERBOSE

#include "../../../../../include/command_uart_controller.h"
#define RADIOLIB_DEBUG_PORT BossSerial

*/
