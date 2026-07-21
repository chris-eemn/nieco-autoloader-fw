/**
 * @file w25q_config.h
 * @brief Project-specific configuration required by driver_w25q/w25q.c for the
 *        W25Q16V (16 Mbit) flash chip wired to SPI_FLASH_BUS (SPI2).
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC. All Rights Reserved.
 */

#ifndef W25Q_CONFIG_H_
#define W25Q_CONFIG_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "definitions.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define W25Q_FLASH_SIZE (0x200000UL) /* W25Q16V: 16 Mbit = 2 MiB */
#define W25Q_COMMAND_QUEUE_SIZE (8U)

#endif /* W25Q_CONFIG_H_ */
