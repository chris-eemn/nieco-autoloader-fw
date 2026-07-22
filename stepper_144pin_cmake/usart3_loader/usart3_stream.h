/**
 * @file usart3_stream.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Binary stream receiver for USART3 -- the data-only channel that never carries text.
 *        Streams an incoming header+payload blob straight into the SPI flash staging slot,
 *        fixed-size chunk at a time, without ever buffering the whole thing in SRAM.
 *
 *        This is the same protocol the bootloader originally hosted; it has been moved into
 *        the application so the running app stages an incoming image into SPI flash, and the
 *        bootloader (on the next commit/reset) applies it to internal flash. When the app is
 *        built in this mode, USART3 belongs to the loader instead of the modbus slave (see
 *        usart3_loader.h / USART3_MODE).
 * @version 0.1
 * @date 2026-07-22
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef USART3_STREAM_H_
#define USART3_STREAM_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/
#include <stdbool.h>
#include <stdint.h>

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
 * @brief reconfigures USART3 for the binary stream -- notably raises the baud rate from the
 *        modbus default (57600) to the loader's 115200, matching the bootloader's original
 *        USART3 receiver and the host-side sender tooling
 * @note call once before the first usart3_stream_receive_update(). USART3 must already have
 *       been brought up by mx_usart3_uart_init() (done in mx_system_init()); this only
 *       overrides the settings that differ for the loader role.
 * @return bool true if the reconfiguration succeeded
 */
bool usart3_stream_init(void);

/**
 * @brief peeks for a transfer on USART3 and, if one has started, reads and stages the whole
 *        header+payload blob into the fixed SPI-flash staging slot
 * @note returns immediately when no transfer is in progress (non-blocking peek for the first
 *       byte), so it is safe to poll from a loop. Once the first byte of a real transfer has
 *       arrived, blocking for the rest is fine.
 *
 *       This is a stop-and-wait protocol: once the staging erase completes, and again after
 *       every chunk is written to SPI flash, a single ready-ack byte is sent back over
 *       USART3. The host must wait for each ack before sending the next chunk -- erasing and
 *       writing to SPI flash can each take longer than a chunk takes to arrive at the UART
 *       baud rate, and USART3's single-byte receive register has no DMA/interrupt buffering
 *       behind it, so sending ahead of the ack overruns the receiver and silently drops
 *       bytes. Prints a one-line success/failure summary over the app console when done;
 *       USART3 itself never carries text apart from the single-byte acks.
 * @return bool true only if a complete image was staged into SPI flash this call; false when
 *         idle (no transfer in progress) or when a transfer failed partway through
 */
bool usart3_stream_receive_update(void);

/**
 * @brief re-reads the just-staged image back out of SPI flash and recomputes its payload
 *        CRC16-CCITT, reporting both the expected (from the header) and computed values so
 *        the caller can decide whether to commit
 * @note this is an app-side pre-commit guard -- the bootloader independently re-validates the
 *       CRC before it programs internal flash, so this only avoids committing (and rebooting
 *       for) an image we can already tell is corrupt. Streams the payload through the module
 *       chunk buffer; does not buffer the whole payload in SRAM.
 * @param out_expected destination for the header's stored payload CRC; must not be NULL
 * @param out_computed destination for the CRC recomputed over the staged payload; must not be NULL
 * @return bool true if the header and payload were read back successfully (a valid image is
 *         then indicated by *out_expected == *out_computed); false on any SPI-flash read failure
 */
bool usart3_stream_validate_staged_crc(uint16_t *out_expected, uint16_t *out_computed);

#endif /* USART3_STREAM_H_ */
