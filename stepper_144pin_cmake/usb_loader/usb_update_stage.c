/**
 * @file usb_update_stage.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief See usb_update_stage.h.
 * @version 0.1
 * @date 2026-07-28
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include "usb_update_stage.h"

#include "FreeRTOS.h"
#include "task.h"

#include "app_console.h"
#include "crc16_ccitt.h"
#include "spi_flash_io.h"
#include "update_image.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
/** Bytes moved per pass. Capped by spi_flash_io_write(), which rejects anything longer than
 * SPI_FLASH_IO_WRITE_VERIFY_MAX_LEN (512) because it read-back-verifies every write out of a
 * buffer of that size. The same buffer serves the CRC read-back pass. */
#define USB_UPDATE_STAGE_CHUNK_SIZE_BYTES 512U

/* Progress feedback over the app console. app_console_print() only queues a line; the console TX
 * task that drains the queue runs at priority 1, below the USB loader task's 2, and the copy loop
 * below never blocks -- so without an explicit yield the queue fills and progress lines are
 * dropped. Printing every USB_UPDATE_STAGE_PROGRESS_STEP_PCT percent and then delaying long
 * enough for the TX task to drain keeps the output intact. Unlike the USART3 loader, nothing is
 * streaming in during the delay: the file is sitting on the drive. */
#define USB_UPDATE_STAGE_PROGRESS_STEP_PCT 10U
#define USB_UPDATE_STAGE_PROGRESS_FLUSH_MS 6U

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
/** Kept off the caller's stack: an FX_FILE control block is several hundred bytes and the chunk
 * buffer another 512. Both are used only by usb_update_stage_copy_and_verify(), which is called
 * only from the usb_loader task, so single-task use is guaranteed the same way
 * usb_update_file.c's statics are. */
static FX_FILE s_open_file;
static uint8_t s_chunk_buffer[USB_UPDATE_STAGE_CHUNK_SIZE_BYTES];

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
/**
 * @brief prints a progress line each time the copy crosses a whole progress step
 * @param previous_done bytes copied before the chunk that just landed
 * @param current_done bytes copied after it
 * @param total total bytes to copy; must not be zero
 */
static void report_progress(uint32_t previous_done, uint32_t current_done, uint32_t total);

/**
 * @brief copies the whole of an already-open file into the staging slot, chunk by chunk
 * @note the file's read position must be at its start; the header is copied along with the
 *       payload, since the on-drive layout is already the on-flash layout.
 * @param total_size number of bytes to copy, header included
 * @return usb_update_stage_status_enum USB_UPDATE_STAGE_OK, or the reason the copy stopped
 */
static usb_update_stage_status_enum copy_file_to_staging(uint32_t total_size);

/**
 * @brief reads the staged payload back out of SPI flash and computes its CRC16-CCITT, holding
 *        only one chunk in SRAM at a time
 * @param binary_size payload length in bytes, excluding the header
 * @param out_crc destination for the computed CRC; must not be NULL
 * @return bool true if every chunk read back successfully
 */
static bool compute_staged_payload_crc(uint32_t binary_size, uint16_t *out_crc);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
usb_update_stage_status_enum usb_update_stage_copy_and_verify(FX_MEDIA *media, const usb_update_file_t *file) {
  usb_update_stage_status_enum result = USB_UPDATE_STAGE_OPEN_FAILED;

  if ((media != NULL) && (file != NULL)) {
    app_console_print("[USB] Staging %s (%lu bytes) -- erasing staging area...\r\n", file->name, (unsigned long)file->file_size);
    vTaskDelay(pdMS_TO_TICKS(USB_UPDATE_STAGE_PROGRESS_FLUSH_MS));

    /* Erase covers the header as well as the payload, so the whole file size is passed here.
     * usb_update_file_find() has already confirmed that fits STAGED_IMAGE_MAX_SIZE, and the
     * staging region is sector-aligned, so rounding the erase up to a sector boundary cannot
     * reach the bootloader trigger flag below it or cal-data above it. */
    if (spi_flash_io_erase_range(STAGED_HEADER_OFFSET, file->file_size) == false) {
      result = USB_UPDATE_STAGE_ERASE_FAILED;
    }
    else if (fx_file_open(media, &s_open_file, (CHAR *)file->name, FX_OPEN_FOR_READ) != FX_SUCCESS) {
      result = USB_UPDATE_STAGE_OPEN_FAILED;
    }
    else {
      result = copy_file_to_staging(file->file_size);

      (void)fx_file_close(&s_open_file);

      if (result == USB_UPDATE_STAGE_OK) {
        uint16_t computed_crc = CRC16_CCITT_INITIAL_VALUE;

        app_console_print("[USB] Verifying staged image...\r\n");
        vTaskDelay(pdMS_TO_TICKS(USB_UPDATE_STAGE_PROGRESS_FLUSH_MS));

        /* Checked against the header held in RAM rather than one re-read from flash: every
         * chunk written above was already read back and compared by spi_flash_io_write(), so a
         * header that did not land would have failed the copy. What this pass adds is the
         * payload CRC, which nothing has checked until now -- usb_update_file_find()
         * deliberately skips it to avoid a full read at find time. */
        if (compute_staged_payload_crc(file->header.binary_size, &computed_crc) == false) {
          result = USB_UPDATE_STAGE_FLASH_READ_FAILED;
        }
        else if (computed_crc != file->header.crc16_ccit_checksum) {
          app_console_print("[USB] Staged CRC mismatch: expected 0x%04X, computed 0x%04X\r\n",
                            (unsigned int)file->header.crc16_ccit_checksum, (unsigned int)computed_crc);
          result = USB_UPDATE_STAGE_CRC_MISMATCH;
        }
        else {
          app_console_print("[USB] Staged image verified (crc 0x%04X).\r\n", (unsigned int)computed_crc);
          result = USB_UPDATE_STAGE_OK;
        }
      }
    }
  }

  return result;
}

const CHAR *usb_update_stage_status_string(usb_update_stage_status_enum status) {
  const CHAR *description;

  switch (status) {
    case USB_UPDATE_STAGE_OK:
      description = "ok";
      break;

    case USB_UPDATE_STAGE_OPEN_FAILED:
      description = "file would not open";
      break;

    case USB_UPDATE_STAGE_ERASE_FAILED:
      description = "SPI flash erase failed";
      break;

    case USB_UPDATE_STAGE_FILE_READ_FAILED:
      description = "read from the USB drive failed";
      break;

    case USB_UPDATE_STAGE_FLASH_WRITE_FAILED:
      description = "SPI flash write failed";
      break;

    case USB_UPDATE_STAGE_FLASH_READ_FAILED:
      description = "staged image could not be read back";
      break;

    case USB_UPDATE_STAGE_CRC_MISMATCH:
      description = "staged image CRC mismatch";
      break;

    default:
      description = "unknown error";
      break;
  }

  return description;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static void report_progress(uint32_t previous_done, uint32_t current_done, uint32_t total) {
  if (total > 0U) {
    uint32_t previous_step = ((previous_done * 100U) / total) / USB_UPDATE_STAGE_PROGRESS_STEP_PCT;
    uint32_t current_step = ((current_done * 100U) / total) / USB_UPDATE_STAGE_PROGRESS_STEP_PCT;

    if (previous_step != current_step) {
      app_console_print("[USB]   %lu / %lu bytes (%lu%%)\r\n", (unsigned long)current_done, (unsigned long)total,
                        (unsigned long)((current_done * 100U) / total));
      vTaskDelay(pdMS_TO_TICKS(USB_UPDATE_STAGE_PROGRESS_FLUSH_MS));
    }
  }
}

static usb_update_stage_status_enum copy_file_to_staging(uint32_t total_size) {
  usb_update_stage_status_enum result = USB_UPDATE_STAGE_OK;
  uint32_t bytes_done = 0U;

  while ((bytes_done < total_size) && (result == USB_UPDATE_STAGE_OK)) {
    uint32_t remaining = total_size - bytes_done;
    uint32_t chunk_length = (remaining > (uint32_t)sizeof(s_chunk_buffer)) ? (uint32_t)sizeof(s_chunk_buffer) : remaining;
    ULONG bytes_read = 0U;
    UINT fx_status;

    fx_status = fx_file_read(&s_open_file, s_chunk_buffer, (ULONG)chunk_length, &bytes_read);

    if ((fx_status != FX_SUCCESS) || (bytes_read != (ULONG)chunk_length)) {
      /* The directory walk said the file was total_size bytes, so a short read here means the
       * drive went away mid-copy or the volume disagrees with its own directory entry. */
      app_console_print("[USB] Drive read failed at byte %lu of %lu (fx status=0x%X).\r\n", (unsigned long)bytes_done,
                        (unsigned long)total_size, (unsigned int)fx_status);
      result = USB_UPDATE_STAGE_FILE_READ_FAILED;
    }
    else if (spi_flash_io_write(STAGED_HEADER_OFFSET + bytes_done, s_chunk_buffer, chunk_length) == false) {
      app_console_print("[USB] SPI flash write failed at byte %lu of %lu.\r\n", (unsigned long)bytes_done,
                        (unsigned long)total_size);
      result = USB_UPDATE_STAGE_FLASH_WRITE_FAILED;
    }
    else {
      uint32_t previous_done = bytes_done;

      bytes_done += chunk_length;
      report_progress(previous_done, bytes_done, total_size);
    }
  }

  return result;
}

static bool compute_staged_payload_crc(uint32_t binary_size, uint16_t *out_crc) {
  bool read_ok = true;
  uint32_t bytes_remaining = binary_size;
  uint32_t spi_offset = STAGED_HEADER_OFFSET + (uint32_t)sizeof(ede_update_file_header_t);
  uint16_t running_crc = CRC16_CCITT_INITIAL_VALUE;

  while ((bytes_remaining > 0U) && (read_ok == true)) {
    uint32_t chunk_length = (bytes_remaining > (uint32_t)sizeof(s_chunk_buffer)) ? (uint32_t)sizeof(s_chunk_buffer) : bytes_remaining;

    read_ok = spi_flash_io_read(spi_offset, s_chunk_buffer, chunk_length);

    if (read_ok == true) {
      running_crc = crc16_ccitt_update(running_crc, s_chunk_buffer, (size_t)chunk_length);
      spi_offset += chunk_length;
      bytes_remaining -= chunk_length;
    }
  }

  if ((out_crc != NULL) && (read_ok == true)) {
    *out_crc = running_crc;
  }

  return read_ok;
}
