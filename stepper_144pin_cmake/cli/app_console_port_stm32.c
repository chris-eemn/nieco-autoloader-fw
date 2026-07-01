/**
 * @file app_console_port_stm32.c
 * @author Chris Owens (cowens@eemn.io)
 * @brief STM32 HAL2 implementation of the console port interface.
 *        RX is interrupt-driven via HAL_UART_Receive_IT() and a binary
 *        semaphore. To retarget to a different UART change
 *        CONSOLE_UART_GETHANDLE and the matching mx_usartX.h include.
 * @version 0.1
 * @date 2026-06-23
 *
 * @copyright Copyright (c) 2026 Embedded Design Solutions, LLC.  All Rights Reserved.
 *
 */

/*******************************************************************************
 * Includes
 *******************************************************************************/

#include "app_console_port.h"
#include "mx_usart2.h"

#include "FreeRTOS.h"
#include "semphr.h"

/*******************************************************************************
 * Module Macros
 *******************************************************************************/

/* Change this macro (and the matching #include above) to retarget the console
 * to a different UART peripheral. Everything else stays the same. */
#define CONSOLE_UART_GETHANDLE mx_usart2_uart_gethandle

/*******************************************************************************
 * Module Typedefs
 *******************************************************************************/

/*******************************************************************************
 * Module Variable Definitions
 *******************************************************************************/

static SemaphoreHandle_t s_rxDoneSem;
static volatile uint8_t s_rxByte;
static volatile int32_t s_rxOk;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void console_uart_rx_cplt_cb(hal_uart_handle_t *huart, uint32_t size_byte, hal_uart_rx_event_types_t rx_event);
static void console_uart_error_cb(hal_uart_handle_t *huart);

/*******************************************************************************
 * Public Function Definitions
 *******************************************************************************/

void console_port_init(void) {
  /* mx_usart2_uart_init() is called from mx_system_init() before the
   * scheduler starts, so the peripheral itself needs no setup here. */
  s_rxDoneSem = xSemaphoreCreateBinary();
  configASSERT(s_rxDoneSem != NULL);
  configASSERT(HAL_UART_RegisterRxCpltCallback(CONSOLE_UART_GETHANDLE(), console_uart_rx_cplt_cb) == HAL_OK);
  configASSERT(HAL_UART_RegisterErrorCallback(CONSOLE_UART_GETHANDLE(), console_uart_error_cb) == HAL_OK);
}

int32_t console_port_transmit(const uint8_t* buf, uint16_t len, uint32_t timeout_ms) {
  hal_status_t status = HAL_UART_Transmit(CONSOLE_UART_GETHANDLE(), buf, len, timeout_ms);
  return (status == HAL_OK) ? 0 : -1;
}

int32_t console_port_receive(uint8_t* byte, uint32_t timeout_ms) {
  TickType_t ticks = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

  if (HAL_UART_Receive_IT(CONSOLE_UART_GETHANDLE(), (uint8_t*)&s_rxByte, 1) != HAL_OK) {
    return -1;
  }

  if (xSemaphoreTake(s_rxDoneSem, ticks) != pdTRUE) {
    /* Timed out — cancel the pending receive so the next call starts clean. */
    (void)HAL_UART_AbortReceive(CONSOLE_UART_GETHANDLE());
    return -1;
  }

  if (s_rxOk == 0) {
    return -1;
  }

  *byte = s_rxByte;
  return 0;
}

/*******************************************************************************
 * Private Function Definitions
 *******************************************************************************/

/**
 * @brief UART RX-complete callback registered on the console UART handle.
 *        Called from IRQ context on successful single-byte receive.
 */
static void console_uart_rx_cplt_cb(hal_uart_handle_t *huart, uint32_t size_byte, hal_uart_rx_event_types_t rx_event) {
  (void)huart;
  (void)size_byte;
  (void)rx_event;
  BaseType_t higher_priority_task_woken = pdFALSE;
  s_rxOk = 1;
  xSemaphoreGiveFromISR(s_rxDoneSem, &higher_priority_task_woken);
  portYIELD_FROM_ISR(higher_priority_task_woken);
}

/**
 * @brief UART error callback registered on the console UART handle.
 *        Called from IRQ context on framing/parity/overrun/noise error.
 */
static void console_uart_error_cb(hal_uart_handle_t *huart) {
  (void)huart;
  BaseType_t higher_priority_task_woken = pdFALSE;
  s_rxOk = 0;
  xSemaphoreGiveFromISR(s_rxDoneSem, &higher_priority_task_woken);
  portYIELD_FROM_ISR(higher_priority_task_woken);
}
