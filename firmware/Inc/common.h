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
#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

// include some un-called HAL stuff
////////////////////////////////////////////////////

/*******************  Bit definition for ADC_SQR3 register  *******************/
#define ADC_SQR3_SQ1_Pos                    (0U)                               
#define ADC_SQR3_SQ1_Msk                    (0x1FUL << ADC_SQR3_SQ1_Pos)        /*!< 0x0000001F */
#define ADC_SQR3_SQ1                        ADC_SQR3_SQ1_Msk                   /*!< ADC group regular sequencer rank 1 */
#define ADC_SQR3_SQ1_0                      (0x01UL << ADC_SQR3_SQ1_Pos)        /*!< 0x00000001 */
#define ADC_SQR3_SQ1_1                      (0x02UL << ADC_SQR3_SQ1_Pos)        /*!< 0x00000002 */
#define ADC_SQR3_SQ1_2                      (0x04UL << ADC_SQR3_SQ1_Pos)        /*!< 0x00000004 */
#define ADC_SQR3_SQ1_3                      (0x08UL << ADC_SQR3_SQ1_Pos)        /*!< 0x00000008 */
#define ADC_SQR3_SQ1_4                      (0x10UL << ADC_SQR3_SQ1_Pos)        /*!< 0x00000010 */

#define ADC_SQR3_SQ2_Pos                    (5U)                               
#define ADC_SQR3_SQ2_Msk                    (0x1FUL << ADC_SQR3_SQ2_Pos)        /*!< 0x000003E0 */
#define ADC_SQR3_SQ2                        ADC_SQR3_SQ2_Msk                   /*!< ADC group regular sequencer rank 2 */
#define ADC_SQR3_SQ2_0                      (0x01UL << ADC_SQR3_SQ2_Pos)        /*!< 0x00000020 */
#define ADC_SQR3_SQ2_1                      (0x02UL << ADC_SQR3_SQ2_Pos)        /*!< 0x00000040 */
#define ADC_SQR3_SQ2_2                      (0x04UL << ADC_SQR3_SQ2_Pos)        /*!< 0x00000080 */
#define ADC_SQR3_SQ2_3                      (0x08UL << ADC_SQR3_SQ2_Pos)        /*!< 0x00000100 */
#define ADC_SQR3_SQ2_4                      (0x10UL << ADC_SQR3_SQ2_Pos)        /*!< 0x00000200 */

#define ADC_SQR3_SQ3_Pos                    (10U)                              
#define ADC_SQR3_SQ3_Msk                    (0x1FUL << ADC_SQR3_SQ3_Pos)        /*!< 0x00007C00 */
#define ADC_SQR3_SQ3                        ADC_SQR3_SQ3_Msk                   /*!< ADC group regular sequencer rank 3 */
#define ADC_SQR3_SQ3_0                      (0x01UL << ADC_SQR3_SQ3_Pos)        /*!< 0x00000400 */
#define ADC_SQR3_SQ3_1                      (0x02UL << ADC_SQR3_SQ3_Pos)        /*!< 0x00000800 */
#define ADC_SQR3_SQ3_2                      (0x04UL << ADC_SQR3_SQ3_Pos)        /*!< 0x00001000 */
#define ADC_SQR3_SQ3_3                      (0x08UL << ADC_SQR3_SQ3_Pos)        /*!< 0x00002000 */
#define ADC_SQR3_SQ3_4                      (0x10UL << ADC_SQR3_SQ3_Pos)        /*!< 0x00004000 */

#define ADC_SQR3_SQ4_Pos                    (15U)                              
#define ADC_SQR3_SQ4_Msk                    (0x1FUL << ADC_SQR3_SQ4_Pos)        /*!< 0x000F8000 */
#define ADC_SQR3_SQ4                        ADC_SQR3_SQ4_Msk                   /*!< ADC group regular sequencer rank 4 */
#define ADC_SQR3_SQ4_0                      (0x01UL << ADC_SQR3_SQ4_Pos)        /*!< 0x00008000 */
#define ADC_SQR3_SQ4_1                      (0x02UL << ADC_SQR3_SQ4_Pos)        /*!< 0x00010000 */
#define ADC_SQR3_SQ4_2                      (0x04UL << ADC_SQR3_SQ4_Pos)        /*!< 0x00020000 */
#define ADC_SQR3_SQ4_3                      (0x08UL << ADC_SQR3_SQ4_Pos)        /*!< 0x00040000 */
#define ADC_SQR3_SQ4_4                      (0x10UL << ADC_SQR3_SQ4_Pos)        /*!< 0x00080000 */

#define ADC_SQR3_SQ5_Pos                    (20U)                              
#define ADC_SQR3_SQ5_Msk                    (0x1FUL << ADC_SQR3_SQ5_Pos)        /*!< 0x01F00000 */
#define ADC_SQR3_SQ5                        ADC_SQR3_SQ5_Msk                   /*!< ADC group regular sequencer rank 5 */
#define ADC_SQR3_SQ5_0                      (0x01UL << ADC_SQR3_SQ5_Pos)        /*!< 0x00100000 */
#define ADC_SQR3_SQ5_1                      (0x02UL << ADC_SQR3_SQ5_Pos)        /*!< 0x00200000 */
#define ADC_SQR3_SQ5_2                      (0x04UL << ADC_SQR3_SQ5_Pos)        /*!< 0x00400000 */
#define ADC_SQR3_SQ5_3                      (0x08UL << ADC_SQR3_SQ5_Pos)        /*!< 0x00800000 */
#define ADC_SQR3_SQ5_4                      (0x10UL << ADC_SQR3_SQ5_Pos)        /*!< 0x01000000 */

#define ADC_SQR3_SQ6_Pos                    (25U)                              
#define ADC_SQR3_SQ6_Msk                    (0x1FUL << ADC_SQR3_SQ6_Pos)        /*!< 0x3E000000 */
#define ADC_SQR3_SQ6                        ADC_SQR3_SQ6_Msk                   /*!< ADC group regular sequencer rank 6 */
#define ADC_SQR3_SQ6_0                      (0x01UL << ADC_SQR3_SQ6_Pos)        /*!< 0x02000000 */
#define ADC_SQR3_SQ6_1                      (0x02UL << ADC_SQR3_SQ6_Pos)        /*!< 0x04000000 */
#define ADC_SQR3_SQ6_2                      (0x04UL << ADC_SQR3_SQ6_Pos)        /*!< 0x08000000 */
#define ADC_SQR3_SQ6_3                      (0x08UL << ADC_SQR3_SQ6_Pos)        /*!< 0x10000000 */
#define ADC_SQR3_SQ6_4                      (0x10UL << ADC_SQR3_SQ6_Pos)        /*!< 0x20000000 */


/** @defgroup ADC_channels ADC channels
  * @{
  */
/* Note: Depending on devices, some channels may not be available on package  */
/*       pins. Refer to device datasheet for channels availability.           */
#define ADC_CHANNEL_0                       0x00000000U
#define ADC_CHANNEL_1           ((uint32_t)(                                                                    ADC_SQR3_SQ1_0))
#define ADC_CHANNEL_2           ((uint32_t)(                                                   ADC_SQR3_SQ1_1                 ))
#define ADC_CHANNEL_3           ((uint32_t)(                                                   ADC_SQR3_SQ1_1 | ADC_SQR3_SQ1_0))
#define ADC_CHANNEL_4           ((uint32_t)(                                  ADC_SQR3_SQ1_2                                  ))
#define ADC_CHANNEL_5           ((uint32_t)(                                  ADC_SQR3_SQ1_2                  | ADC_SQR3_SQ1_0))
#define ADC_CHANNEL_6           ((uint32_t)(                                  ADC_SQR3_SQ1_2 | ADC_SQR3_SQ1_1                 ))
#define ADC_CHANNEL_7           ((uint32_t)(                                  ADC_SQR3_SQ1_2 | ADC_SQR3_SQ1_1 | ADC_SQR3_SQ1_0))
#define ADC_CHANNEL_8           ((uint32_t)(                 ADC_SQR3_SQ1_3                                                   ))
#define ADC_CHANNEL_9           ((uint32_t)(                 ADC_SQR3_SQ1_3                                   | ADC_SQR3_SQ1_0))
#define ADC_CHANNEL_10          ((uint32_t)(                 ADC_SQR3_SQ1_3                  | ADC_SQR3_SQ1_1                 ))
#define ADC_CHANNEL_11          ((uint32_t)(                 ADC_SQR3_SQ1_3                  | ADC_SQR3_SQ1_1 | ADC_SQR3_SQ1_0))
#define ADC_CHANNEL_12          ((uint32_t)(                 ADC_SQR3_SQ1_3 | ADC_SQR3_SQ1_2                                  ))
#define ADC_CHANNEL_13          ((uint32_t)(                 ADC_SQR3_SQ1_3 | ADC_SQR3_SQ1_2                  | ADC_SQR3_SQ1_0))
#define ADC_CHANNEL_14          ((uint32_t)(                 ADC_SQR3_SQ1_3 | ADC_SQR3_SQ1_2 | ADC_SQR3_SQ1_1                 ))
#define ADC_CHANNEL_15          ((uint32_t)(                 ADC_SQR3_SQ1_3 | ADC_SQR3_SQ1_2 | ADC_SQR3_SQ1_1 | ADC_SQR3_SQ1_0))
#define ADC_CHANNEL_16          ((uint32_t)(ADC_SQR3_SQ1_4                                                                    ))
#define ADC_CHANNEL_17          ((uint32_t)(ADC_SQR3_SQ1_4                                                    | ADC_SQR3_SQ1_0))

#define ADC_CHANNEL_TEMPSENSOR  ADC_CHANNEL_16  /* ADC internal channel (no connection on device pin) */
#define ADC_CHANNEL_VREFINT     ADC_CHANNEL_17  /* ADC internal channel (no connection on device pin) */


/////////////////////////////////////////////////////////////


#define CRC32_INIT 0xFFFFFFFF

extern uint8_t call[6]; //device callsign
extern uint8_t callSsid; //device ssid

extern uint8_t dest[7]; //destination address for own beacons. Should be APNV01-0 for VP-Digi, but can be changed. SSID MUST remain 0.

extern const uint8_t *versionString; //version string

extern uint8_t autoReset;
extern uint32_t autoResetTimer;
extern uint8_t kissMonitor;

/**
 * @brief Generate random number from min to max
 * @param[in] min Lower boundary
 * @param[in] max Higher boundary
 * @return Generated number
 */
int16_t rando(int16_t min, int16_t max);

/**
 * @brief Convert string to int
 * @param[in] *str Input string
 * @param[in] len String length or 0 to detect by strlen()
 * @return Converted int
 */
int64_t strToInt(uint8_t *str, uint8_t len);

/**
 * @brief Convert AX25 frame to TNC2 (readable) format
 * @param[in] *from Input AX25 frame
 * @param[in] len Input frame length
 * @param[out] *to Destination buffer, will be NULL terminated
 */
void common_toTNC2(uint8_t *from, uint16_t fromlen, uint8_t *to);

/**
 * @brief Calculate CRC32
 * @param[in] crc0 Initial or current CRC value
 * @param[in] *s Input data
 * @param[in] n Input data length
 * @return Calculated CRC32
 */
uint32_t crc32(uint32_t crc0, uint8_t *s, uint64_t n);

/**
 * @brief Send frame to available UARTs and USB in KISS format
 * @param[in] *buf Frame buffer
 * @param[in] len Frame buffer length
 */
void SendKiss(uint8_t *buf, uint16_t len);

uint32_t get_system_uptime_ms();
uint32_t get_system_uptime_sec();

void delay_ms(uint32_t ms);

#endif /* COMMON_H_ */
