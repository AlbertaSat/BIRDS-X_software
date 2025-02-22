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

#include "beacon.h"
#include "digipeater.h"
#include "common.h"
#include "ax25.h"
#include "terminal.h"
#include "frame_handler.h"
#include "mboss_handler.h"
#include "drivers/temperature_sensors.h"
#include "drivers/systick.h"

#include <string.h>
#include <stdio.h>

uint32_t beaconDelay[8] = {0};

Beacon beacon[8];

/**
 * @brief Send specified beacon
 * @param[in] no Beacon number (0-7)
 */
void Beacon_send(uint8_t no)
{
	if(beacon[no].enable == 0)
		return; //beacon disabled

	uint8_t buf[FRAMELEN] = {0}; //frame buffer
	uint16_t idx = 0;

	beacon_count_since_boot++;

	for(uint8_t i = 0; i < 7; i++) //add destination address
		buf[idx++] = dest[i];

	for(uint8_t i = 0; i < 6; i++) //add source address
		buf[idx++] = call[i];

	buf[idx++] = ((callSsid << 1) + 0b01100000); //add source ssid

	if(beacon[no].path[0] > 0) //this beacon has some path set
	{
		for(uint8_t i = 0; i < 14; i++) //loop through path
		{
			if((beacon[no].path[i] > 0) || (i == 6) || (i == 13)) //normal data, not a NULL symbol
			{
				buf[idx] = (beacon[no].path[i] << 1); //copy path
				if((i == 6) || (i == 13)) //it was and ssid
				{
					buf[idx] += 0b01100000; //add appropriate bits for ssid
				}
				idx++;
			}
			else //NULL in path
				break; //end here
		}
	}
	buf[idx - 1] |= 1; //add c-bit on the last element
	buf[idx++] = 0x03; //control
	buf[idx++] = 0xF0; //pid

	if (no == 0) {
		// for JASPER, beacon 0 is a special case which must fetch telemetry data
		uint16_t temp_sensor_val_k = get_external_temperature_k(4);

		uint8_t beacon_data[100];
		sprintf(
			(char*)beacon_data,
			"JASPER-SAT up%lu m%d temp%03dlo%03dhi%03d sto%02d rx%lu tx%lu bt%lu",
			get_system_uptime_sec(),
			(uint8_t) current_aprs_mode,
			temp_sensor_val_k, min_sensor_temp_k, max_sensor_temp_k,
			(uint8_t)(sf_buffer_wr_idx/STORE_AND_FORWARD_BUFFER_SIZE*100),
			frame_rx_count_since_boot, beacon_count_since_boot,
			timestamp_sec_at_boot
		);
		beacon_data[69] = '\0'; // truncate to 69 characters (one less than max of 70 chars)
		for (uint8_t i = 0; i < strlen((char*)beacon_data); i++) {
			buf[idx++] = beacon_data[i]; // copy beacon comment
		}
	}
	else {
		for(uint8_t i = 0; i < strlen((char*)beacon[no].data); i++) {
			buf[idx++] = beacon[no].data[i]; // copy beacon comment
		}
	}

	if((FRAMEBUFLEN - ax25.xmitIdx) > (idx + 2)) //check for free space in TX buffer
	{
		uint16_t frameStart = ax25.xmitIdx; //store index

		for(uint8_t i = 0; i < idx; i++)
		{
			ax25.frameXmit[ax25.xmitIdx++] = buf[i]; //copy frame to main TX buffer
		}

        if(kissMonitor) //monitoring mode, send own frames to KISS ports
        {
        	SendKiss(ax25.frameXmit, ax25.xmitIdx);
        }

		ax25.frameXmit[ax25.xmitIdx++] = 0xFF; //frame separator
		Digi_storeDeDupeFromXmitBuf(frameStart); //store frame hash in duplicate protection buffer (to prevent from digipeating own packets)

		uint8_t bufto[200];
		common_toTNC2((uint8_t *)&ax25.frameXmit[frameStart], ax25.xmitIdx - frameStart - 1, bufto);

		term_sendMonitor((uint8_t*)"(AX.25) Transmitting beacon ", 0);

		term_sendMonitorNumber(no);
		term_sendMonitor((uint8_t*)": ", 0);
		term_sendMonitor(bufto, 0);
		term_sendMonitor((uint8_t*)"\r\n", 0);
	}



}

/**
 * @brief Check if any beacon should be transmitted and transmit if neccessary
 */
void Beacon_check(void)
{
	for(uint8_t i = 0; i < 8; i++)
	{
		if(beacon[i].enable == 0)
			continue;

		if((beacon[i].interval > 0) && ((ticks >= beacon[i].next) || (beacon[i].next == 0)))
		{
			if(beaconDelay[i] > ticks) //check for beacon delay (only for the very first transmission)
				return;
			beacon[i].next = ticks + beacon[i].interval; //save next beacon timestamp
			beaconDelay[i] = 0;
			Beacon_send(i);
		}
	}
}


/**
 * @brief Initialize beacon module
 */
void Beacon_init(void)
{
	for(uint8_t i = 0; i < 8; i++)
	{
		beaconDelay[i] = (beacon[i].delay * 100) + ticks + 3000; //set delay for beacons and add constant 30 seconds of delay
		beacon[i].next = 0;
	}
}

