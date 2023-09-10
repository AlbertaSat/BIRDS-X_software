
#include "frame_handler.h"
#include "common.h"
#include "ax25.h"
#include "digipeater.h"
#include "mboss_handler.h"
#include "drivers/modem.h"
#include "drivers/uart.h"

#include <string.h>
#include <stdio.h>

// store and forward buffer storage (externs)
uint8_t sf_buffer[STORE_AND_FORWARD_BUFFER_SIZE];
uint16_t sf_buffer_wr_idx = 0;
uint32_t frame_rx_count_since_boot = 0;
uint32_t beacon_count_since_boot = 0;

/**
 * \brief Handle received frame from RF
 */
void handleFrame(void)
{
	uint8_t modemReceived = ax25.frameReceived; //store states
	ax25.frameReceived = 0; //clear flag

	uint8_t buf[FRAMELEN]; // buffer for raw frames
	uint8_t buf_tnc2[FRAMELEN + 30]; // buffer for raw frames, converted to TNC2 frames
	uint16_t bufidx = 0;
	uint16_t i = ax25.frameBufRd;

	while(i != ax25.frameBufWr)
	{
		if(ax25.frameBuf[i] != 0xFF)
		{
			buf[bufidx++] = ax25.frameBuf[i++]; //store frame in temporary buffer
		}
		else
		{
			break;
		}
		i %= (FRAMEBUFLEN);
	}

	ax25.frameBufRd = ax25.frameBufWr;

	for(i = 0; i < (bufidx); i++)
	{
		if(buf[i] & 1)
			break; //look for path end bit
	}


	SendKiss(buf, bufidx); //send KISS frames if ports available

	// store for store-and-forward mode
	if (get_current_aprs_mode() == RF_APRS_MODE_STORE_AND_FORWARD) {
		store_frame_for_store_and_forward(buf, bufidx);
	}

	frame_rx_count_since_boot++;


	if(((uart1.mode == MODE_MONITOR) || (uart2.mode == MODE_MONITOR)))
	{
		common_toTNC2(buf, bufidx, buf_tnc2); //convert to TNC2 format

		//in general, the RMS of the frame is calculated (excluding preamble!)
		//it it calculated from samples ranging from -4095 to 4095 (amplitude of 4095)
		//that should give a RMS of around 2900 for pure sine wave
		//for pure square wave it should be equal to the amplitude (around 4095)
		//real data contains lots of imperfections (especially mark/space amplitude imbalance) and this value is far smaller than 2900 for standard frames
		//division by 9 was selected by trial and error to provide a value of 100(%) when the input signal had peak-peak voltage of 3.3V
		//this probably should be done in a different way, like some peak amplitude tracing
		ax25.sLvl /= 9;

		if(ax25.sLvl > 100) {
			term_sendMonitor((uint8_t*)"\r\nInput level too high! Please reduce so most stations are around 50-70%.\r\n", 0);
		}
		else if(ax25.sLvl < 10) {
			term_sendMonitor((uint8_t*)"\r\nInput level too low! Please increase so most stations are around 50-70%.\r\n", 0);
		}

		term_sendMonitor((uint8_t*)"(AX.25) Frame received [", 0); //show which modem received the frame: [FP] (flat and preemphasized), [FD] (flat and deemphasized - in flat audio input mode)
																   //[F_] (only flat), [_P] (only preemphasized) or [_D] (only deemphasized - in flat audio input mode)
		uint8_t t[2] = {0};
		if(modemReceived & 1)
		{
			t[0] = 'F';
		}
		else
			t[0] = '_';
		if(modemReceived & 2)
		{
			if(afskCfg.flatAudioIn)
				t[1] = 'D';
			else
				t[1] = 'P';
		}
		else
			t[1] = '_';

		term_sendMonitor(t, 2);
		term_sendMonitor((uint8_t*)"], signal level ", 0);
		term_sendMonitorNumber(ax25.sLvl);
		term_sendMonitor((uint8_t*)"%: ", 0);

		term_sendMonitor(buf_tnc2, 0);
		term_sendMonitor((uint8_t*)"\r\n", 0);

	}


	if(digi.enable) {
		Digi_digipeat(buf, bufidx);
	}

}

void store_frame_for_store_and_forward(uint8_t *buf, uint16_t buflen) {
	// similar to Send_Kiss();

	if (sf_buffer_wr_idx + buflen + STORED_FRAME_DELIM_LEN + 3 > STORE_AND_FORWARD_BUFFER_SIZE) { // 3 arbitrary for safety
		// OPTION 1: circle back to start (bad)
		// sf_buffer_wr_idx = 0;

		// OPTION 2: shift left by a couple frames to make room
		uint8_t frame_array[FRAMELEN+10];
		uint16_t frame_len = 0;
		for (int i = 0; i < 15; i++) { // i < n, where n is the number of frames to kill
			get_leftmost_stored_frame_and_shift_left(frame_array, &frame_len);
		}
	}

	if (sf_buffer_wr_idx + buflen + STORED_FRAME_DELIM_LEN + 3 > STORE_AND_FORWARD_BUFFER_SIZE) { // 3 arbitrary for safety
		// still not enough room, do the nuclear option
		sf_buffer_wr_idx = 0;
	}

	for (uint16_t i = 0; i < buflen; i++) {
		sf_buffer[sf_buffer_wr_idx++] = buf[i];
	}

	// add delim
	sf_buffer[sf_buffer_wr_idx++] = STORED_FRAME_DELIM_BYTE_0;
	sf_buffer[sf_buffer_wr_idx++] = STORED_FRAME_DELIM_BYTE_1;
	sf_buffer[sf_buffer_wr_idx++] = STORED_FRAME_DELIM_BYTE_2;
	sf_buffer[sf_buffer_wr_idx++] = STORED_FRAME_DELIM_BYTE_3;

	// send_str_to_mboss("INFO: stored frame for store-and-forward");
}

void clear_frame_store() {
	sf_buffer_wr_idx = 0;
}

uint16_t get_stored_frame_count() {
	uint16_t frame_count = 0;

	for (uint16_t i = 0; i < sf_buffer_wr_idx - (STORED_FRAME_DELIM_LEN-1); i++) {
		if (sf_buffer[i] == STORED_FRAME_DELIM_BYTE_0 &&
			sf_buffer[i+1] == STORED_FRAME_DELIM_BYTE_1 &&
			sf_buffer[i+2] == STORED_FRAME_DELIM_BYTE_2 &&
			sf_buffer[i+3] == STORED_FRAME_DELIM_BYTE_3
		) {
			frame_count++;
			i += STORED_FRAME_DELIM_LEN-1; // skip a few for safety and to avoid weird cases with the delim near borders
		}
	}
	return frame_count;
}

// returns >0 if there's an error
uint8_t get_leftmost_stored_frame_and_shift_left(uint8_t result_array[], uint16_t *result_len) {
	if (sf_buffer_wr_idx == 0) {
		return 1;
	}

	uint16_t length_of_first_frame = 0; // length of the pre-delim bytes (i.e., the index of the first delim)

	for (uint16_t i = 0; i < sf_buffer_wr_idx - (STORED_FRAME_DELIM_LEN-1); i++) {
		if (sf_buffer[i] == STORED_FRAME_DELIM_BYTE_0 &&
			sf_buffer[i+1] == STORED_FRAME_DELIM_BYTE_1 &&
			sf_buffer[i+2] == STORED_FRAME_DELIM_BYTE_2 &&
			sf_buffer[i+3] == STORED_FRAME_DELIM_BYTE_3
		) {
			length_of_first_frame = i;
			break;
		}
	}

	// give back the length of the frame
	*result_len = length_of_first_frame;

	if (length_of_first_frame > FRAMELEN) {
		// probably won't happen, but let's just catch it anyway
		return 2;
	}

	// copy the frame into the dest
	memcpy(result_array, sf_buffer, length_of_first_frame);

	// move all elements left by (length_of_first_frame + STORED_FRAME_DELIM_LEN)
	for (uint16_t i = 0; i < sf_buffer_wr_idx - (length_of_first_frame + STORED_FRAME_DELIM_LEN); i++) {
		if (i >= STORE_AND_FORWARD_BUFFER_SIZE) {
			// mem safety
			return 3;
		}
		sf_buffer[i] = sf_buffer[i + length_of_first_frame + STORED_FRAME_DELIM_LEN];
	}

	// update sf_buffer_wr_idx
	sf_buffer_wr_idx -= (length_of_first_frame + STORED_FRAME_DELIM_LEN);

	return 0;
}
