/**
 * @file usb_loader.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Owns the USB host role for the application: detects a USB mass-storage thumb drive,
 *        mounts its FAT filesystem via FileX, and (for now) prints the files found at its root.
 *        A later phase extends this to stage a firmware image from the drive into SPI flash.
 *
 *        Deliberately does not use the CubeMX2-generated USBX/FileX applicative stubs
 *        (generated/middleware/mx_usbx_host.c, mx_usbx_host_msc.c) -- those are under the
 *        generated/ vendor boundary and are left untouched/unused. This module duplicates the
 *        small handful of USBX/FileX calls those stubs would have made, using only public
 *        USBX/FileX APIs.
 * @version 0.1
 * @date 2026-07-27
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef USB_LOADER_H_
#define USB_LOADER_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

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
 * @brief Initializes USBX + FileX, registers the STM32 HCD controller and the mass-storage
 *        class, and starts the background task that waits for a USB drive to be inserted and
 *        prints its root directory contents.
 * @note Call once from main(), after w25q_initialize() -- this creates a FreeRTOS task, and
 *       w25q_initialize() must run before any FreeRTOS API call (see main.c).
 */
void usb_loader_start(void);

#endif /* USB_LOADER_H_ */
