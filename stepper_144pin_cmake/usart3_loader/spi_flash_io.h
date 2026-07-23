/**
 * @file spi_flash_io.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Blocking wrapper over the non-blocking driver_w25q API. driver_w25q itself is
 *        interrupt-driven (spi_portable_* transfers complete via SPI2 IRQ callbacks and are
 *        polled through w25q_get_transfer_status()) -- this module hides that polling behind
 *        simple blocking calls for the USART3 image-loader's single-threaded control flow.
 * @version 0.1
 * @date 2026-07-22
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef SPI_FLASH_IO_H_
#define SPI_FLASH_IO_H_

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
 * @brief initializes the underlying w25q driver
 * @note the application already calls w25q_initialize() from main() before the scheduler
 *       starts (it must run before any FreeRTOS API call, per the SPI2 IRQ priority
 *       constraint noted in main.c), so the loader does NOT call this. It is retained for
 *       parity with the bootloader's copy of this module and for standalone use.
 */
void spi_flash_io_init(void);

/**
 * @brief erases enough W25Q16 sectors to cover [offset, offset + length), rounding the
 *        covered range up to the sector erase size
 * @param offset SPI-flash byte offset the erased range must start at or before
 * @param length number of bytes that must end up erased starting at offset
 * @return bool true if every sector erase completed successfully
 */
bool spi_flash_io_erase_range(uint32_t offset, uint32_t length);

/**
 * @brief blocking write to SPI flash, with read-back verification and retry
 * @note writes have been observed to occasionally report success without the data actually
 *       landing in flash. To catch that, every write is immediately read back and compared;
 *       a mismatch is retried (re-written and re-verified) up to 3 times, printing a failure
 *       line over the app console on every failed attempt, including the last
 * @param offset SPI-flash byte offset to write to
 * @param data buffer holding the bytes to write; must not be NULL
 * @param length number of bytes to write; capped at 512 bytes (SPI_FLASH_IO_WRITE_VERIFY_MAX_LEN
 *        in spi_flash_io.c) -- longer writes always fail
 * @return bool true if the write was verified to match within the retry budget
 */
bool spi_flash_io_write(uint32_t offset, const uint8_t *data, uint32_t length);

/**
 * @brief blocking read from SPI flash
 * @param offset SPI-flash byte offset to read from
 * @param data destination buffer; must not be NULL
 * @param length number of bytes to read
 * @return bool true if the read completed successfully
 */
bool spi_flash_io_read(uint32_t offset, uint8_t *data, uint32_t length);

#endif /* SPI_FLASH_IO_H_ */
