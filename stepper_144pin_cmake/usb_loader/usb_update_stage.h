/**
 * @file usb_update_stage.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Copies a validated update file from a mounted FileX volume into the SPI-flash staging
 *        slot and verifies the staged copy's payload CRC.
 *
 *        The file on the drive is already header+payload in exactly the on-flash layout the
 *        bootloader expects (see usb_update_file.h), so it is copied byte-for-byte to
 *        STAGED_HEADER_OFFSET with no repackaging -- the same blob the USART3 loader stages,
 *        arriving over a different transport.
 *
 *        This module stops once the staged copy is verified. Arming the bootloader trigger and
 *        resetting is the caller's decision, made with btl_flash_trigger_arm_and_reset().
 * @version 0.1
 * @date 2026-07-28
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef USB_UPDATE_STAGE_H_
#define USB_UPDATE_STAGE_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdint.h>

#include "fx_api.h"
#include "usb_update_file.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/
/** Outcome of usb_update_stage_copy_and_verify(). Anything other than USB_UPDATE_STAGE_OK means
 * the staging slot's contents are incomplete or unverified and must not be committed. */
typedef enum {
  USB_UPDATE_STAGE_OK = 0,             /* the whole file is staged and its payload CRC matches */
  USB_UPDATE_STAGE_OPEN_FAILED,        /* the file would not open for reading */
  USB_UPDATE_STAGE_ERASE_FAILED,       /* the staging slot could not be erased */
  USB_UPDATE_STAGE_FILE_READ_FAILED,   /* a read from the USB drive failed or came up short */
  USB_UPDATE_STAGE_FLASH_WRITE_FAILED, /* a write to SPI flash failed its read-back verify */
  USB_UPDATE_STAGE_FLASH_READ_FAILED,  /* the staged copy could not be read back for the CRC */
  USB_UPDATE_STAGE_CRC_MISMATCH        /* the staged payload's CRC disagrees with the header's */
} usb_update_stage_status_enum;

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

/*******************************************************************************
 *Function Prototypes
 *******************************************************************************/
/**
 * @brief erases the SPI-flash staging slot, copies the update file into it, and confirms the
 *        staged payload's CRC16-CCITT matches the value in the file's header
 * @note blocking and slow -- an erase plus a read-back-verified write of every 512-byte chunk,
 *       then a second full pass to check the CRC. Expect seconds for a typical image. Reports
 *       progress over the app console as it goes.
 *
 *       SPI flash has no locking (see spi_flash_io.h) and this call holds it for that whole
 *       time, which is far longer than any existing caller does. A "param set" typed at the
 *       console while this runs reaches the same unguarded driver state from the console RX
 *       task, which sits at the same FreeRTOS priority as the USB loader task. Known and
 *       accepted for now; the fix is a lock in spi_flash_io, deliberately deferred.
 *
 *       Nothing is committed here: the bootloader trigger is untouched, so a failure part-way
 *       through leaves a corrupt staging slot that the bootloader will never be told to apply.
 * @param media mounted FileX media holding the file; must not be NULL
 * @param file file returned by usb_update_file_find(); must not be NULL. Its header and
 *        file_size are trusted -- usb_update_file_find() has already confirmed they agree with
 *        each other and that the file fits STAGED_IMAGE_MAX_SIZE.
 * @return usb_update_stage_status_enum USB_UPDATE_STAGE_OK when the staged image is complete and
 *         verified, otherwise the reason staging failed
 */
usb_update_stage_status_enum usb_update_stage_copy_and_verify(FX_MEDIA *media, const usb_update_file_t *file);

/**
 * @brief maps a usb_update_stage_copy_and_verify() result to a short human-readable phrase for
 *        the console
 * @param status status to describe
 * @return const CHAR* a static, never-NULL description
 */
const CHAR *usb_update_stage_status_string(usb_update_stage_status_enum status);

#endif /* USB_UPDATE_STAGE_H_ */
