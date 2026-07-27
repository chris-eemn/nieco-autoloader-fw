/**
 * @file w25q_config.h
 * @brief Project-specific configuration required by driver_w25q/w25q.c for the
 *        W25Q16V (16 Mbit) flash chip wired to SPI_FLASH_BUS (SPI2), plus the
 *        authoritative top-level address map of the chip.
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

#ifndef W25Q_CONFIG_H_
#define W25Q_CONFIG_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
/* w25q.h (not definitions.h) is included here so the region macros below can be expressed in
 * terms of W25Q_SECTOR_SIZE while this header stays free of any HAL/GPIO dependency -- that
 * keeps it includable from pure logic modules such as cal-data. w25q.c gets definitions.h
 * (PORT_PinSet/PORT_PinClear, SPI_FLASH_CS_PIN) transitively via spi_portable.h. */
#include "w25q.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define W25Q_FLASH_SIZE (0x200000UL) /* W25Q16V: 16 Mbit = 2 MiB */
#define W25Q_COMMAND_QUEUE_SIZE (8U)

/* --- W25Q top-level address map ----------------------------------------------------------
 *
 *   0x000000 - 0x000FFF   bootloader commit-trigger flag     1 sector      4 KiB
 *   0x001000 - 0x0FFFFF   staged firmware image slot       255 sectors  1020 KiB
 *   0x100000 - 0x1FFFFF   cal-data (non-volatile app data) 256 sectors     1 MiB
 *
 * This is the single source of truth for which part of the chip belongs to whom; every module
 * that touches SPI flash derives its addresses from the macros below rather than hard-coding
 * offsets. Region boundaries are sector-aligned so that erasing anything inside one region can
 * never disturb another (the W25Q erase granularity is one 4 KiB sector).
 *
 * The first two regions are shared with the bootloader -- the application only ever writes
 * them (arming the trigger, staging an image) and the bootloader reads them back at boot, so
 * their base addresses MUST match the bootloader's copy of this map exactly.
 *
 * Sizing rationale for the staged image slot: the STM32C5A3ZG has 1 MiB of internal flash and
 * the application is linked at 0x08020000 (128 KiB in, past the bootloader), so the largest
 * application image that can ever be staged is ~896 KiB. The 1020 KiB slot covers that with
 * room to spare, and placing cal-data at the round 0x100000 boundary leaves the whole upper
 * half of the chip (1 MiB) available for application data.
 */

/** Bootloader commit-trigger flag. One sector, reserved solely for the flag -- see
 * usart3_loader/btl_flash_trigger.c. */
#define W25Q_BTL_TRIGGER_BASE_ADDRESS (0x00000000UL)
#define W25Q_BTL_TRIGGER_SIZE (W25Q_SECTOR_SIZE)

/** Cal-data region. Sub-divided into sections by cal-data/cal_data.h. */
#define W25Q_CAL_DATA_BASE_ADDRESS (0x00100000UL)
#define W25Q_CAL_DATA_SIZE (W25Q_FLASH_SIZE - W25Q_CAL_DATA_BASE_ADDRESS)

/** Staged firmware image slot. Fills everything between the trigger flag and cal-data -- see
 * usart3_loader/update_image.h for the on-flash file format written here. */
#define W25Q_STAGED_IMAGE_BASE_ADDRESS (W25Q_BTL_TRIGGER_BASE_ADDRESS + W25Q_BTL_TRIGGER_SIZE)
#define W25Q_STAGED_IMAGE_SIZE (W25Q_CAL_DATA_BASE_ADDRESS - W25Q_STAGED_IMAGE_BASE_ADDRESS)

#endif /* W25Q_CONFIG_H_ */
