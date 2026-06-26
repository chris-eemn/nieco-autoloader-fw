/**
  ******************************************************************************
  * @file           : mx_gpio_default.h
  * @brief          : Header for mx_gpio_default.c file.
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
#ifndef MX_GPIO_DEFAULT_H
#define MX_GPIO_DEFAULT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"
#include "mx_def.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/******************************************************************************/
/* Exported defines for gpio_default in HAL layer                             */
/******************************************************************************/

/* Primary aliases for GPIO PA0 pin */
#define PA0_PORT                                        HAL_GPIOA
#define PA0_PIN                                         HAL_GPIO_PIN_0
#define PA0_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PA0_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PA0_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PA0 pin */
#define M1_NSLP_PORT                                    HAL_GPIOA
#define M1_NSLP_PIN                                     HAL_GPIO_PIN_0
#define M1_NSLP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M1_NSLP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M1_NSLP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

#define M1_NSLP_PORT                                    HAL_GPIOA
#define M1_NSLP_PIN                                     HAL_GPIO_PIN_0
#define M1_NSLP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M1_NSLP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M1_NSLP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PA10 pin */
#define PA10_PORT                                       HAL_GPIOA
#define PA10_PIN                                        HAL_GPIO_PIN_10
#define PA10_INIT_STATE                                 HAL_GPIO_PIN_RESET
#define PA10_ACTIVE_STATE                               HAL_GPIO_PIN_SET
#define PA10_INACTIVE_STATE                             HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PA10 pin */
#define M1_EN_PORT                                      HAL_GPIOA
#define M1_EN_PIN                                       HAL_GPIO_PIN_10
#define M1_EN_INIT_STATE                                HAL_GPIO_PIN_RESET
#define M1_EN_ACTIVE_STATE                              HAL_GPIO_PIN_SET
#define M1_EN_INACTIVE_STATE                            HAL_GPIO_PIN_RESET

#define M1_EN_PORT                                      HAL_GPIOA
#define M1_EN_PIN                                       HAL_GPIO_PIN_10
#define M1_EN_INIT_STATE                                HAL_GPIO_PIN_RESET
#define M1_EN_ACTIVE_STATE                              HAL_GPIO_PIN_SET
#define M1_EN_INACTIVE_STATE                            HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PC6 pin */
#define PC6_PORT                                        HAL_GPIOC
#define PC6_PIN                                         HAL_GPIO_PIN_6
#define PC6_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PC6_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PC6_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PC6 pin */
#define M1_DIR_PORT                                     HAL_GPIOC
#define M1_DIR_PIN                                      HAL_GPIO_PIN_6
#define M1_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M1_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M1_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

#define M1_DIR_PORT                                     HAL_GPIOC
#define M1_DIR_PIN                                      HAL_GPIO_PIN_6
#define M1_DIR_INIT_STATE                               HAL_GPIO_PIN_RESET
#define M1_DIR_ACTIVE_STATE                             HAL_GPIO_PIN_SET
#define M1_DIR_INACTIVE_STATE                           HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD5 pin */
#define PD5_PORT                                        HAL_GPIOD
#define PD5_PIN                                         HAL_GPIO_PIN_5
#define PD5_INIT_STATE                                  HAL_GPIO_PIN_RESET
#define PD5_ACTIVE_STATE                                HAL_GPIO_PIN_SET
#define PD5_INACTIVE_STATE                              HAL_GPIO_PIN_RESET

/* Secondary aliases for GPIO PD5 pin */
#define M1_STEP_PORT                                    HAL_GPIOD
#define M1_STEP_PIN                                     HAL_GPIO_PIN_5
#define M1_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M1_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M1_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

#define M1_STEP_PORT                                    HAL_GPIOD
#define M1_STEP_PIN                                     HAL_GPIO_PIN_5
#define M1_STEP_INIT_STATE                              HAL_GPIO_PIN_RESET
#define M1_STEP_ACTIVE_STATE                            HAL_GPIO_PIN_SET
#define M1_STEP_INACTIVE_STATE                          HAL_GPIO_PIN_RESET

/* Primary aliases for GPIO PD6 pin */
#define PD6_PORT                                        HAL_GPIOD
#define PD6_PIN                                         HAL_GPIO_PIN_6

/* Secondary aliases for GPIO PD6 pin */
#define M1_NFAULT_PORT                                  HAL_GPIOD
#define M1_NFAULT_PIN                                   HAL_GPIO_PIN_6

#define M1_NFAULT_PORT                                  HAL_GPIOD
#define M1_NFAULT_PIN                                   HAL_GPIO_PIN_6

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for gpio_default in HAL layer                           */
/******************************************************************************/
/**
  * @brief mx_gpio_default init function
  * This function configures the hardware resources used in this example
  * @retval 0  GPIO group correctly initialized
  * @retval -1 Issue detected during GPIO group initialization
  */
system_status_t mx_gpio_default_init(void);

/**
  * @brief  De-initialize gpio_default instance.
  */
system_status_t mx_gpio_default_deinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_GPIO_DEFAULT_H */
