/**
  ******************************************************************************
  * @file           : mx_usart3.h
  * @brief          : Header for mx_usart3.c file.
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
#ifndef MX_USART3_H
#define MX_USART3_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for USART3_RX pin */
#define UI_MB_RX_PORT                         HAL_GPIOB
#define UI_MB_RX_PIN                          HAL_GPIO_PIN_4

/** Primary aliases for USART3_TX pin */
#define UI_MB_TX_PORT                         HAL_GPIOB
#define UI_MB_TX_PIN                          HAL_GPIO_PIN_10

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/******************************************************************************/
/* Exported functions for UART in HAL layer */
/******************************************************************************/
/**
  * @brief mx_usart3_uart init function
  * This function configures the hardware resources used in this example
  * @retval pointer to handle or NULL in case of failure
  */
hal_uart_handle_t *mx_usart3_uart_init(void);

/**
  * @brief  De-initialize mx_usart3_uart instance and return it.
  * @retval None
  */
void mx_usart3_uart_deinit(void);

/**
  * @brief  Get the mx_usart3_uart object.
  * @retval Pointer on the mx_usart3_uartHandle
  */
hal_uart_handle_t *mx_usart3_uart_gethandle(void);

/******************************************************************************/
/*                          USART3 global interrupt                           */
/******************************************************************************/
void USART3_IRQHandler(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_USART3_H */
