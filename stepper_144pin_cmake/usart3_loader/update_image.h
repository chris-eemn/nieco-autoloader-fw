/**
 * @file update_image.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Staged-update file format and SPI-flash staging location, shared with the bootloader.
 *        The application uses this only to stage an incoming image into SPI flash over USART3;
 *        the bootloader is what later copies the staged image into internal application flash,
 *        so the internal-flash geometry constants that live in the bootloader's copy of this
 *        header are intentionally omitted here.
 * @version 0.1
 * @date 2026-07-22
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef UPDATE_IMAGE_H_
#define UPDATE_IMAGE_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include "w25q.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** Fixed compile-time SPI-flash byte offset of the single staging slot. Must match the
 * bootloader's STAGED_HEADER_OFFSET -- the bootloader reads the staged image back from this
 * exact offset at apply time.
 *
 * Starts right after the first W25Q sector, which the bootloader reserves for its commit-
 * trigger flag -- keeping that flag's own sector erase from ever touching the staged image,
 * and leaving the rest of the chip past the staged image free for other application-related
 * flash usage. */
#define STAGED_HEADER_OFFSET (W25Q_SECTOR_SIZE)

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
/** Legacy update-file header, reused as-is. No magic number and no header self-CRC --
 * known limitation of reusing this format, accepted for this iteration. The CRC covers
 * the payload only, and the payload immediately follows this header in SPI flash. */
typedef struct {
  uint16_t version;
  uint16_t crc16_ccit_checksum; /* CRC over the payload only, not the header */
  uint32_t binary_size;         /* payload size, excluding this header */
  uint8_t version_major;
  uint8_t version_minor;
  uint8_t version_build;
  uint8_t reserved1;
  uint32_t reserved2;
} ede_update_file_header_t; /* 16 bytes */

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/

#endif /* UPDATE_IMAGE_H_ */
