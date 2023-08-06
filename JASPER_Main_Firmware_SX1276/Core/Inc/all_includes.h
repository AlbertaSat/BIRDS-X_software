// all_includes.h

#pragma once

// normal ones that just always get included obviously
#include <string.h> // stuff like strlen()
#include <stdio.h> // stuff like sprintf()

// main STM32
#include "main.h"
#include "usart.h"
#include "spi.h"

// all the ones for this project
#include "aprs_controller.h"
#include "command_controller.h"
#include "experiment_controller.h"
#include "main_loop_controller.h"
#include "pin_interface_defs.h"
#include "SX1278.h"
#include "rf_controller.h"
