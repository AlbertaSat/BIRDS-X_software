
#ifndef __FRAME_HANDLER_H__
#define __FRAME_HANDLER_H__

#include "common.h"

// store and forward buffer storage
#define STORE_AND_FORWARD_BUFFER_SIZE 10000
extern uint8_t sf_buffer[STORE_AND_FORWARD_BUFFER_SIZE];
extern uint16_t sf_buffer_wr_idx;

/**
 * \brief Handle received frame from RF
 */
void handleFrame(void);

void store_frame_for_store_and_forward(uint8_t *buf, uint16_t buflen);

void clear_frame_store();

#endif	// __FRAME_HANDLER_H__
