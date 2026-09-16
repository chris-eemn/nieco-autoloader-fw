/**
 * @file app_console_port.h
 * @author Chris Owens (cowens@eemn.io)
 * @brief Hardware abstraction interface for the console UART. Replace the
 *        paired .c to retarget to a different peripheral.
 * @version 0.1
 * @date 2026-06-23
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

#ifndef APP_CONSOLE_PORT_H_
#define APP_CONSOLE_PORT_H_

/*******************************************************************************
 * Includes
 *******************************************************************************/

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
 * Function Prototypes
 *******************************************************************************/

/**
 * @brief Initialise the underlying UART hardware. Called once from APP_Console_Init().
 */
void console_port_init(void);

/**
 * @brief Blocking transmit of len bytes over the console UART.
 * @param buf Pointer to the data to transmit.
 * @param len Number of bytes to send.
 * @param timeout_ms Maximum time to wait in milliseconds.
 * @return 0 on success, -1 on error.
 */
int32_t console_port_transmit(const uint8_t* buf, uint16_t len, uint32_t timeout_ms);

/**
 * @brief Blocking receive of one byte from the console UART.
 * @param byte Pointer to store the received byte.
 * @param timeout_ms Maximum time to wait in milliseconds.
 * @return 0 on success, -1 on timeout or error.
 */
int32_t console_port_receive(uint8_t* byte, uint32_t timeout_ms);

#endif /* APP_CONSOLE_PORT_H_ */
