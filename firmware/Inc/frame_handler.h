
#ifndef __FRAME_HANDLER_H__
#define __FRAME_HANDLER_H__

#include "common.h"


/**
 * \brief Handle received frame from RF
 */
void handleFrame(void);

void store_frame_for_store_and_forward(uint8_t *buf, uint16_t buflen);


#endif	// __FRAME_HANDLER_H__
