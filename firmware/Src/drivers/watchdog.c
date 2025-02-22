/*
This file is part of VP-Digi.

VP-Digi is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

VP-Digi is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with VP-Digi.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "drivers/watchdog.h"
#include "stm32f1xx.h"

void Wdog_init(void)
{
	IWDG->KR = 0x5555; //configuration mode
	IWDG->PR = 0b101; //prescaler
	IWDG->RLR = 0xFFF; //timeout register
	IWDG->KR = 0xCCCC; //start

	// Timeout Period:
	// To calculate the timeout period:
	// Timeout = (RLR + 1) / (Clock source frequency / Prescaler)
	// Timeout = (4095 + 1) / (40,000 Hz / 256)
	// Timeout ≈ (4096) / (156.25 Hz)
	// Timeout ≈ 26.13 seconds (approximately)
	// Source: ChatGPT
	// TODO: confirm this timeout period is correct
}


void Wdog_reset(void)
{
	IWDG->KR = 0xAAAA; //reset
}
