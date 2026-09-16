/**
  ******************************************************************************
  * @file           : mx_lptim1.h
  * @brief          : Header for mx_lptim1.c file.
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
#ifndef MX_LPTIM1_H
#define MX_LPTIM1_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for LPTIM1_CH2 pin */
#define PD10_PORT                             HAL_GPIOD
#define PD10_PIN                              HAL_GPIO_PIN_10

/** Primary aliases for LPTIM1_IN1 pin */
#define PG12_PORT                             HAL_GPIOG
#define PG12_PIN                              HAL_GPIO_PIN_12

/** Primary aliases for LPTIM1_IN2 pin */
#define PE1_PORT                              HAL_GPIOE
#define PE1_PIN                               HAL_GPIO_PIN_1

/** Primary aliases for LPTIM1_CH1 pin */
#define PG13_PORT                             HAL_GPIOG
#define PG13_PIN                              HAL_GPIO_PIN_13

#define M8_ENCODER_TIMER_IRQN       I2C1_ERR_IRQn
#define M8_ENCODER_TIMER_IRQHANDLER I2C1_ERR_IRQHandler
#define MYLPTIM_1_IRQN       M8_ENCODER_TIMER_IRQN
#define MYLPTIM_1_IRQHANDLER M8_ENCODER_TIMER_IRQHANDLER
/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

/******************************************************************************/
/* Exported functions for SW instance in HAL layer */
/******************************************************************************/
/**
  * @brief mx_lptim1 init function
  * This function configures the hardware resources used in this example
  * @retval pointer to handle or NULL in case of failure
  */
hal_lptim_handle_t *mx_lptim1_init(void);

/**
  * @brief  De-initialize mx_lptim1 instance and return it.
  */
void mx_lptim1_deinit(void);

/**
  * @brief  Get the mx_lptim1 object.
  * @retval Pointer on the mx_lptim1 Handle
  */
hal_lptim_handle_t *mx_lptim1_gethandle(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_LPTIM1_H */
