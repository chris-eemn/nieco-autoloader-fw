/**
 * @file spi_flash_io.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief
 * @version 0.1
 * @date 2026-07-22
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <string.h>
#include "spi_flash_io.h"
#include "stm32_hal.h"
#include "app_console.h"
#include "w25q.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define SPI_FLASH_IO_TRANSFER_TIMEOUT_MS 5000u

/* w25q writes were observed to occasionally report success (transfer status COMPLETED)
 * without the data actually landing in flash -- a subsequent read of the same address would
 * come back still showing the pre-write (erased, 0xFF) contents. Root cause looked like a
 * race around the erase/write busy-bit status-register polling rather than anything in a
 * specific address range (a directly-addressed sector that failed once passed cleanly on a
 * later retry, and vice versa for other sectors -- not a fixed bad sector). Rather than
 * chase that further on a link this project is already moving off of, every write reads
 * itself back and retries on mismatch. This is intentionally kept in this project-owned
 * wrapper rather than in driver_w25q (the vendored driver) or driver_w25q_port (the generic
 * SPI2 port layer) -- neither of those needs to know about this. */
#define SPI_FLASH_IO_WRITE_VERIFY_MAX_LEN 512u
#define SPI_FLASH_IO_WRITE_MAX_RETRIES 3u

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
static uint8_t verify_buffer[SPI_FLASH_IO_WRITE_VERIFY_MAX_LEN];

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
/**
 * @brief blocks until the current w25q transfer leaves the busy state or the timeout elapses
 * @return bool true if the transfer completed successfully before the timeout
 */
static bool wait_for_transfer_complete(void);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
void spi_flash_io_init(void) {
  w25q_initialize();
}

bool spi_flash_io_erase_range(uint32_t offset, uint32_t length) {
  bool erase_ok = true;
  uint32_t sector_address = (offset / W25Q_SECTOR_SIZE) * W25Q_SECTOR_SIZE;
  uint32_t end_offset = offset + length;

  while ((sector_address < end_offset) && (erase_ok == true)) {
    if (w25q_sector_erase(sector_address) == true) {
      erase_ok = wait_for_transfer_complete();
    } else {
      erase_ok = false;
    }

    sector_address += W25Q_SECTOR_SIZE;
  }

  return erase_ok;
}

bool spi_flash_io_write(uint32_t offset, const uint8_t *data, uint32_t length) {
  bool verified = false;
  bool write_ok;
  bool read_ok;
  uint32_t attempt;

  if ((data != NULL) && (length <= (uint32_t)SPI_FLASH_IO_WRITE_VERIFY_MAX_LEN)) {
    for (attempt = 0u; (attempt < SPI_FLASH_IO_WRITE_MAX_RETRIES) && (verified == false); attempt++) {
      write_ok = false;

      if (w25q_write((uint8_t *)data, length, offset) == true) {
        write_ok = wait_for_transfer_complete();
      }

      if (write_ok == true) {
        read_ok = false;

        if (w25q_read(verify_buffer, length, offset) == true) {
          read_ok = wait_for_transfer_complete();
        }

        verified = (read_ok == true) && (memcmp(verify_buffer, data, length) == 0);
      }

      if (verified == false) {
        app_console_print("spi_flash_io: write-verify failed at offset 0x%08lX (%lu bytes), attempt %lu/%lu\r\n",
                          (unsigned long)offset, (unsigned long)length, (unsigned long)(attempt + 1u),
                          (unsigned long)SPI_FLASH_IO_WRITE_MAX_RETRIES);
      }
    }
  }

  return verified;
}

bool spi_flash_io_read(uint32_t offset, uint8_t *data, uint32_t length) {
  bool read_ok = false;

  if (data != NULL) {
    if (w25q_read(data, length, offset) == true) {
      read_ok = wait_for_transfer_complete();
    }
  }

  return read_ok;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static bool wait_for_transfer_complete(void) {
  uint32_t start_tick = HAL_GetTick();
  W25Q_TRANSFER_STATUS status;

  do {
    status = w25q_get_transfer_status();
  } while ((status == W25Q_TRANSFER_STATUS_BUSY) &&
           ((HAL_GetTick() - start_tick) < SPI_FLASH_IO_TRANSFER_TIMEOUT_MS));

  return (status == W25Q_TRANSFER_STATUS_COMPLETED);
}
