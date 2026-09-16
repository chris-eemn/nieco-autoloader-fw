/**
  ******************************************************************************
  * @file           : mx_uart4.h
  * @brief          : Header for mx_uart4.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_stm32c5xx_hal_drivers_license.md file
  * in the same directory as the generated code.
  * If no mx_stm32c5xx_hal_drivers_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MX_UART4_H
#define MX_UART4_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for UART4_RX pin */
#define CLI_RX_PORT                           HAL_GPIOD
#define CLI_RX_PIN                            HAL_GPIO_PIN_11

/** Primary aliases for UART4_TX pin */
#define CLI_TX_PORT                           HAL_GPIOD
#define CLI_TX_PIN                            HAL_GPIO_PIN_1

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/******************************************************************************/
/* Exported functions for UART in HAL layer */
/******************************************************************************/
/**
  * @brief mx_uart4_uart init function
  * This function configures the hardware resources used in this example
  * @retval pointer to handle or NULL in case of failure
  */
hal_uart_handle_t *mx_uart4_uart_init(void);

/**
  * @brief  De-initialize mx_uart4_uart instance and return it.
  * @retval None
  */
void mx_uart4_uart_deinit(void);

/**
  * @brief  Get the mx_uart4_uart object.
  * @retval Pointer on the mx_uart4_uartHandle
  */
hal_uart_handle_t *mx_uart4_uart_gethandle(void);

/******************************************************************************/
/*                           UART4 global interrupt                           */
/******************************************************************************/
void UART4_IRQHandler(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_UART4_H */
