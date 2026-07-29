/**
 * @file crc16_ccitt.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief CRC16-CCITT (poly 0x1021, init 0xFFFF, no reflection -- "CCITT-FALSE")
 * @version 0.1
 * @date 2026-07-22
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef CRC16_CCITT_H_
#define CRC16_CCITT_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include <stddef.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define CRC16_CCITT_INITIAL_VALUE 0xFFFFu

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/
/**
 * @brief runs CRC16-CCITT over one chunk of data, carrying the running value between chunks
 * @note pass CRC16_CCITT_INITIAL_VALUE as running_crc to start a new checksum; feed the
 *       returned value back in as running_crc for the next chunk
 * @param running_crc CRC value accumulated so far
 * @param data buffer to checksum; if NULL, running_crc is returned unchanged
 * @param length number of bytes in data to checksum
 * @return uint16_t updated running CRC value
 */
uint16_t crc16_ccitt_update(uint16_t running_crc, const uint8_t *data, size_t length);

#endif /* CRC16_CCITT_H_ */
