/**
  ******************************************************************************
  * @file           : mx_tim15.h
  * @brief          : Header for mx_tim15.c file.
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
#ifndef MX_TIM15_H
#define MX_TIM15_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for TIM15_CH2 pin */
#define ENC3_CHB_PORT                         HAL_GPIOE
#define ENC3_CHB_PIN                          HAL_GPIO_PIN_6

/** Primary aliases for TIM15_CH1 pin */
#define ENC3_CHA_PORT                         HAL_GPIOC
#define ENC3_CHA_PIN                          HAL_GPIO_PIN_12

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for TIM in HAL layer */
/******************************************************************************/
/**
  * @brief  mx_tim15 init function.
  *         This function configures the hardware resources used in this example.
  * @retval Pointer to handle
  * @retval NULL in case of failure
  */
hal_tim_handle_t *mx_tim15_init(void);

/**
  * @brief  De-initialize mx_tim15 instance and return it.
  */
void mx_tim15_deinit(void);

/**
  * @brief  Get the mx_tim15 object.
  * @return Pointer on the mx_tim15 Handle
  */
hal_tim_handle_t *mx_tim15_gethandle(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_TIM15_H */
