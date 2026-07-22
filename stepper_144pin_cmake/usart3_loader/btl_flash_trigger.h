/**
 * @file btl_flash_trigger.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Application-side arming of the bootloader's commit trigger. The trigger lives in the
 *        W25Q SPI flash (first sector) so it survives the reset regardless of what the reset
 *        or a debug session does to SRAM. This is the application half of the mechanism: the
 *        app, after staging and CRC-validating an image, arms the flag and resets; the
 *        bootloader reads the same flag on the next boot and applies the staged image. The
 *        flag layout here must stay byte-for-byte identical to the bootloader's copy of this
 *        module (btl_flash_trigger.c in the bootloader project) -- they are the two ends of
 *        one cross-project contract.
 * @note the flag lives in the first W25Q sector; STAGED_HEADER_OFFSET (see update_image.h)
 *       starts right after it, so streaming a new image never disturbs the flag.
 * @version 0.1
 * @date 2026-07-22
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef BTL_FLASH_TRIGGER_H_
#define BTL_FLASH_TRIGGER_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

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
 * @brief erases and rewrites the bootloader commit trigger in SPI flash, then performs a
 *        software reset so the board reboots into the bootloader to apply the staged image
 * @note does not return if the flag was written successfully -- the reset happens instead.
 *       Returns false without resetting if the SPI-flash erase/write failed, so the caller
 *       can report the failure instead of silently rebooting into a state that isn't armed.
 * @param staged_header_offset SPI-flash byte offset of the staged header to apply on reboot
 *        (STAGED_HEADER_OFFSET for this iteration's single staging slot)
 * @return bool false if arming failed (erase/write error); never returns on success
 */
bool btl_flash_trigger_arm_and_reset(uint32_t staged_header_offset);

#endif /* BTL_FLASH_TRIGGER_H_ */
