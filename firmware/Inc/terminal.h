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

#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "drivers/uart.h"
#include <stdint.h>
#include <string.h>

extern uint8_t internal_response_buffer[500]; // internal buffer for reading response of code

typedef enum
{
	TERM_ANY,
	TERM_UART1,
	TERM_UART2,
	TERM_NONE, // no terminal to write to
	TERM_INTERNAL_BUFFER, // FIXME: make this do something
} Terminal_stream;

#define TERMBUFLEN 300

/**
 * @brief Handle "special" terminal cases like backspace or local echo
 * @param[in] src Source: TERM_USB, TERM_UART1, TERM_UART2
 * @attention Must be called for every received data
 */
void term_handleSpecial(Terminal_stream src);


/**
 * \brief Send data to all available monitor outputs
 * \param[in] *data Data to send
 * \param[in] len Data length or 0 for NULL-terminated data
 */
void term_sendMonitor(uint8_t *data, uint16_t len);

void term_sendToMode(uint8_t *data, uint16_t len, Uart_mode mode);

/**
 * \brief Send number to all available monitor outputs
 * \param[in] data Number to send
 */
void term_sendMonitorNumber(int32_t data);

void term_sendNumberToMode(int32_t data, Uart_mode mode);

/**
* \brief Send terminal buffer using specified stream
* \param[in] way Stream: TERM_ANY, TERM_USB, TERM_UART1, TERM_UART2
*/
void term_sendBuf(Terminal_stream way);

/**
 * \brief Push byte to terminal buffer
 * \param[in] data Byte to store
 */
void term_sendByte(uint8_t data);

/**
 * \brief Push string to terminal buffer
 * \param[in] *data String
 * \param[in] len String length or 0 for NULL-terminated string
 */
void term_sendString(uint8_t *data, uint16_t len);

/**
 * \brief Push number (in ASCII form) in terminal buffer
 * \param[in] n Number
 */
void term_sendNumber(int32_t n);

/**
 * \brief Parse and process received data
 * \param[in] *cmd Data
 * \param[in] len Data length
 * \param[in] src Source: TERM_USB, TERM_UART1, TERM_UART2
 * \param[in] type Data type: DATA_KISS, DATA_TERM, etc.
 * \param[in] mode Input mode: MODE_KISS, MODE_TERM, MODE_MONITOR, etc.
 */
void term_parse(uint8_t *cmd, uint16_t len, Terminal_stream src, Uart_data_type type, Uart_mode mode);

void term_setPortMode(uint8_t *cmd, uint16_t len, Terminal_stream src, Uart_data_type type, Uart_mode mode);

void term_doIncomingMonitorCommand(uint8_t *cmd, uint16_t len, Terminal_stream src);

void term_doIncomingTerminalCommand(uint8_t *cmd, uint16_t len, Terminal_stream src);

void term_printHelpMessage(Terminal_stream src);

void switchPortToMonitorMode(Terminal_stream src);

void execute_vp_digi_config_cmd(uint8_t cmd[]);
void execute_vp_digi_monitor_cmd(uint8_t cmd[]);

#endif /* DEBUG_H_ */
