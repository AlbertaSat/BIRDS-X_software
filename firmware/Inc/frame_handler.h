
#ifndef __FRAME_HANDLER_H__
#define __FRAME_HANDLER_H__

#include "common.h"

// store and forward buffer storage
#define STORE_AND_FORWARD_BUFFER_SIZE 10000
extern uint8_t sf_buffer[STORE_AND_FORWARD_BUFFER_SIZE];
extern uint16_t sf_buffer_wr_idx;
extern uint32_t frame_rx_count_since_boot;
extern uint32_t beacon_count_since_boot;
extern uint32_t timestamp_rx_led_turned_on_ms;

#define STORED_FRAME_DELIM_BYTE_0 0xFF
#define STORED_FRAME_DELIM_BYTE_1 0xA0
#define STORED_FRAME_DELIM_BYTE_2 0xFF
#define STORED_FRAME_DELIM_BYTE_3 0xDF
#define STORED_FRAME_DELIM_LEN 4 // four bytes defined above

/**
 * \brief Handle received frame from RF
 */
void handleFrame(void);

void store_frame_for_store_and_forward(uint8_t *buf, uint16_t buflen);

void clear_frame_store();

uint16_t get_stored_frame_count();

uint8_t get_leftmost_stored_frame_and_shift_left(uint8_t result_array[], uint16_t *result_len);

#endif	// __FRAME_HANDLER_H__
