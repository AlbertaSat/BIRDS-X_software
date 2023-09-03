
#include "mboss_handler.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


void receive_incoming_boss_cmd(uint8_t *cmd, uint16_t len, Terminal_stream src) {
	

	// DEBUG: printf the received command back, in hex
	// TODO: change this part to the echo command
	char received_msg_as_hex[255];
	sprintf(received_msg_as_hex, "Received BOSS cmd: ");
	for (int i = 0; i < len; i++) {
		char hex[10];
		sprintf(hex, "%02X ", cmd[i]);
		strcat(received_msg_as_hex, hex);
	}
	strcat(received_msg_as_hex, "<END>\n");

	term_sendToMode(received_msg_as_hex, strlen(received_msg_as_hex), MODE_BOSS); // send msg like "Received BOSS cmd: 0xE0 0x64 0xED <END>\n"

	// FIXME: implement this

}

