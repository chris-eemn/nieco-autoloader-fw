/**
 * @file usart3_stream.c
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
#include <stdbool.h>
#include "usart3_stream.h"
#include "app_console.h"
#include "spi_flash_io.h"
#include "update_image.h"
#include "crc16_ccitt.h"
#include "stm32_hal.h"
#include "mx_usart3.h"

#include "FreeRTOS.h"
#include "task.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/
#define USART3_STREAM_CHUNK_SIZE_BYTES 512u
#define USART3_STREAM_CHUNK_TIMEOUT_MS 2000u
#define USART3_STREAM_LENGTH_PREFIX_BYTES 4u

/* The loader runs USART3 at 115200 rather than the modbus default of 57600 -- this matches
 * the bootloader's original USART3 receiver and the host-side sender tooling, so the same
 * host script works whether the image is streamed to the bootloader or (now) to the app. */
#define USART3_STREAM_BAUD_RATE 115200u

/* Sent back to the host over USART3 once the staging erase completes and after every chunk
 * is written to SPI flash, so the host never sends ahead of what the firmware is ready to
 * receive. USART3's receive register is a single byte deep with no DMA/interrupt buffering
 * behind it, and spi_flash_io_write()/erase busy-wait on SPI for several milliseconds
 * without touching UART at all -- without a per-chunk ack, any bytes the host sends during
 * that window are silently overrun and lost. */
#define USART3_STREAM_READY_ACK_BYTE 0xA5u
#define USART3_STREAM_READY_ACK_TIMEOUT_MS 200u

/* This UART link turned out unreliable enough in practice (data landing wrong somewhere in
 * the middle of large multi-chunk transfers) that catching it at the protocol level is
 * cheaper than chasing the root cause -- this is a stand-in transport until USB is up, not
 * worth over-investing in. Each chunk is followed by a 2-byte little-endian CRC16-CCITT of
 * that chunk (same algorithm as the payload CRC, see crc16_ccitt.h); a mismatch gets NAKed so
 * the host resends the same chunk, up to a retry cap. */
#define USART3_STREAM_CHUNK_CRC_BYTES 2u
#define USART3_STREAM_NAK_BYTE 0x5Au
#define USART3_STREAM_MAX_CHUNK_RETRIES 5u

/* Progress feedback over the app console. The loader task runs above the console TX task and
 * busy-waits inside the HAL while receiving, so a queued line won't actually flush until the
 * loader yields. Progress is therefore printed only at points where the host is waiting for an
 * ack (before the erase, and before each chunk's ack) -- never during active reception -- and
 * followed by a short delay so the lower-priority console TX task can drain the line. The
 * delay is safe there precisely because no inbound bytes arrive until the firmware acks.
 * Printed roughly every USART3_STREAM_PROGRESS_STEP_PCT percent so large transfers don't flood
 * the (drop-if-full) console queue. */
#define USART3_STREAM_PROGRESS_STEP_PCT 10u
#define USART3_STREAM_PROGRESS_FLUSH_MS 6u

/* Temporary bring-up diagnostic. When non-zero, usart3_stream_init() dumps the live USART3
 * config registers to the app console and transmits a test string out of USART3 itself, so
 * a terminal on the USART3 port confirms TX works (isolating any problem to the RX side) and
 * the register dump confirms the receiver is enabled and the baud divisor is correct. Set to
 * 0 to remove once the link is verified. */
#define USART3_STREAM_DEBUG 0

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/
static uint8_t chunk_buffer[USART3_STREAM_CHUNK_SIZE_BYTES];

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
/**
 * @brief receives one chunk's data bytes followed by its 2-byte little-endian CRC16-CCITT
 *        into chunk_buffer, NAKing and retrying the whole chunk on a CRC mismatch
 * @param huart3 USART3 handle; must not be NULL
 * @param chunk_len number of data bytes expected (excludes the trailing CRC bytes)
 * @return bool true if a CRC-verified chunk ended up in chunk_buffer within the retry cap
 */
static bool receive_verified_chunk(hal_uart_handle_t *huart3, uint32_t chunk_len);

/**
 * @brief streams the staged payload back out of SPI flash and computes its CRC16-CCITT,
 *        without holding more than one chunk in SRAM at a time
 * @param payload_offset SPI-flash byte offset of the first payload byte
 * @param binary_size payload length in bytes
 * @param out_crc destination for the computed CRC; must not be NULL
 * @return bool true if every chunk read from SPI flash succeeded
 */
static bool compute_staged_payload_crc(uint32_t payload_offset, uint32_t binary_size, uint16_t *out_crc);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/
bool usart3_stream_init(void) {
  hal_uart_handle_t *huart3 = mx_usart3_uart_gethandle();
  hal_uart_config_t uart_config;
  bool init_ok = false;

  if (huart3 != NULL) {
    /* Same USART3 configuration mx_usart3_uart_init() applied, but at the loader's baud rate
     * instead of the modbus default. The peripheral clock, GPIO, and NVIC setup done by
     * mx_usart3_uart_init() (in mx_system_init()) remain in effect. */
    uart_config.baud_rate = USART3_STREAM_BAUD_RATE;
    uart_config.clock_prescaler = HAL_UART_PRESCALER_DIV1;
    uart_config.word_length = HAL_UART_WORD_LENGTH_8_BIT;
    uart_config.stop_bits = HAL_UART_STOP_BIT_1;
    uart_config.parity = HAL_UART_PARITY_NONE;
    uart_config.direction = HAL_UART_DIRECTION_TX_RX;
    uart_config.hw_flow_ctl = HAL_UART_HW_CONTROL_NONE;
    uart_config.oversampling = HAL_UART_OVERSAMPLING_16;
    uart_config.one_bit_sampling = HAL_UART_ONE_BIT_SAMPLE_DISABLE;

    if (HAL_UART_SetConfig(huart3, &uart_config) == HAL_OK) {
      /* Enable the RX FIFO. USART3's receive register is otherwise a single byte deep, and the
       * loader polls for the first byte from a FreeRTOS task rather than a tight bare-metal
       * loop -- so a host that sends the 4-byte length prefix back-to-back can land bytes 2-4
       * before the task's next poll reads byte 1, overrunning the receiver so the transfer
       * never starts. The protocol is stop-and-wait (the host never sends more than the prefix,
       * and later a single chunk, before waiting for an ack), so the 8-deep FIFO always has
       * room to cover the gap between polls. */
      init_ok = (HAL_UART_EnableFifoMode(huart3) == HAL_OK);

      if (init_ok == true) {
        (void)HAL_UART_SetRxFifoThreshold(huart3, HAL_UART_FIFO_THRESHOLD_1_8);
        (void)HAL_UART_SetTxFifoThreshold(huart3, HAL_UART_FIFO_THRESHOLD_1_8);

#if (USART3_STREAM_DEBUG != 0)
        {
          static const uint8_t tx_test[] = "usart3 loader tx test @115200\r\n";

          /* Dumped over the app console (USART2). CR1 bit0 = UE (USART enable), bit2 = RE
           * (receiver enable), bit3 = TE. BRR is the baud divisor = kernel_clk / baud for
           * oversampling-16 (e.g. 48 MHz / 115200 = 416 = 0x1A0). ISR bit3 = ORE, bit1 = FE. */
          app_console_print("usart3 dbg: CR1=0x%08lX, BRR=0x%08lX, ISR=0x%08lX\r\n", (unsigned long)USART3->CR1,
                            (unsigned long)USART3->BRR, (unsigned long)USART3->ISR);

          /* Sent out of USART3 itself: if this shows up on a terminal on the USART3 port, TX
           * (and thus pins/baud/clock) is good and any remaining problem is on the RX side. */
          (void)HAL_UART_Transmit(huart3, tx_test, (uint32_t)(sizeof(tx_test) - 1u), 100u);
        }
#endif
      }
    }
  }

  return init_ok;
}

bool usart3_stream_receive_update(void) {
  hal_uart_handle_t *huart3 = mx_usart3_uart_gethandle();
  uint8_t length_prefix[USART3_STREAM_LENGTH_PREFIX_BYTES];
  uint32_t total_length;
  uint32_t bytes_received = 0u;
  uint32_t chunk_len;
  uint8_t ready_ack;
  bool success;

  if (huart3 == NULL) {
    return false;
  }

  /* Non-blocking peek for the first byte -- this function is polled from the loader task, so
   * it must return immediately when no transfer is in progress rather than blocking on
   * USART3_STREAM_CHUNK_TIMEOUT_MS every call. Once the first byte of a real transfer has
   * arrived, blocking for the rest is fine. */
  if (HAL_UART_Receive(huart3, &length_prefix[0], 1u, 0u) != HAL_OK) {
    return false;
  }

  if (HAL_UART_Receive(huart3, &length_prefix[1], USART3_STREAM_LENGTH_PREFIX_BYTES - 1u,
                       USART3_STREAM_CHUNK_TIMEOUT_MS) != HAL_OK) {
    app_console_print("usart3 stream: timed out waiting for length prefix\r\n");
    return false;
  }

  total_length = (uint32_t)length_prefix[0] | ((uint32_t)length_prefix[1] << 8) | ((uint32_t)length_prefix[2] << 16) |
                 ((uint32_t)length_prefix[3] << 24);

  /* Safe to print+flush here: the host has sent only the length prefix and is now waiting for
   * the post-erase ready ack, so nothing arrives on USART3 during the delay. */
  app_console_print("usart3 stream: incoming image, %lu bytes; erasing staging area...\r\n",
                    (unsigned long)total_length);
  vTaskDelay(pdMS_TO_TICKS(USART3_STREAM_PROGRESS_FLUSH_MS));

  if (spi_flash_io_erase_range(STAGED_HEADER_OFFSET, total_length) == false) {
    app_console_print("usart3 stream: SPI flash erase failed\r\n");
    return false;
  }

  ready_ack = USART3_STREAM_READY_ACK_BYTE;

  if (HAL_UART_Transmit(huart3, &ready_ack, 1u, USART3_STREAM_READY_ACK_TIMEOUT_MS) != HAL_OK) {
    app_console_print("usart3 stream: failed to send ready ack\r\n");
    return false;
  }

  success = true;

  while ((bytes_received < total_length) && (success == true)) {
    chunk_len = ((total_length - bytes_received) > sizeof(chunk_buffer)) ? (uint32_t)sizeof(chunk_buffer)
                                                                          : (total_length - bytes_received);

    if (receive_verified_chunk(huart3, chunk_len) == false) {
      app_console_print("usart3 stream: chunk failed CRC verification at byte %lu of %lu\r\n",
                        (unsigned long)bytes_received, (unsigned long)total_length);
      success = false;
    } else if (spi_flash_io_write(STAGED_HEADER_OFFSET + bytes_received, chunk_buffer, chunk_len) == false) {
      app_console_print("usart3 stream: SPI flash write failed at byte %lu of %lu\r\n", (unsigned long)bytes_received,
                        (unsigned long)total_length);
      success = false;
    } else {
      uint32_t prev_received = bytes_received;
      bytes_received += chunk_len;

      /* Emit a progress line each time we cross a USART3_STREAM_PROGRESS_STEP_PCT boundary.
       * Placed before this chunk's ack -- the host is waiting for the ack, so the flush delay
       * doesn't risk overrunning the receiver. */
      if ((total_length > 0u) && (((prev_received * 100u) / total_length / USART3_STREAM_PROGRESS_STEP_PCT) !=
                                  ((bytes_received * 100u) / total_length / USART3_STREAM_PROGRESS_STEP_PCT))) {
        app_console_print("usart3 stream: %lu / %lu bytes (%lu%%)\r\n", (unsigned long)bytes_received,
                          (unsigned long)total_length, (unsigned long)((bytes_received * 100u) / total_length));
        vTaskDelay(pdMS_TO_TICKS(USART3_STREAM_PROGRESS_FLUSH_MS));
      }

      ready_ack = USART3_STREAM_READY_ACK_BYTE;

      if (HAL_UART_Transmit(huart3, &ready_ack, 1u, USART3_STREAM_READY_ACK_TIMEOUT_MS) != HAL_OK) {
        app_console_print("usart3 stream: failed to send chunk ack at byte %lu of %lu\r\n",
                          (unsigned long)bytes_received, (unsigned long)total_length);
        success = false;
      }
    }
  }

  if (success == true) {
    app_console_print("usart3 stream: received %lu bytes successfully\r\n", (unsigned long)bytes_received);
  } else {
    app_console_print("usart3 stream: failed after %lu of %lu bytes\r\n", (unsigned long)bytes_received,
                      (unsigned long)total_length);
  }

  return success;
}

bool usart3_stream_validate_staged_crc(uint16_t *out_expected, uint16_t *out_computed) {
  ede_update_file_header_t header;
  uint32_t payload_offset;
  bool read_ok = false;

  if ((out_expected != NULL) && (out_computed != NULL)) {
    if (spi_flash_io_read(STAGED_HEADER_OFFSET, (uint8_t *)&header, (uint32_t)sizeof(header)) == true) {
      payload_offset = STAGED_HEADER_OFFSET + (uint32_t)sizeof(header);
      read_ok = compute_staged_payload_crc(payload_offset, header.binary_size, out_computed);

      if (read_ok == true) {
        *out_expected = header.crc16_ccit_checksum;
      }
    }
  }

  return read_ok;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static bool receive_verified_chunk(hal_uart_handle_t *huart3, uint32_t chunk_len) {
  uint8_t crc_bytes[USART3_STREAM_CHUNK_CRC_BYTES];
  uint16_t received_crc;
  uint16_t computed_crc;
  uint32_t attempt;
  uint8_t nak;
  bool verified = false;
  bool comm_ok = true;

  for (attempt = 0u; (attempt < USART3_STREAM_MAX_CHUNK_RETRIES) && (verified == false) && (comm_ok == true);
       attempt++) {
    if (HAL_UART_Receive(huart3, chunk_buffer, chunk_len, USART3_STREAM_CHUNK_TIMEOUT_MS) != HAL_OK) {
      app_console_print("usart3 stream: chunk data timed out\r\n");
      comm_ok = false;
    } else if (HAL_UART_Receive(huart3, crc_bytes, (uint32_t)USART3_STREAM_CHUNK_CRC_BYTES,
                                USART3_STREAM_CHUNK_TIMEOUT_MS) != HAL_OK) {
      app_console_print("usart3 stream: chunk CRC bytes timed out\r\n");
      comm_ok = false;
    } else {
      received_crc = (uint16_t)crc_bytes[0] | ((uint16_t)crc_bytes[1] << 8);
      computed_crc = crc16_ccitt_update(CRC16_CCITT_INITIAL_VALUE, chunk_buffer, chunk_len);

      if (computed_crc == received_crc) {
        verified = true;
      } else {
        app_console_print("usart3 stream: chunk CRC mismatch (expected 0x%04X, got 0x%04X), requesting resend\r\n",
                          received_crc, computed_crc);
        nak = USART3_STREAM_NAK_BYTE;

        if (HAL_UART_Transmit(huart3, &nak, 1u, USART3_STREAM_READY_ACK_TIMEOUT_MS) != HAL_OK) {
          app_console_print("usart3 stream: failed to send chunk nak\r\n");
          comm_ok = false;
        }
      }
    }
  }

  return ((verified == true) && (comm_ok == true));
}

static bool compute_staged_payload_crc(uint32_t payload_offset, uint32_t binary_size, uint16_t *out_crc) {
  bool read_ok = true;
  uint32_t bytes_remaining = binary_size;
  uint32_t spi_offset = payload_offset;
  uint32_t chunk_len;
  uint16_t running_crc = CRC16_CCITT_INITIAL_VALUE;

  /* Reuses chunk_buffer -- validation runs only after usart3_stream_receive_update() has
   * fully returned, so there is no overlap with the receive path's use of the buffer. */
  while ((bytes_remaining > 0u) && (read_ok == true)) {
    chunk_len = (bytes_remaining > sizeof(chunk_buffer)) ? (uint32_t)sizeof(chunk_buffer) : bytes_remaining;
    read_ok = spi_flash_io_read(spi_offset, chunk_buffer, chunk_len);

    if (read_ok == true) {
      running_crc = crc16_ccitt_update(running_crc, chunk_buffer, chunk_len);
      spi_offset += chunk_len;
      bytes_remaining -= chunk_len;
    }
  }

  if ((out_crc != NULL) && (read_ok == true)) {
    *out_crc = running_crc;
  }

  return read_ok;
}
